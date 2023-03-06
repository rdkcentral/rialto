/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdexcept>

#include "MediaKeySession.h"
#include "MediaKeysCommon.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
std::shared_ptr<IMediaKeySessionFactory> IMediaKeySessionFactory::createFactory()
{
    std::shared_ptr<IMediaKeySessionFactory> factory;

    try
    {
        factory = std::make_shared<MediaKeySessionFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media key session factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IMediaKeySession>
MediaKeySessionFactory::createMediaKeySession(const std::string &keySystem, int32_t keySessionId,
                                              const IOcdmSystem &ocdmSystem, KeySessionType sessionType,
                                              std::weak_ptr<IMediaKeysClient> client, bool isLDL) const
{
    std::unique_ptr<IMediaKeySession> mediaKeys;
    try
    {
        mediaKeys = std::make_unique<server::MediaKeySession>(keySystem, keySessionId, ocdmSystem, sessionType, client,
                                                              isLDL, server::IMainThreadFactory::createFactory());
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media key session, reason: %s", e.what());
    }

    return mediaKeys;
}

MediaKeySession::MediaKeySession(const std::string &keySystem, int32_t keySessionId, const IOcdmSystem &ocdmSystem,
                                 KeySessionType sessionType, std::weak_ptr<IMediaKeysClient> client, bool isLDL,
                                 const std::shared_ptr<IMainThreadFactory> &mainThreadFactory)
    : m_kKeySystem(keySystem), m_kKeySessionId(keySessionId), m_kSessionType(sessionType), m_mediaKeysClient(client),
      m_kIsLDL(isLDL), m_isSessionConstructed(false), m_licenseRequested(false)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    m_mainThread = mainThreadFactory->getMainThread();
    if (!m_mainThread)
    {
        throw std::runtime_error("Failed to get the main thread");
    }
    m_mainThreadClientId = m_mainThread->registerClient();

    m_ocdmSession = ocdmSystem.createSession(this);
    if (!m_ocdmSession)
    {
        throw std::runtime_error("Ocdm session could not be created");
    }
}

MediaKeySession::~MediaKeySession()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (m_isSessionConstructed)
    {
        static_cast<void>(closeKeySession());
    }

    m_mainThread->unregisterClient(m_mainThreadClientId);
}

MediaKeyErrorStatus MediaKeySession::generateRequest(InitDataType initDataType, const std::vector<uint8_t> &initData)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    // Set the request flag for the onLicenseRequest callback
    m_licenseRequested = true;

    // Only construct session if it hasnt previously been constructed
    if (!m_isSessionConstructed)
    {
        MediaKeyErrorStatus status =
            m_ocdmSession->constructSession(m_kSessionType, initDataType, &initData[0], initData.size());
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to construct the key session");
            m_licenseRequested = false;
        }
        else
        {
            m_isSessionConstructed = true;
            if (kNetflixKeySystem == m_kKeySystem)
            {
                // Ocdm-playready netflix does not notify onProcessChallenge when complete.
                // Fetch the challenge manually.
                getChallenge();
            }
        }
        return status;
    }

    return MediaKeyErrorStatus::OK;
}

void MediaKeySession::getChallenge()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto task = [&]()
    {
        uint32_t challengeSize = 0;
        MediaKeyErrorStatus status = m_ocdmSession->getChallengeData(m_kIsLDL, nullptr, &challengeSize);
        std::vector<uint8_t> challenge(challengeSize, 0x00);
        status = m_ocdmSession->getChallengeData(m_kIsLDL, &challenge[0], &challengeSize);
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to get the challenge data, no onLicenseRequest will be generated");
            return;
        }

        std::string url;
        onProcessChallenge(url.c_str(), &challenge[0], challengeSize);
    };
    m_mainThread->enqueueTask(m_mainThreadClientId, task);
}

MediaKeyErrorStatus MediaKeySession::loadSession()
{
    MediaKeyErrorStatus status = m_ocdmSession->load();
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to load the key session");
    }
    return status;
}

MediaKeyErrorStatus MediaKeySession::updateSession(const std::vector<uint8_t> &responseData)
{
    MediaKeyErrorStatus status;
    if (kNetflixKeySystem == m_kKeySystem)
    {
        status = m_ocdmSession->storeLicenseData(&responseData[0], responseData.size());
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to store the license data for the key session");
        }
        return status;
    }
    else
    {
        status = m_ocdmSession->update(&responseData[0], responseData.size());
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to update the key session");
        }
        return status;
    }
}

