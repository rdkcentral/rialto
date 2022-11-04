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

#include "PlaybackService.h"
#include "IMediaPipelineServerInternal.h"
#include "RialtoServerLogging.h"
#include <exception>
#include <future>
#include <string>
#include <utility>
#include <vector>

namespace firebolt::rialto::server::service
{
PlaybackService::PlaybackService(IMainThread &mainThread,
                                 std::shared_ptr<IMediaPipelineServerInternalFactory> &&mediaPipelineFactory,
                                 std::shared_ptr<IMediaPipelineCapabilitiesFactory> &&mediaPipelineCapabilitiesFactory,
                                 std::unique_ptr<ISharedMemoryBufferFactory> &&shmBufferFactory,
                                 IDecryptionService &decryptionService)
    : m_mainThread{mainThread}, m_mediaPipelineFactory{mediaPipelineFactory},
      m_mediaPipelineCapabilities{mediaPipelineCapabilitiesFactory->createMediaPipelineCapabilities()},
      m_shmBufferFactory{std::move(shmBufferFactory)}, m_decryptionService{decryptionService}, m_isActive{false},
      m_maxPlaybacks{0}
{
    if (!m_mediaPipelineCapabilities)
    {
        throw std::runtime_error("Could not create Media Pipeline Capabilities");
    }

    RIALTO_SERVER_LOG_DEBUG("PlaybackService is constructed");
}

PlaybackService::~PlaybackService()
{
    RIALTO_SERVER_LOG_DEBUG("PlaybackService is destructed");
}

bool PlaybackService::switchToActive()
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [this, &promise]() {
        try
        {
            RIALTO_SERVER_LOG_INFO("Switching SessionServer to Active state.");
            m_shmBuffer = m_shmBufferFactory->createSharedMemoryBuffer(m_maxPlaybacks);
            m_isActive = true;
            return promise.set_value(true);
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("SessionServer failed to switch to active: %s", e.what());
            m_isActive = false;
            return promise.set_value(false);
        }
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

void PlaybackService::switchToInactive()
{
    auto task = [this]() {
        RIALTO_SERVER_LOG_INFO("Switching SessionServer to Inactive state. Cleaning resources...");
        m_isActive = false;
        m_mediaPipelines.clear();
        m_shmBuffer.reset();
    };
    m_mainThread.enqueueTask(task);
}

void PlaybackService::setMaxPlaybacks(int maxPlaybacks)
{
    // Method called during initialization only (before setting any state), no need to execute it on a task thread.
    m_maxPlaybacks = maxPlaybacks;
}

bool PlaybackService::createSession(int sessionId, const std::shared_ptr<IMediaPipelineClient> &mediaPipelineClient,
                                    std::uint32_t maxWidth, std::uint32_t maxHeight)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_DEBUG("PlaybackService requested to create new session with id: %d", sessionId);
        if (!m_isActive)
        {
            RIALTO_SERVER_LOG_ERROR("Skip to create session with id: %d - Session Server in Inactive state", sessionId);
            return promise.set_value(false);
        }
        if (m_mediaPipelines.size() == static_cast<size_t>(m_maxPlaybacks))
        {
            RIALTO_SERVER_LOG_ERROR("Unable to create a session with id: %d. Max session number reached.", sessionId);
            return promise.set_value(false);
        }
        if (m_mediaPipelines.find(sessionId) != m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d already exists", sessionId);
            return promise.set_value(false);
        }
        m_mediaPipelines.emplace(
            std::make_pair(sessionId,
                           m_mediaPipelineFactory->createMediaPipelineServerInternal(mediaPipelineClient,
                                                                                     VideoRequirements{maxWidth, maxHeight},
                                                                                     sessionId, m_shmBuffer,
                                                                                     m_decryptionService)));
        if (!m_mediaPipelines.at(sessionId))
        {
            RIALTO_SERVER_LOG_ERROR("Could not create MediaPipeline for session with id: %d", sessionId);
            m_mediaPipelines.erase(sessionId);
            return promise.set_value(false);
        }
        RIALTO_SERVER_LOG_INFO("New session with id: %d created", sessionId);
        return promise.set_value(true);
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::destroySession(int sessionId)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_DEBUG("PlaybackService requested to destroy session with id: %d", sessionId);
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return promise.set_value(false);
        }
        m_mediaPipelines.erase(mediaPipelineIter);
        RIALTO_SERVER_LOG_INFO("Session with id: %d destroyed", sessionId);
        return promise.set_value(true);
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::load(int sessionId, MediaType type, const std::string &mimeType, const std::string &url)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_INFO("PlaybackService requested to load session with id: %d", sessionId);
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return promise.set_value(false);
        }
        return promise.set_value(mediaPipelineIter->second->load(type, mimeType, url));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::attachSource(int sessionId, IMediaPipeline::MediaSource &source)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_INFO("PlaybackService requested to attach source, session id: %d", sessionId);
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return promise.set_value(false);
        }
        return promise.set_value(mediaPipelineIter->second->attachSource(source));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::removeSource(int sessionId, std::int32_t sourceId)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_INFO("PlaybackService requested to remove source, session id: %d", sessionId);
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return promise.set_value(false);
        }
        return promise.set_value(mediaPipelineIter->second->removeSource(sourceId));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::play(int sessionId)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_INFO("PlaybackService requested to play, session id: %d", sessionId);
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return promise.set_value(false);
        }
        return promise.set_value(mediaPipelineIter->second->play());
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::pause(int sessionId)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_INFO("PlaybackService requested to pause, session id: %d", sessionId);
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return promise.set_value(false);
        }
        return promise.set_value(mediaPipelineIter->second->pause());
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::stop(int sessionId)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_INFO("PlaybackService requested to stop, session id: %d", sessionId);
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return promise.set_value(false);
        }
        return promise.set_value(mediaPipelineIter->second->stop());
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::setPlaybackRate(int sessionId, double rate)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_INFO("PlaybackService requested to set playback rate, session id: %d", sessionId);
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return promise.set_value(false);
        }
        return promise.set_value(mediaPipelineIter->second->setPlaybackRate(rate));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::setPosition(int sessionId, std::int64_t position)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_INFO("PlaybackService requested to set position, session id: %d", sessionId);
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return promise.set_value(false);
        }
        return promise.set_value(mediaPipelineIter->second->setPosition(position));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::getPosition(int sessionId, std::int64_t &position)
{
    RIALTO_SERVER_LOG_INFO("PlaybackService requested to get position, session id: %d", sessionId);
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->getPosition(position);
}

