#include "rdk_perf.h"
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

#include "KeyIdMap.h"
#include "MediaKeys.h"
#include "RialtoClientLogging.h"

namespace
{
bool isNetflixPlayready(const std::string &keySystem)
{
    
    RDKPerf perf(__func__);
    return keySystem.find("netflix") != std::string::npos;
}
} // namespace

namespace firebolt::rialto
{
std::shared_ptr<IMediaKeysFactory> IMediaKeysFactory::createFactory()
{
    
    RDKPerf perf(__func__);
    std::shared_ptr<IMediaKeysFactory> factory;

    try
    {
        factory = std::make_shared<MediaKeysFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the media keys factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IMediaKeys> MediaKeysFactory::createMediaKeys(const std::string &keySystem) const
{
    
    RDKPerf perf(__func__);
    return createMediaKeys(keySystem, {});
}

std::unique_ptr<IMediaKeys>
MediaKeysFactory::createMediaKeys(const std::string &keySystem,
                                  std::weak_ptr<firebolt::rialto::client::IMediaKeysIpcFactory> mediaKeysIpcFactory) const
{
    
    RDKPerf perf(__func__);
    std::unique_ptr<IMediaKeys> mediaKeys;
    try
    {
        std::shared_ptr<firebolt::rialto::client::IMediaKeysIpcFactory> mediaKeysIpcFactoryLocked =
            mediaKeysIpcFactory.lock();
        mediaKeys = std::make_unique<client::MediaKeys>(keySystem, mediaKeysIpcFactoryLocked
                                                                       ? mediaKeysIpcFactoryLocked
                                                                       : client::IMediaKeysIpcFactory::createFactory());
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the media keys, reason: %s", e.what());
    }

    return mediaKeys;
}
}; // namespace firebolt::rialto

namespace firebolt::rialto::client
{
MediaKeys::MediaKeys(const std::string &keySystem, const std::shared_ptr<IMediaKeysIpcFactory> &mediaKeysIpcFactory)
    : m_keySystem{keySystem}
{
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    m_mediaKeysIpc = mediaKeysIpcFactory->createMediaKeysIpc(keySystem);
    if (!m_mediaKeysIpc)
    {
        throw std::runtime_error("Media keys ipc could not be created");
    }
}

MediaKeys::~MediaKeys()
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    m_mediaKeysIpc.reset();
}

MediaKeyErrorStatus MediaKeys::selectKeyId(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    if (KeyIdMap::instance().updateKey(keySessionId, keyId))
    {
        return MediaKeyErrorStatus::OK;
    }
    return MediaKeyErrorStatus::FAIL;
}

bool MediaKeys::containsKey(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->containsKey(keySessionId, keyId);
}

MediaKeyErrorStatus MediaKeys::createKeySession(KeySessionType sessionType, std::weak_ptr<IMediaKeysClient> client,
                                                int32_t &keySessionId)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    auto result{m_mediaKeysIpc->createKeySession(sessionType, client, keySessionId)};
    if (isNetflixPlayready(m_keySystem) && MediaKeyErrorStatus::OK == result)
    {
        KeyIdMap::instance().addSession(keySessionId);
    }
    return result;
}

MediaKeyErrorStatus MediaKeys::generateRequest(int32_t keySessionId, InitDataType initDataType,
                                               const std::vector<uint8_t> &initData,
                                               const LimitedDurationLicense &ldlState)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->generateRequest(keySessionId, initDataType, initData, ldlState);
}

MediaKeyErrorStatus MediaKeys::loadSession(int32_t keySessionId)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->loadSession(keySessionId);
}

MediaKeyErrorStatus MediaKeys::updateSession(int32_t keySessionId, const std::vector<uint8_t> &responseData)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->updateSession(keySessionId, responseData);
}

MediaKeyErrorStatus MediaKeys::setDrmHeader(int32_t keySessionId, const std::vector<uint8_t> &requestData)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->setDrmHeader(keySessionId, requestData);
}

MediaKeyErrorStatus MediaKeys::closeKeySession(int32_t keySessionId)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    if (isNetflixPlayready(m_keySystem))
    {
        KeyIdMap::instance().erase(keySessionId);
    }
    return m_mediaKeysIpc->closeKeySession(keySessionId);
}

MediaKeyErrorStatus MediaKeys::removeKeySession(int32_t keySessionId)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->removeKeySession(keySessionId);
}

MediaKeyErrorStatus MediaKeys::deleteDrmStore()
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->deleteDrmStore();
}

MediaKeyErrorStatus MediaKeys::deleteKeyStore()
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->deleteKeyStore();
}

MediaKeyErrorStatus MediaKeys::getDrmStoreHash(std::vector<unsigned char> &drmStoreHash)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->getDrmStoreHash(drmStoreHash);
}

MediaKeyErrorStatus MediaKeys::getKeyStoreHash(std::vector<unsigned char> &keyStoreHash)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->getKeyStoreHash(keyStoreHash);
}

MediaKeyErrorStatus MediaKeys::getLdlSessionsLimit(uint32_t &ldlLimit)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->getLdlSessionsLimit(ldlLimit);
}

MediaKeyErrorStatus MediaKeys::getLastDrmError(int32_t keySessionId, uint32_t &errorCode)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->getLastDrmError(keySessionId, errorCode);
}

MediaKeyErrorStatus MediaKeys::getDrmTime(uint64_t &drmTime)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->getDrmTime(drmTime);
}

MediaKeyErrorStatus MediaKeys::getCdmKeySessionId(int32_t keySessionId, std::string &cdmKeySessionId)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->getCdmKeySessionId(keySessionId, cdmKeySessionId);
}

MediaKeyErrorStatus MediaKeys::releaseKeySession(int32_t keySessionId)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    return m_mediaKeysIpc->releaseKeySession(keySessionId);
}

MediaKeyErrorStatus MediaKeys::getMetricSystemData(std::vector<uint8_t> &buffer)
{
    
    RDKPerf perf(__func__);
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaKeysIpc->getMetricSystemData(buffer);
}
}; // namespace firebolt::rialto::client