MediaKeyErrorStatus MediaKeySession::decrypt(GstBuffer *encrypted, GstCaps *caps)
{
    MediaKeyErrorStatus status = MediaKeyErrorStatus::FAIL;

#ifdef RIALTO_ENABLE_DECRYPT_BUFFER
    status = m_ocdmSession->decrypt_buffer(encrypted, caps);
#else
    GstProtectionMeta* protectionMeta = reinterpret_cast<GstProtectionMeta*>(gstBufferGetProtectionMeta(buffer));
    if (protectionMeta)
    {
        uint32_t subsampleCount = 0;
        uint32_t initWithLast15 = 0;
        GstBuffer *key = nullptr;
        GstBuffer *iv = nullptr;
        GstBuffer *subsamples = nullptr;
        GstStructure* info = protectionMeta->info;

        if (extractDecryptionData(info, subSampleCount, initWithLast15, keyId, iv, subSample))
        {
            status = m_ocdmSession->decrypt(encrypted, subSample, subSampleCount, IV, keyId, initWithLast15, caps);
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to get the protection meta");
        }
    }
#endif
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to decrypt");
    }

    return status;
}

#ifndef RIALTO_ENABLE_DECRYPT_BUFFER
bool MediaKeySession::extractDecryptionData(GstStructure *protectionMetaInfo,
                                            uint32_t &subsampleCount, uint32_t &initWithLast15,
                                            GstBuffer **key, GstBuffer **iv, GstBuffer **subsamples)
{
    bool ret = false;
    const GValue *keyValue = nullptr;
    const GValue *ivValue = nullptr;
    const GValue *subsamplesValue = nullptr;

    if (!m_gstWrapper->gstStructureGetUint(protectionMetaInfo, "subsample_count", &subsampleCount))
    {
        GST_ERROR_OBJECT(self, "Failed to get subsamples_count");
    }
    else if (!m_gstWrapper->gstStructureGetUint(protectionMetaInfo, "init_with_last_15", &initWithLast15))
    {
        GST_ERROR_OBJECT(self, "Failed to get init_with_last_15");
    }
    else if (!(keyValue = m_gstWrapper->gstStructureGetValue(protectionMetaInfo, "kid")))
    {
        GST_ERROR_OBJECT(self, "Failed to get the key ID");
    }
    else if (!(ivValue = m_gstWrapper->gstStructureGetValue(protectionMetaInfo, "iv")))
    {
        GST_ERROR_OBJECT(self, "Failed to get IV buffer");
    }
    else if ((0u != subsampleCount) &&
             !(subsamplesValue = m_gstWrapper->gstStructureGetValue(protectionMetaInfo, "subsamples")))
    {
        GST_ERROR_OBJECT(self, "Failed to get subsamples buffer");
    }
    else if (!(*key = m_gstWrapper->gstValueGetBuffer(keyValue)))
    {
        GST_ERROR_OBJECT(self, "Failed to extract key from GValue");
    }
    else if (!(*iv = m_gstWrapper->gstValueGetBuffer(ivValue)))
    {
        GST_ERROR_OBJECT(self, "Failed to extract iv from GValue");
    }
    else if ((subsamplesValue) && !(*subsamples = m_gstWrapper->gstValueGetBuffer(subsamplesValue)))
    {
        GST_ERROR_OBJECT(self, "Failed to extract subsamples from GValue");
    }
    else
    {
        GST_TRACE_OBJECT(self, "Successfully extracted the decryption info");
        ret = GST_FLOW_OK;
    }

    return ret;
}
#endif

MediaKeyErrorStatus MediaKeySession::closeKeySession()
{
    MediaKeyErrorStatus status;
    if (kNetflixKeySystem == m_kKeySystem)
    {
        status = m_ocdmSession->cancelChallengeData();
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_WARN("Failed to cancel the challenge data for the key session");
        }

        status = m_ocdmSession->cleanDecryptContext();
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_WARN("Failed to clean the decrypt context for the key session");
        }
    }
    else
    {
        status = m_ocdmSession->close();
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to Close the key session");
            return status;
        }
    }

    status = m_ocdmSession->destructSession();
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to destruct the key session");
    }
    else
    {
        m_isSessionConstructed = false;
    }

    return status;
}