bool PlaybackService::setVideoWindow(int sessionId, std::uint32_t x, std::uint32_t y, std::uint32_t width,
                                     std::uint32_t height)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_INFO("PlaybackService requested to set video window, session id: %d", sessionId);
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return promise.set_value(false);
        }
        return promise.set_value(mediaPipelineIter->second->setVideoWindow(x, y, width, height));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::haveData(int sessionId, MediaSourceStatus status, std::uint32_t numFrames,
                               std::uint32_t needDataRequestId)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        RIALTO_SERVER_LOG_DEBUG("New data available, session id: %d", sessionId);
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return promise.set_value(false);
        }
        return promise.set_value(mediaPipelineIter->second->haveData(status, numFrames, needDataRequestId));
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

bool PlaybackService::getSharedMemory(int32_t &fd, uint32_t &size)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    auto task = [&]() {
        if (!m_shmBuffer)
        {
            return promise.set_value(false);
        }
        fd = m_shmBuffer->getFd();
        size = m_shmBuffer->getSize();
        return promise.set_value(true);
    };
    m_mainThread.enqueueTask(task);
    return future.get();
}

std::vector<std::string> PlaybackService::getSupportedMimeTypes(MediaSourceType type)
{
    return m_mediaPipelineCapabilities->getSupportedMimeTypes(type);
}

bool PlaybackService::isMimeTypeSupported(const std::string &mimeType)
{
    return m_mediaPipelineCapabilities->isMimeTypeSupported(mimeType);
}
} // namespace firebolt::rialto::server::service
