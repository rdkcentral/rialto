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

#include "CdmService.h"
#include "RialtoServerLogging.h"
#include <algorithm>
#include <exception>
#include <future>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace firebolt::rialto::server::service
{
CdmService::CdmService(IMainThread &mainThread, std::shared_ptr<IMediaKeysServerInternalFactory> &&mediaKeysFactory,
                       std::shared_ptr<IMediaKeysCapabilitiesFactory> &&mediaKeysCapabilitiesFactory)
    : m_mainThread{mainThread}, m_mediaKeysFactory{mediaKeysFactory},
      m_mediaKeysCapabilitiesFactory{mediaKeysCapabilitiesFactory}, m_isActive{false}
{
    RIALTO_SERVER_LOG_DEBUG("CdmService is constructed");
}

CdmService::~CdmService()
{
    RIALTO_SERVER_LOG_DEBUG("CdmService is destructed");
}

bool CdmService::switchToActive()
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [this, &promise]()
    {
        RIALTO_SERVER_LOG_INFO("Switching SessionServer to Active state.");

        m_mediaKeysCapabilities = m_mediaKeysCapabilitiesFactory->getMediaKeysCapabilities();
        if (!m_mediaKeysCapabilities)
        {
            RIALTO_SERVER_LOG_ERROR("SessionServer failed to switch to active");
            return promise.set_value(false);
        }
        m_isActive = true;
        return promise.set_value(true);
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

void CdmService::switchToInactive()
{
    auto task = [this]()
    {
        RIALTO_SERVER_LOG_INFO("Switching SessionServer to Inactive state. Cleaning resources...");
        m_isActive = false;
        m_mediaKeysCapabilities.reset();
        m_mediaKeys.clear();
    };
    m_mainThread.enqueueTask(task);
}

bool CdmService::createMediaKeys(int mediaKeysHandle, std::string keySystem)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to create new media keys handle: %d", mediaKeysHandle);
        if (!m_isActive)
        {
            RIALTO_SERVER_LOG_ERROR("Skip to create media keys handle: %d - Session Server in Inactive state",
                                    mediaKeysHandle);
            return promise.set_value(false);
        }
        if (m_mediaKeys.find(mediaKeysHandle) != m_mediaKeys.end())
        {
            RIALTO_SERVER_LOG_ERROR("Media keys handle: %d already exists", mediaKeysHandle);
            return promise.set_value(false);
        }
        m_mediaKeys.emplace(std::make_pair(mediaKeysHandle, m_mediaKeysFactory->createMediaKeysServerInternal(keySystem)));
        if (!m_mediaKeys.at(mediaKeysHandle))
        {
            RIALTO_SERVER_LOG_ERROR("Could not create MediaKeys for media keys handle: %d", mediaKeysHandle);
            m_mediaKeys.erase(mediaKeysHandle);
            return promise.set_value(false);
        }
        RIALTO_SERVER_LOG_INFO("New media keys handle: %d created", mediaKeysHandle);
        return promise.set_value(true);
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool CdmService::destroyMediaKeys(int mediaKeysHandle)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to destroy media keys handle: %d", mediaKeysHandle);
        auto mediaKeysIter = m_mediaKeys.find(mediaKeysHandle);
        if (mediaKeysIter == m_mediaKeys.end())
        {
            RIALTO_SERVER_LOG_ERROR("Media keys handle: %d does not exists", mediaKeysHandle);
            return promise.set_value(false);
        }
        m_mediaKeys.erase(mediaKeysIter);
        RIALTO_SERVER_LOG_INFO("Media keys handle: %d destroyed", mediaKeysHandle);
        return promise.set_value(true);
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

MediaKeyErrorStatus CdmService::createKeySession(int mediaKeysHandle, KeySessionType sessionType,
                                                 const std::shared_ptr<IMediaKeysClient> &client, bool isLDL,
                                                 int32_t &keySessionId)
{
    std::promise<MediaKeyErrorStatus> promise;
    std::future<MediaKeyErrorStatus> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to create key session: %d", mediaKeysHandle);
        auto mediaKeysIter = m_mediaKeys.find(mediaKeysHandle);
        if (mediaKeysIter == m_mediaKeys.end())
        {
            RIALTO_SERVER_LOG_ERROR("Media keys handle: %d does not exists", mediaKeysHandle);
            return promise.set_value(MediaKeyErrorStatus::FAIL);
        }

        MediaKeyErrorStatus status = mediaKeysIter->second->createKeySession(sessionType, client, isLDL, keySessionId);
        if (MediaKeyErrorStatus::OK == status)
        {
            if (m_mediaKeysClients.find(keySessionId) != m_mediaKeysClients.end())
            {
                RIALTO_SERVER_LOG_ERROR("Media keys client for key session: %d already exists", keySessionId);
                static_cast<void>(removeKeySession(mediaKeysHandle, keySessionId));
                return promise.set_value(MediaKeyErrorStatus::FAIL);
            }
            m_mediaKeysClients.emplace(std::make_pair(keySessionId, client));
        }
        return promise.set_value(status);
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

MediaKeyErrorStatus CdmService::generateRequest(int mediaKeysHandle, int32_t keySessionId, InitDataType initDataType,
                                                const std::vector<uint8_t> &initData)
{
    std::promise<MediaKeyErrorStatus> promise;
    std::future<MediaKeyErrorStatus> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to generate request: %d", mediaKeysHandle);
        auto mediaKeysIter = m_mediaKeys.find(mediaKeysHandle);
        if (mediaKeysIter == m_mediaKeys.end())
        {
            RIALTO_SERVER_LOG_ERROR("Media keys handle: %d does not exists", mediaKeysHandle);
            return promise.set_value(MediaKeyErrorStatus::FAIL);
        }
        return promise.set_value(mediaKeysIter->second->generateRequest(keySessionId, initDataType, initData));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

MediaKeyErrorStatus CdmService::loadSession(int mediaKeysHandle, int32_t keySessionId)
{
    std::promise<MediaKeyErrorStatus> promise;
    std::future<MediaKeyErrorStatus> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to load session: %d", mediaKeysHandle);
        auto mediaKeysIter = m_mediaKeys.find(mediaKeysHandle);
        if (mediaKeysIter == m_mediaKeys.end())
        {
            RIALTO_SERVER_LOG_ERROR("Media keys handle: %d does not exists", mediaKeysHandle);
            return promise.set_value(MediaKeyErrorStatus::FAIL);
        }
        return promise.set_value(mediaKeysIter->second->loadSession(keySessionId));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

MediaKeyErrorStatus CdmService::updateSession(int mediaKeysHandle, int32_t keySessionId,
                                              const std::vector<uint8_t> &responseData)
{
    std::promise<MediaKeyErrorStatus> promise;
    std::future<MediaKeyErrorStatus> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to update session: %d", mediaKeysHandle);
        auto mediaKeysIter = m_mediaKeys.find(mediaKeysHandle);
        if (mediaKeysIter == m_mediaKeys.end())
        {
            RIALTO_SERVER_LOG_ERROR("Media keys handle: %d does not exists", mediaKeysHandle);
            return promise.set_value(MediaKeyErrorStatus::FAIL);
        }
        return promise.set_value(mediaKeysIter->second->updateSession(keySessionId, responseData));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

MediaKeyErrorStatus CdmService::closeKeySession(int mediaKeysHandle, int32_t keySessionId)
{
    std::promise<MediaKeyErrorStatus> promise;
    std::future<MediaKeyErrorStatus> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to close key session: %d", mediaKeysHandle);
        auto mediaKeysIter = m_mediaKeys.find(mediaKeysHandle);
        if (mediaKeysIter == m_mediaKeys.end())
        {
            RIALTO_SERVER_LOG_ERROR("Media keys handle: %d does not exists", mediaKeysHandle);
            return promise.set_value(MediaKeyErrorStatus::FAIL);
        }
        return promise.set_value(mediaKeysIter->second->closeKeySession(keySessionId));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

MediaKeyErrorStatus CdmService::removeKeySession(int mediaKeysHandle, int32_t keySessionId)
{
    std::promise<MediaKeyErrorStatus> promise;
    std::future<MediaKeyErrorStatus> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to remove key session: %d", mediaKeysHandle);
        auto mediaKeysIter = m_mediaKeys.find(mediaKeysHandle);
        if (mediaKeysIter == m_mediaKeys.end())
        {
            RIALTO_SERVER_LOG_ERROR("Media keys handle: %d does not exists", mediaKeysHandle);
            return promise.set_value(MediaKeyErrorStatus::FAIL);
        }

        MediaKeyErrorStatus status = mediaKeysIter->second->removeKeySession(keySessionId);
        if (MediaKeyErrorStatus::OK == status)
        {
            auto mediaKeysClientsIter = m_mediaKeysClients.find(keySessionId);
            if (mediaKeysClientsIter != m_mediaKeysClients.end())
            {
                m_mediaKeysClients.erase(mediaKeysClientsIter);
            }
        }
        return promise.set_value(status);
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

MediaKeyErrorStatus CdmService::getCdmKeySessionId(int mediaKeysHandle, int32_t keySessionId, std::string &cdmKeySessionId)
{
    std::promise<MediaKeyErrorStatus> promise;
    std::future<MediaKeyErrorStatus> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to get cdm key session id: %d", mediaKeysHandle);
        auto mediaKeysIter = m_mediaKeys.find(mediaKeysHandle);
        if (mediaKeysIter == m_mediaKeys.end())
        {
            RIALTO_SERVER_LOG_ERROR("Media keys handle: %d does not exists", mediaKeysHandle);
            return promise.set_value(MediaKeyErrorStatus::FAIL);
        }

        MediaKeyErrorStatus status = mediaKeysIter->second->getCdmKeySessionId(keySessionId, cdmKeySessionId);
        return promise.set_value(status);
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

std::vector<std::string> CdmService::getSupportedKeySystems()
{
    std::promise<std::vector<std::string>> promise;
    std::future<std::vector<std::string>> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to getSupportedKeySystems");
        if (!m_mediaKeysCapabilities)
        {
            return promise.set_value({});
        }
        return promise.set_value(m_mediaKeysCapabilities->getSupportedKeySystems());
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool CdmService::supportsKeySystem(const std::string &keySystem)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to supportsKeySystem");
        if (!m_mediaKeysCapabilities)
        {
            return promise.set_value(false);
        }
        return promise.set_value(m_mediaKeysCapabilities->supportsKeySystem(keySystem));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool CdmService::getSupportedKeySystemVersion(const std::string &keySystem, std::string &version)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to getSupportedKeySystemVersion");
        if (!m_mediaKeysCapabilities)
        {
            return promise.set_value(false);
        }
        return promise.set_value(m_mediaKeysCapabilities->getSupportedKeySystemVersion(keySystem, version));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

MediaKeyErrorStatus CdmService::decrypt(int32_t keySessionId, GstBuffer *encrypted, GstBuffer *subSample,
                                        const uint32_t subSampleCount, GstBuffer *IV, GstBuffer *keyId,
                                        uint32_t initWithLast15)
{
    std::promise<MediaKeyErrorStatus> promise;
    std::future<MediaKeyErrorStatus> future = promise.get_future();
    auto task = [&]()
    {
        RIALTO_SERVER_LOG_DEBUG("CdmService requested to decrypt, key session id: %d", keySessionId);
        auto mediaKeysIter = std::find_if(m_mediaKeys.begin(), m_mediaKeys.end(),
                                          [&](const auto &iter) { return iter.second->hasSession(keySessionId); });
        if (mediaKeysIter == m_mediaKeys.end())
        {
            RIALTO_SERVER_LOG_ERROR("Media keys handle for mksId: %d does not exists", keySessionId);
            return promise.set_value(MediaKeyErrorStatus::FAIL);
        }
        return promise.set_value(mediaKeysIter->second->decrypt(keySessionId, encrypted, subSample, subSampleCount, IV,
                                                                keyId, initWithLast15));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}
} // namespace firebolt::rialto::server::service
