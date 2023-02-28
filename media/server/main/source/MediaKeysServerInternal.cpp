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

#include "MediaKeysServerInternal.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto
{
std::shared_ptr<IMediaKeysFactory> IMediaKeysFactory::createFactory()
{
    return server::IMediaKeysServerInternalFactory::createFactory();
}
} // namespace firebolt::rialto

namespace firebolt::rialto::server
{
int32_t generateSessionId()
{
    static int32_t keySessionId{0};
    return keySessionId++;
}

std::shared_ptr<IMediaKeysServerInternalFactory> IMediaKeysServerInternalFactory::createFactory()
{
    std::shared_ptr<IMediaKeysServerInternalFactory> factory;

    try
    {
        factory = std::make_shared<MediaKeysServerInternalFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media keys factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IMediaKeys> MediaKeysServerInternalFactory::createMediaKeys(const std::string &keySystem) const
{
    RIALTO_SERVER_LOG_ERROR("This function can't be used by rialto server. Please use createMediaKeysServerInternal");
    return nullptr;
}

std::unique_ptr<IMediaKeysServerInternal>
MediaKeysServerInternalFactory::createMediaKeysServerInternal(const std::string &keySystem) const
{
    std::unique_ptr<IMediaKeysServerInternal> mediaKeys;
    try
    {
        mediaKeys = std::make_unique<server::MediaKeysServerInternal>(keySystem,
                                                                      server::IMainThreadFactory::createFactory(),
                                                                      server::IOcdmSystemFactory::createFactory(),
                                                                      server::IMediaKeySessionFactory::createFactory());
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media keys, reason: %s", e.what());
    }

    return mediaKeys;
}
}; // namespace firebolt::rialto::server

namespace firebolt::rialto::server
{
MediaKeysServerInternal::MediaKeysServerInternal(const std::string &keySystem,
                                                 const std::shared_ptr<IMainThreadFactory> &mainThreadFactory,
                                                 std::shared_ptr<IOcdmSystemFactory> ocdmSystemFactory,
                                                 std::shared_ptr<IMediaKeySessionFactory> mediaKeySessionFactory)
    : m_mediaKeySessionFactory(mediaKeySessionFactory), m_keySystem(keySystem)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    m_mainThread = mainThreadFactory->getMainThread();
    if (!m_mainThread)
    {
        throw std::runtime_error("Failed to get the main thread");
    }
    m_mainThreadClientId = m_mainThread->registerClient();

    bool result = false;
    auto task = [&]()
    {
        m_ocdmSystem = ocdmSystemFactory->createOcdmSystem(keySystem);
        if (!m_ocdmSystem)
        {
            RIALTO_SERVER_LOG_ERROR("Ocdm system could not be created");
        }
        else
        {
            result = true;
        }
    };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    if (!result)
    {
        throw std::runtime_error("MediaKeys construction failed");
    }
}

MediaKeysServerInternal::~MediaKeysServerInternal()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&]()
    {
        m_ocdmSystem.reset();

        m_mainThread->unregisterClient(m_mainThreadClientId);
    };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
}

