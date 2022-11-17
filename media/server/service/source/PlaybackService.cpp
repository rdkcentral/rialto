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
PlaybackService::PlaybackService(std::shared_ptr<IMediaPipelineServerInternalFactory> &&mediaPipelineFactory,
                                 std::shared_ptr<IMediaPipelineCapabilitiesFactory> &&mediaPipelineCapabilitiesFactory,
                                 std::unique_ptr<ISharedMemoryBufferFactory> &&shmBufferFactory,
                                 IDecryptionService &decryptionService)
    : m_mediaPipelineFactory{mediaPipelineFactory},
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
    try
    {
        RIALTO_SERVER_LOG_INFO("Switching SessionServer to Active state.");
        m_shmBuffer = m_shmBufferFactory->createSharedMemoryBuffer(m_maxPlaybacks);
        m_isActive = true;
        return true;
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("SessionServer failed to switch to active: %s", e.what());
        m_isActive = false;
        return false;
    }
}

void PlaybackService::switchToInactive()
{
    RIALTO_SERVER_LOG_INFO("Switching SessionServer to Inactive state. Cleaning resources...");
    m_isActive = false;
    {
        std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
        m_mediaPipelines.clear();
    }
    m_shmBuffer.reset();
}

void PlaybackService::setMaxPlaybacks(int maxPlaybacks)
{
    // Method called during initialization only (before setting any state), no need to execute it on a task thread.
    m_maxPlaybacks = maxPlaybacks;
}

bool PlaybackService::createSession(int sessionId, const std::shared_ptr<IMediaPipelineClient> &mediaPipelineClient,
                                    std::uint32_t maxWidth, std::uint32_t maxHeight)
{
    RIALTO_SERVER_LOG_DEBUG("PlaybackService requested to create new session with id: %d", sessionId);
    if (!m_isActive)
    {
        RIALTO_SERVER_LOG_ERROR("Skip to create session with id: %d - Session Server in Inactive state", sessionId);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
        if (m_mediaPipelines.size() == static_cast<size_t>(m_maxPlaybacks))
        {
            RIALTO_SERVER_LOG_ERROR("Unable to create a session with id: %d. Max session number reached.", sessionId);
            return false;
        }
        if (m_mediaPipelines.find(sessionId) != m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d already exists", sessionId);
            return false;
        }
        auto shmBuffer = m_shmBuffer;
        m_mediaPipelines.emplace(
            std::make_pair(sessionId,
                           m_mediaPipelineFactory->createMediaPipelineServerInternal(mediaPipelineClient,
                                                                                     VideoRequirements{maxWidth, maxHeight},
                                                                                     sessionId, shmBuffer,
                                                                                     m_decryptionService)));
        if (!m_mediaPipelines.at(sessionId))
        {
            RIALTO_SERVER_LOG_ERROR("Could not create MediaPipeline for session with id: %d", sessionId);
            m_mediaPipelines.erase(sessionId);
            return false;
        }
    }

    RIALTO_SERVER_LOG_INFO("New session with id: %d created", sessionId);
    return true;
}

bool PlaybackService::destroySession(int sessionId)
{
    RIALTO_SERVER_LOG_DEBUG("PlaybackService requested to destroy session with id: %d", sessionId);
    {
        std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
        auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
        if (mediaPipelineIter == m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
            return false;
        }
        m_mediaPipelines.erase(mediaPipelineIter);
    }
    RIALTO_SERVER_LOG_INFO("Session with id: %d destroyed", sessionId);
    return true;
}

bool PlaybackService::load(int sessionId, MediaType type, const std::string &mimeType, const std::string &url)
{
    RIALTO_SERVER_LOG_INFO("PlaybackService requested to load session with id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->load(type, mimeType, url);
}

bool PlaybackService::attachSource(int sessionId, IMediaPipeline::MediaSource &source)
{
    RIALTO_SERVER_LOG_INFO("PlaybackService requested to attach source, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->attachSource(source);
}

bool PlaybackService::removeSource(int sessionId, std::int32_t sourceId)
{
    RIALTO_SERVER_LOG_INFO("PlaybackService requested to remove source, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->removeSource(sourceId);
}

bool PlaybackService::play(int sessionId)
{
    RIALTO_SERVER_LOG_INFO("PlaybackService requested to play, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->play();
}

bool PlaybackService::pause(int sessionId)
{
    RIALTO_SERVER_LOG_INFO("PlaybackService requested to pause, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->pause();
}

bool PlaybackService::stop(int sessionId)
{
    RIALTO_SERVER_LOG_INFO("PlaybackService requested to stop, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->stop();
}

bool PlaybackService::setPlaybackRate(int sessionId, double rate)
{
    RIALTO_SERVER_LOG_INFO("PlaybackService requested to set playback rate, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->setPlaybackRate(rate);
}

bool PlaybackService::setPosition(int sessionId, std::int64_t position)
{
    RIALTO_SERVER_LOG_INFO("PlaybackService requested to set position, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->setPosition(position);
}

bool PlaybackService::getPosition(int sessionId, std::int64_t &position)
{
    RIALTO_SERVER_LOG_INFO("PlaybackService requested to get position, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
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
    RIALTO_SERVER_LOG_INFO("PlaybackService requested to set video window, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->setVideoWindow(x, y, width, height);
}

bool PlaybackService::haveData(int sessionId, MediaSourceStatus status, std::uint32_t numFrames,
                               std::uint32_t needDataRequestId)
{
    RIALTO_SERVER_LOG_DEBUG("New data available, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->haveData(status, numFrames, needDataRequestId);
}

bool PlaybackService::getSharedMemory(int32_t &fd, uint32_t &size)
{
    auto shmBuffer = m_shmBuffer;

    if (!shmBuffer)
    {
        return false;
    }
    fd = shmBuffer->getFd();
    size = shmBuffer->getSize();
    return true;
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
