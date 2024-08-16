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

#include "MediaPipelineService.h"
#include "IMediaPipelineServerInternal.h"
#include "RialtoServerLogging.h"
#include <exception>
#include <future>
#include <string>
#include <utility>
#include <vector>

namespace firebolt::rialto::server::service
{
MediaPipelineService::MediaPipelineService(
    IPlaybackService &playbackService, std::shared_ptr<IMediaPipelineServerInternalFactory> &&mediaPipelineFactory,
    std::shared_ptr<IMediaPipelineCapabilitiesFactory> &&mediaPipelineCapabilitiesFactory,
    IDecryptionService &decryptionService)
    : m_playbackService{playbackService}, m_mediaPipelineFactory{mediaPipelineFactory},
      m_mediaPipelineCapabilities{mediaPipelineCapabilitiesFactory->createMediaPipelineCapabilities()},
      m_decryptionService{decryptionService}
{
    if (!m_mediaPipelineCapabilities)
    {
        throw std::runtime_error("Could not create Media Pipeline Capabilities");
    }

    RIALTO_SERVER_LOG_DEBUG("MediaPipelineService is constructed");
}

MediaPipelineService::~MediaPipelineService()
{
    RIALTO_SERVER_LOG_DEBUG("MediaPipelineService is destructed");
}

void MediaPipelineService::clearMediaPipelines()
{
    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    m_mediaPipelines.clear();
}

bool MediaPipelineService::createSession(int sessionId, const std::shared_ptr<IMediaPipelineClient> &mediaPipelineClient,
                                         std::uint32_t maxWidth, std::uint32_t maxHeight)
{
    RIALTO_SERVER_LOG_DEBUG("MediaPipelineService requested to create new session with id: %d", sessionId);
    if (!m_playbackService.isActive())
    {
        RIALTO_SERVER_LOG_ERROR("Skip to create session with id: %d - Session Server in Inactive state", sessionId);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
        if (m_mediaPipelines.size() == static_cast<size_t>(m_playbackService.getMaxPlaybacks()))
        {
            RIALTO_SERVER_LOG_ERROR("Unable to create a session with id: %d. Max session number reached.", sessionId);
            return false;
        }
        if (m_mediaPipelines.find(sessionId) != m_mediaPipelines.end())
        {
            RIALTO_SERVER_LOG_ERROR("Session with id: %d already exists", sessionId);
            return false;
        }
        auto shmBuffer = m_playbackService.getShmBuffer();
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

bool MediaPipelineService::destroySession(int sessionId)
{
    RIALTO_SERVER_LOG_DEBUG("MediaPipelineService requested to destroy session with id: %d", sessionId);
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

bool MediaPipelineService::load(int sessionId, MediaType type, const std::string &mimeType, const std::string &url)
{
    RIALTO_SERVER_LOG_INFO("MediaPipelineService requested to load session with id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->load(type, mimeType, url);
}

bool MediaPipelineService::attachSource(int sessionId, const std::unique_ptr<IMediaPipeline::MediaSource> &source)
{
    RIALTO_SERVER_LOG_INFO("MediaPipelineService requested to attach source, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->attachSource(source);
}

bool MediaPipelineService::removeSource(int sessionId, std::int32_t sourceId)
{
    RIALTO_SERVER_LOG_INFO("MediaPipelineService requested to remove source, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->removeSource(sourceId);
}

bool MediaPipelineService::allSourcesAttached(int sessionId)
{
    RIALTO_SERVER_LOG_INFO("MediaPipelineService notified that all sources were attached, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->allSourcesAttached();
}

bool MediaPipelineService::play(int sessionId)
{
    RIALTO_SERVER_LOG_INFO("MediaPipelineService requested to play, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->play();
}

bool MediaPipelineService::pause(int sessionId)
{
    RIALTO_SERVER_LOG_INFO("MediaPipelineService requested to pause, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->pause();
}

bool MediaPipelineService::stop(int sessionId)
{
    RIALTO_SERVER_LOG_INFO("MediaPipelineService requested to stop, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->stop();
}

bool MediaPipelineService::setPlaybackRate(int sessionId, double rate)
{
    RIALTO_SERVER_LOG_INFO("MediaPipelineService requested to set playback rate, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->setPlaybackRate(rate);
}

bool MediaPipelineService::setPosition(int sessionId, std::int64_t position)
{
    RIALTO_SERVER_LOG_INFO("MediaPipelineService requested to set position, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->setPosition(position);
}

bool MediaPipelineService::getPosition(int sessionId, std::int64_t &position)
{
    RIALTO_SERVER_LOG_INFO("MediaPipelineService requested to get position, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->getPosition(position);
}

bool MediaPipelineService::setVideoWindow(int sessionId, std::uint32_t x, std::uint32_t y, std::uint32_t width,
                                          std::uint32_t height)
{
    RIALTO_SERVER_LOG_INFO("MediaPipelineService requested to set video window, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->setVideoWindow(x, y, width, height);
}

bool MediaPipelineService::haveData(int sessionId, MediaSourceStatus status, std::uint32_t numFrames,
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

bool MediaPipelineService::renderFrame(int sessionId)
{
    RIALTO_SERVER_LOG_DEBUG("Render frame requested, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->renderFrame();
}
bool MediaPipelineService::setVolume(int sessionId, double volume)
{
    RIALTO_SERVER_LOG_DEBUG("Set volume requested, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->setVolume(volume);
}

bool MediaPipelineService::getVolume(int sessionId, double &volume)
{
    RIALTO_SERVER_LOG_DEBUG("Get volume requested, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exists", sessionId);
        return false;
    }
    return mediaPipelineIter->second->getVolume(volume);
}

bool MediaPipelineService::setMute(int sessionId, std::int32_t sourceId, bool mute)
{
    RIALTO_SERVER_LOG_DEBUG("Set mute requested, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exist", sessionId);
        return false;
    }
    return mediaPipelineIter->second->setMute(sourceId, mute);
}

bool MediaPipelineService::getMute(int sessionId, std::int32_t sourceId, bool &mute)
{
    RIALTO_SERVER_LOG_DEBUG("Get mute requested, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exist", sessionId);
        return false;
    }
    return mediaPipelineIter->second->getMute(sourceId, mute);
}

bool MediaPipelineService::flush(int sessionId, std::int32_t sourceId, bool resetTime)
{
    RIALTO_SERVER_LOG_DEBUG("Flush requested, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exist", sessionId);
        return false;
    }
    return mediaPipelineIter->second->flush(sourceId, resetTime);
}

bool MediaPipelineService::setSourcePosition(int sessionId, int32_t sourceId, int64_t position)
{
    RIALTO_SERVER_LOG_DEBUG("Set Source Position requested, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exist", sessionId);
        return false;
    }
    return mediaPipelineIter->second->setSourcePosition(sourceId, position);
}

bool MediaPipelineService::processAudioGap(int sessionId, int64_t position, uint32_t duration, uint32_t level)
{
    RIALTO_SERVER_LOG_DEBUG("Process Audio Gap requested, session id: %d", sessionId);

    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    auto mediaPipelineIter = m_mediaPipelines.find(sessionId);
    if (mediaPipelineIter == m_mediaPipelines.end())
    {
        RIALTO_SERVER_LOG_ERROR("Session with id: %d does not exist", sessionId);
        return false;
    }
    return mediaPipelineIter->second->processAudioGap(position, duration, level);
}

std::vector<std::string> MediaPipelineService::getSupportedMimeTypes(MediaSourceType type)
{
    return m_mediaPipelineCapabilities->getSupportedMimeTypes(type);
}

bool MediaPipelineService::isMimeTypeSupported(const std::string &mimeType)
{
    return m_mediaPipelineCapabilities->isMimeTypeSupported(mimeType);
}

void MediaPipelineService::ping(const std::shared_ptr<IHeartbeatProcedure> &heartbeatProcedure)
{
    RIALTO_SERVER_LOG_DEBUG("Ping requested");
    std::lock_guard<std::mutex> lock{m_mediaPipelineMutex};
    for (const auto &mediaPipelinePair : m_mediaPipelines)
    {
        auto &mediaPipeline = mediaPipelinePair.second;
        mediaPipeline->ping(heartbeatProcedure->createHandler());
    }
}
} // namespace firebolt::rialto::server::service