MediaKeyErrorStatus MediaKeysServerInternal::selectKeyId(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = selectKeyIdInternal(keySessionId, keyId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::selectKeyIdInternal(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second.mediaKeySession->selectKeyId(keyId);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to select key id");
        return status;
    }

    return status;
}

bool MediaKeysServerInternal::containsKey(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result{false};
    auto task = [&]() { result = containsKeyInternal(keySessionId, keyId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaKeysServerInternal::containsKeyInternal(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return false;
    }

    return sessionIter->second.mediaKeySession->containsKey(keyId);
}

MediaKeyErrorStatus MediaKeysServerInternal::createKeySession(KeySessionType sessionType,
                                                              std::weak_ptr<IMediaKeysClient> client, bool isLDL,
                                                              int32_t &keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = createKeySessionInternal(sessionType, client, isLDL, keySessionId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::createKeySessionInternal(KeySessionType sessionType,
                                                                      std::weak_ptr<IMediaKeysClient> client,
                                                                      bool isLDL, int32_t &keySessionId)
{
    int32_t keySessionIdTemp = generateSessionId();
    std::unique_ptr<IMediaKeySession> mediaKeySession =
        m_mediaKeySessionFactory->createMediaKeySession(m_keySystem, keySessionIdTemp, *m_ocdmSystem, sessionType,
                                                        client, isLDL);
    if (!mediaKeySession)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create a new media key session");
        return MediaKeyErrorStatus::FAIL;
    }
    keySessionId = keySessionIdTemp;
    m_mediaKeySessions.emplace(std::make_pair(keySessionId, MediaKeySessionUsage{std::move(mediaKeySession)}));

    return MediaKeyErrorStatus::OK;
}

MediaKeyErrorStatus MediaKeysServerInternal::generateRequest(int32_t keySessionId, InitDataType initDataType,
                                                             const std::vector<uint8_t> &initData)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = generateRequestInternal(keySessionId, initDataType, initData); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::generateRequestInternal(int32_t keySessionId, InitDataType initDataType,
                                                                     const std::vector<uint8_t> &initData)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second.mediaKeySession->generateRequest(initDataType, initData);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to generate request for the key session %d", keySessionId);
        return status;
    }
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::loadSession(int32_t keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = loadSessionInternal(keySessionId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::loadSessionInternal(int32_t keySessionId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second.mediaKeySession->loadSession();
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to load the session %d", keySessionId);
        return status;
    }
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::updateSession(int32_t keySessionId, const std::vector<uint8_t> &responseData)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = updateSessionInternal(keySessionId, responseData); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::updateSessionInternal(int32_t keySessionId,
                                                                   const std::vector<uint8_t> &responseData)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second.mediaKeySession->updateSession(responseData);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to update the session %d", keySessionId);
        return status;
    }
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::setDrmHeader(int32_t keySessionId, const std::vector<uint8_t> &requestData)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = setDrmHeaderInternal(keySessionId, requestData); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::setDrmHeaderInternal(int32_t keySessionId,
                                                                  const std::vector<uint8_t> &requestData)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second.mediaKeySession->setDrmHeader(requestData);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set drm header");
        return status;
    }

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::closeKeySession(int32_t keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = closeKeySessionInternal(keySessionId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::closeKeySessionInternal(int32_t keySessionId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    if (sessionIter->second.bufCounter == 0)
    {
        MediaKeyErrorStatus status = sessionIter->second.mediaKeySession->closeKeySession();
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to close the key session %d", keySessionId);
            return status;
        }
        m_mediaKeySessions.erase(sessionIter);
        return status;
    }

    RIALTO_SERVER_LOG_INFO("Deferring closing of key session %d", keySessionId);
    sessionIter->second.shouldBeDestroyed = true;
    return MediaKeyErrorStatus::OK;
}

MediaKeyErrorStatus MediaKeysServerInternal::removeKeySession(int32_t keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = removeKeySessionInternal(keySessionId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::removeKeySessionInternal(int32_t keySessionId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second.mediaKeySession->removeKeySession();
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to remove the key session %d", keySessionId);
        return status;
    }
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::deleteDrmStore()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = m_ocdmSystem->deleteSecureStore(); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::deleteKeyStore()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = m_ocdmSystem->deleteKeyStore(); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::getDrmStoreHash(std::vector<unsigned char> &drmStoreHash)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    constexpr size_t kHashSize{256};
    drmStoreHash.reserve(kHashSize);
    MediaKeyErrorStatus status;
    auto task = [&]() { status = m_ocdmSystem->getSecureStoreHash(&drmStoreHash[0], kHashSize); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::getKeyStoreHash(std::vector<unsigned char> &keyStoreHash)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    constexpr size_t kHashSize{256};
    keyStoreHash.reserve(kHashSize);
    MediaKeyErrorStatus status;
    auto task = [&]() { status = m_ocdmSystem->getKeyStoreHash(&keyStoreHash[0], kHashSize); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::getLdlSessionsLimit(uint32_t &ldlLimit)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = m_ocdmSystem->getLdlSessionsLimit(&ldlLimit); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::getLastDrmError(int32_t keySessionId, uint32_t &errorCode)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = getLastDrmErrorInternal(keySessionId, errorCode); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::getLastDrmErrorInternal(int32_t keySessionId, uint32_t &errorCode)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second.mediaKeySession->getLastDrmError(errorCode);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get last drm error");
    }

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::getDrmTime(uint64_t &drmTime)
{
    MediaKeyErrorStatus status;
    auto task = [&]() { status = m_ocdmSystem->getDrmTime(&drmTime); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::getCdmKeySessionId(int32_t keySessionId, std::string &cdmKeySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]() { status = getCdmKeySessionIdInternal(keySessionId, cdmKeySessionId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::getCdmKeySessionIdInternal(int32_t keySessionId, std::string &cdmKeySessionId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second.mediaKeySession->getCdmKeySessionId(cdmKeySessionId);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get cdm key session id");
        return status;
    }

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::decrypt(int32_t keySessionId, GstBuffer *encrypted, GstBuffer *subSample,
                                                     const uint32_t subSampleCount, GstBuffer *IV, GstBuffer *keyId,
                                                     uint32_t initWithLast15, GstCaps *caps)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    MediaKeyErrorStatus status;
    auto task = [&]()
    { status = decryptInternal(keySessionId, encrypted, subSample, subSampleCount, IV, keyId, initWithLast15, caps); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::decryptInternal(int32_t keySessionId, GstBuffer *encrypted,
                                                             GstBuffer *subSample, const uint32_t subSampleCount,
                                                             GstBuffer *IV, GstBuffer *keyId, uint32_t initWithLast15,
                                                             GstCaps *caps)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second.mediaKeySession->decrypt(encrypted, subSample, subSampleCount, IV,
                                                                              keyId, initWithLast15, caps);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to decrypt buffer.");
        return status;
    }

    return status;
}

bool MediaKeysServerInternal::hasSession(int32_t keySessionId) const
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = m_mediaKeySessions.find(keySessionId) != m_mediaKeySessions.end(); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaKeysServerInternal::isNetflixKeySystem(int32_t keySessionId) const
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    bool result;
    auto task = [&]() { result = isNetflixKeySystemInternal(keySessionId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaKeysServerInternal::isNetflixKeySystemInternal(int32_t keySessionId) const
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return false;
    }
    return sessionIter->second.mediaKeySession->isNetflixKeySystem();
}

void MediaKeysServerInternal::incrementSessionIdUsageCounter(int32_t keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto task = [&]() { incrementSessionIdUsageCounterInternal(keySessionId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
}

void MediaKeysServerInternal::incrementSessionIdUsageCounterInternal(int32_t keySessionId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return;
    }

    sessionIter->second.bufCounter++;
}

void MediaKeysServerInternal::decrementSessionIdUsageCounter(int32_t keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto task = [&]() { decrementSessionIdUsageCounterInternal(keySessionId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
}

void MediaKeysServerInternal::decrementSessionIdUsageCounterInternal(int32_t keySessionId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return;
    }

    if (sessionIter->second.bufCounter > 0)
    {
        sessionIter->second.bufCounter--;
    }

    if (sessionIter->second.bufCounter == 0 && sessionIter->second.shouldBeDestroyed)
    {
        RIALTO_SERVER_LOG_INFO("Deferred closing of mksId %d", keySessionId);
        MediaKeyErrorStatus status = sessionIter->second.mediaKeySession->closeKeySession();
        if (MediaKeyErrorStatus::OK != status)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to close the key session %d", keySessionId);
            return;
        }
        m_mediaKeySessions.erase(sessionIter);
    }
}
}; // namespace firebolt::rialto::server
