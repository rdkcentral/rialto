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
        mediaKeys =
            std::make_unique<server::MediaKeySession>(keySystem, keySessionId, ocdmSystem, sessionType, client, isLDL);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media key session, reason: %s", e.what());
    }

    return mediaKeys;
}

MediaKeySession::MediaKeySession(const std::string &keySystem, int32_t keySessionId, const IOcdmSystem &ocdmSystem,
                                 KeySessionType sessionType, std::weak_ptr<IMediaKeysClient> client, bool isLDL)
    : m_keySystem(keySystem), m_keySessionId(keySessionId), m_sessionType(sessionType), m_mediaKeysClient(client),
      m_isLDL(isLDL), m_isSessionConstructed(false), m_licenseRequested(false)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

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
}

MediaKeyErrorStatus MediaKeySession::generateRequest(InitDataType initDataType, const std::vector<uint8_t> &initData)
{
    if (!m_isSessionConstructed)
    {
        // Ocdm can send the challenge before constructSession completes, set the request flag
        if (kNetflixKeySystem != m_keySystem)
        {
            m_licenseRequested = true;
        }

        // Only construct session if it hasnt previously been constructed
        MediaKeyErrorStatus status =
            m_ocdmSession->constructSession(m_sessionType, initDataType, &initData[0], initData.size());
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to construct the key session");
            m_licenseRequested = false;
        }
        else
        {
            m_isSessionConstructed = true;
        }
        return status;
    }

    // TODO(LLDEV-31226): If the session had already been constructed or the key system is Netflix
    // the challenge has to be fetched manually after generateRequest.

    return MediaKeyErrorStatus::FAIL;
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
    if (kNetflixKeySystem == m_keySystem)
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

MediaKeyErrorStatus MediaKeySession::decrypt(GstBuffer *encrypted, GstBuffer *subSample, const uint32_t subSampleCount,
                                             GstBuffer *IV, GstBuffer *keyId, uint32_t initWithLast15)
{
    MediaKeyErrorStatus status = m_ocdmSession->decrypt(encrypted, subSample, subSampleCount, IV, keyId, initWithLast15);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to decrypt");
    }
    return status;
}

MediaKeyErrorStatus MediaKeySession::closeKeySession()
{
    MediaKeyErrorStatus status;
    if (kNetflixKeySystem == m_keySystem)
    {
        status = m_ocdmSession->cancelChallengeData();
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to cancel the challenge data for the key session");
            return status;
        }

        status = m_ocdmSession->cleanDecryptContext();
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to clean the decrypt context for the key session");
            return status;
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

void MediaKeySession::onProcessChallenge(const char url[], const uint8_t challenge[], const uint16_t challengeLength)
{
    // TODO(LLDEV-31226) Post onto main thread
    std::shared_ptr<IMediaKeysClient> client = m_mediaKeysClient.lock();
    if (client)
    {
        if (m_licenseRequested)
        {
            std::string urlStr = url;
            client->onLicenseRequest(m_keySessionId, std::vector<unsigned char>{challenge, challenge + challengeLength},
                                     urlStr);
            m_licenseRequested = false;
        }
        else
        {
            client->onLicenseRenewal(m_keySessionId, std::vector<unsigned char>{challenge, challenge + challengeLength});
        }
    }
}

void MediaKeySession::onKeyUpdated(const uint8_t keyId[], const uint8_t keyIdLength)
{
    // TODO(LLDEV-31226) Post onto main thread
    std::shared_ptr<IMediaKeysClient> client = m_mediaKeysClient.lock();
    if (client)
    {
        KeyStatus status = m_ocdmSession->getStatus(keyId, keyIdLength);
        m_updatedKeyStatuses.push_back(std::make_pair(std::vector<unsigned char>{keyId, keyId + keyIdLength}, status));
    }
}

void MediaKeySession::onAllKeysUpdated()
{
    // TODO(LLDEV-31226) Post onto main thread
    std::shared_ptr<IMediaKeysClient> client = m_mediaKeysClient.lock();
    if (client)
    {
        client->onKeyStatusesChanged(m_keySessionId, m_updatedKeyStatuses);
        m_updatedKeyStatuses.clear();
    }
}

void MediaKeySession::onError(const char message[])
{
    RIALTO_SERVER_LOG_ERROR("Ocdm returned error: %s", message);
}
}; // namespace firebolt::rialto::server