MediaKeyErrorStatus MediaKeySession::removeKeySession()
{
    MediaKeyErrorStatus status = m_ocdmSession->remove();
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to remove the key session");
        return status;
    }

    return status;
}

MediaKeyErrorStatus MediaKeySession::getCdmKeySessionId(std::string &cdmKeySessionId)
{
    MediaKeyErrorStatus status = m_ocdmSession->getCdmKeySessionId(cdmKeySessionId);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get cdm key session id");
        return status;
    }

    return status;
}

bool MediaKeySession::containsKey(const std::vector<uint8_t> &keyId)
{
    uint32_t result = m_ocdmSession->hasKeyId(keyId.data(), keyId.size());

    return static_cast<bool>(result);
}

MediaKeyErrorStatus MediaKeySession::setDrmHeader(const std::vector<uint8_t> &requestData)
{
    MediaKeyErrorStatus status = m_ocdmSession->setDrmHeader(requestData.data(), requestData.size());
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set drm header");
    }
    return status;
}

MediaKeyErrorStatus MediaKeySession::getLastDrmError(uint32_t &errorCode)
{
    MediaKeyErrorStatus status = m_ocdmSession->getLastDrmError(errorCode);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get last drm error");
    }
    return status;
}

MediaKeyErrorStatus MediaKeySession::selectKeyId(const std::vector<uint8_t> &keyId)
{
    if (m_selectedKeyId == keyId)
    {
        return MediaKeyErrorStatus::OK;
    }
    MediaKeyErrorStatus status = m_ocdmSession->selectKeyId(keyId.size(), keyId.data());
    if (MediaKeyErrorStatus::OK == status)
    {
        RIALTO_SERVER_LOG_INFO("New keyId selected successfully");
        m_selectedKeyId = keyId;
    }
    return status;
}

bool MediaKeySession::isNetflixKeySystem() const
{
    return (kNetflixKeySystem == m_kKeySystem);
}

void MediaKeySession::onProcessChallenge(const char url[], const uint8_t challenge[], const uint16_t challengeLength)
{
    std::string urlStr = url;
    std::vector<unsigned char> challengeVec = std::vector<unsigned char>{challenge, challenge + challengeLength};
    auto task = [&, urlStr, challengeVec]()
    {
        std::shared_ptr<IMediaKeysClient> client = m_mediaKeysClient.lock();
        if (client)
        {
            if (m_licenseRequested)
            {
                client->onLicenseRequest(m_kKeySessionId, challengeVec, urlStr);
                m_licenseRequested = false;
            }
            else
            {
                client->onLicenseRenewal(m_kKeySessionId, challengeVec);
            }
        }
    };
    m_mainThread->enqueueTask(m_mainThreadClientId, task);
}

void MediaKeySession::onKeyUpdated(const uint8_t keyId[], const uint8_t keyIdLength)
{
    std::vector<unsigned char> keyIdVec = std::vector<unsigned char>{keyId, keyId + keyIdLength};
    auto task = [&, keyIdVec]()
    {
        std::shared_ptr<IMediaKeysClient> client = m_mediaKeysClient.lock();
        if (client)
        {
            KeyStatus status = m_ocdmSession->getStatus(&keyIdVec[0], keyIdVec.size());
            m_updatedKeyStatuses.push_back(std::make_pair(keyIdVec, status));
        }
    };
    m_mainThread->enqueueTask(m_mainThreadClientId, task);
}

void MediaKeySession::onAllKeysUpdated()
{
    auto task = [&]()
    {
        std::shared_ptr<IMediaKeysClient> client = m_mediaKeysClient.lock();
        if (client)
        {
            client->onKeyStatusesChanged(m_kKeySessionId, m_updatedKeyStatuses);
            m_updatedKeyStatuses.clear();
        }
    };
    m_mainThread->enqueueTask(m_mainThreadClientId, task);
}

void MediaKeySession::onError(const char message[])
{
    RIALTO_SERVER_LOG_ERROR("Ocdm returned error: %s", message);
}
}; // namespace firebolt::rialto::server
