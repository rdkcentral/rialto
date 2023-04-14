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

#include "MediaPipelineServerInternal.h"
#include "ActiveRequests.h"
#include "DataReaderFactory.h"
#include "IDataReader.h"
#include "IRdkGstreamerUtilsWrapper.h"
#include "ISharedMemoryBuffer.h"
#include "NeedMediaData.h"
#include "RialtoServerLogging.h"
#include <algorithm>

namespace
{
constexpr std::chrono::milliseconds kNeedMediaDataResendTimeMs{100};

const char *toString(const firebolt::rialto::MediaSourceStatus &status)
{
    switch (status)
    {
    case firebolt::rialto::MediaSourceStatus::OK:
        return "OK";
    case firebolt::rialto::MediaSourceStatus::EOS:
        return "EOS";
    case firebolt::rialto::MediaSourceStatus::ERROR:
        return "ERROR";
    case firebolt::rialto::MediaSourceStatus::CODEC_CHANGED:
        return "CODEC_CHANGED";
    case firebolt::rialto::MediaSourceStatus::NO_AVAILABLE_SAMPLES:
        return "NO_AVAILABLE_SAMPLES";
    }
    return "Unknown";
}

std::int32_t generateSourceId()
{
    static std::int32_t sourceId{1};
    return sourceId++;
}
} // namespace

namespace firebolt::rialto
{
std::shared_ptr<IMediaPipelineFactory> IMediaPipelineFactory::createFactory()
{
    return server::MediaPipelineServerInternalFactory::createFactory();
}
}; // namespace firebolt::rialto

namespace firebolt::rialto::server
{
std::shared_ptr<server::IMediaPipelineServerInternalFactory> IMediaPipelineServerInternalFactory::createFactory()
{
    return MediaPipelineServerInternalFactory::createFactory();
}

std::shared_ptr<MediaPipelineServerInternalFactory> MediaPipelineServerInternalFactory::createFactory()
{
    std::shared_ptr<MediaPipelineServerInternalFactory> factory;

    try
    {
        factory = std::make_shared<MediaPipelineServerInternalFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media player server internal factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IMediaPipeline>
MediaPipelineServerInternalFactory::createMediaPipeline(std::weak_ptr<IMediaPipelineClient> client,
                                                        const VideoRequirements &videoRequirements) const
{
    RIALTO_SERVER_LOG_ERROR(
        "This function can't be used by rialto server. Please use createMediaPipelineServerInternal");
    return nullptr;
}

std::unique_ptr<server::IMediaPipelineServerInternal> MediaPipelineServerInternalFactory::createMediaPipelineServerInternal(
    std::weak_ptr<IMediaPipelineClient> client, const VideoRequirements &videoRequirements, int sessionId,
    const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer, IDecryptionService &decryptionService) const
{
    std::shared_ptr<IMediaPipelineClient> sharedClient = client.lock();
    if (!sharedClient)
    {
        RIALTO_SERVER_LOG_ERROR("Couldn't create client's shared pointer");
        return nullptr;
    }

    std::unique_ptr<server::MediaPipelineServerInternal> mediaPipeline;
    try
    {
        mediaPipeline =
            std::make_unique<server::MediaPipelineServerInternal>(sharedClient, videoRequirements,
                                                                  server::IGstGenericPlayerFactory::getFactory(),
                                                                  sessionId, shmBuffer,
                                                                  server::IMainThreadFactory::createFactory(),
                                                                  common::ITimerFactory::getFactory(),
                                                                  std::make_unique<DataReaderFactory>(),
                                                                  std::make_unique<ActiveRequests>(), decryptionService);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media player server internal, reason: %s", e.what());
    }

    return mediaPipeline;
}

MediaPipelineServerInternal::MediaPipelineServerInternal(
    std::shared_ptr<IMediaPipelineClient> client, const VideoRequirements &videoRequirements,
    const std::shared_ptr<IGstGenericPlayerFactory> &gstPlayerFactory, int sessionId,
    const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer, const std::shared_ptr<IMainThreadFactory> &mainThreadFactory,
    std::shared_ptr<common::ITimerFactory> timerFactory, std::unique_ptr<IDataReaderFactory> &&dataReaderFactory,
    std::unique_ptr<IActiveRequests> &&activeRequests, IDecryptionService &decryptionService)
    : m_mediaPipelineClient(client), m_kGstPlayerFactory(gstPlayerFactory), m_kVideoRequirements(videoRequirements),
      m_sessionId{sessionId}, m_shmBuffer{shmBuffer}, m_dataReaderFactory{std::move(dataReaderFactory)},
      m_timerFactory{timerFactory}, m_activeRequests{std::move(activeRequests)}, m_decryptionService{decryptionService},
      m_currentPlaybackState{PlaybackState::UNKNOWN}, m_wasAllSourcesAttachedCalled{false}
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
        if (!m_shmBuffer->mapPartition(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_sessionId))
        {
            RIALTO_SERVER_LOG_ERROR("Unable to map shm partition");
        }
        else
        {
            result = true;
        }
    };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    if (!result)
    {
        throw std::runtime_error("MediaPipelineServerInternal construction failed");
    }
}

MediaPipelineServerInternal::~MediaPipelineServerInternal()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&]()
    {
        for (const auto &timer : m_needMediaDataTimers)
        {
            if (timer.second && timer.second->isActive())
            {
                timer.second->cancel();
            }
        }
        if (!m_shmBuffer->unmapPartition(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_sessionId))
        {
            RIALTO_SERVER_LOG_ERROR("Unable to unmap shm partition");
        }

        m_shmBuffer.reset();
        m_mainThread->unregisterClient(m_mainThreadClientId);
    };
    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
}

bool MediaPipelineServerInternal::load(MediaType type, const std::string &mimeType, const std::string &url)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = loadInternal(type, mimeType, url); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::loadInternal(MediaType type, const std::string &mimeType, const std::string &url)
{
    /* If gstreamer player already created, destroy the old one first */
    if (m_gstPlayer)
    {
        m_gstPlayer.reset();
    }

    m_gstPlayer = m_kGstPlayerFactory->createGstGenericPlayer(this, m_decryptionService, type, m_kVideoRequirements,
                                                              IRdkGstreamerUtilsWrapperFactory::getFactory());
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to load gstreamer player");
        return false;
    }

    notifyNetworkState(NetworkState::BUFFERING);

    return true;
}

bool MediaPipelineServerInternal::attachSource(const std::unique_ptr<MediaSource> &source)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = attachSourceInternal(source); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::attachSourceInternal(const std::unique_ptr<MediaSource> &source)
{
    source->setId(-1);

    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Gstreamer player has not been loaded");
        return false;
    }

    if (source->getType() == MediaSourceType::UNKNOWN)
    {
        RIALTO_SERVER_LOG_ERROR("Media source type unknown");
        return false;
    }

    m_gstPlayer->attachSource(source);

    const auto kSourceIter = m_attachedSources.find(source->getType());
    if (m_attachedSources.cend() == kSourceIter)
    {
        source->setId(generateSourceId());
        RIALTO_SERVER_LOG_DEBUG("New ID generated for MediaSourceType: %s: %d",
                                (MediaSourceType::AUDIO == source->getType() ? "AUDIO" : "VIDEO"), source->getId());
        m_attachedSources.emplace(source->getType(), source->getId());
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("SourceId: %d updated", kSourceIter->second);
        source->setId(kSourceIter->second);
    }

    return true;
}

bool MediaPipelineServerInternal::removeSource(int32_t id)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = removeSourceInternal(id); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::removeSourceInternal(int32_t id)
{
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to remove source - Gstreamer player has not been loaded");
        return false;
    }
    auto sourceIter = std::find_if(m_attachedSources.begin(), m_attachedSources.end(),
                                   [id](const auto &src) { return src.second == id; });
    if (sourceIter == m_attachedSources.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to remove source - Source not found");
        return false;
    }

    m_gstPlayer->removeSource(sourceIter->first);
    m_attachedSources.erase(sourceIter);
    return true;
}

bool MediaPipelineServerInternal::allSourcesAttached()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = allSourcesAttachedInternal(); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::allSourcesAttachedInternal()
{
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to notify all sources attached - Gstreamer player has not been loaded");
        return false;
    }

    if (m_wasAllSourcesAttachedCalled)
    {
        RIALTO_SERVER_LOG_WARN("Failed to notify all sources attached - It was already called");
        return false;
    }

    m_gstPlayer->allSourcesAttached();
    m_wasAllSourcesAttachedCalled = true;
    return true;
}

bool MediaPipelineServerInternal::play()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = playInternal(); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::playInternal()
{
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to play - Gstreamer player has not been loaded");
        return false;
    }

    m_gstPlayer->play();
    return true;
}

bool MediaPipelineServerInternal::pause()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = pauseInternal(); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::pauseInternal()
{
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to pause - Gstreamer player has not been loaded");
        return false;
    }

    m_gstPlayer->pause();
    return true;
}

bool MediaPipelineServerInternal::stop()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = stopInternal(); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::stopInternal()
{
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to stop - Gstreamer player has not been loaded");
        return false;
    }

    m_gstPlayer->stop();
    return true;
}

bool MediaPipelineServerInternal::setPlaybackRate(double rate)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = setPlaybackRateInternal(rate); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::setPlaybackRateInternal(double rate)
{
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set playback rate - Gstreamer player has not been loaded");
        return false;
    }

    if (0.0 == rate)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set playback rate to 0.0 - pause method should be used instead.");
        return false;
    }

    m_gstPlayer->setPlaybackRate(rate);
    return true;
}

bool MediaPipelineServerInternal::setPosition(int64_t position)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = setPositionInternal(position); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::setPositionInternal(int64_t position)
{
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set position - Gstreamer player has not been loaded");
        return false;
    }

    m_gstPlayer->setPosition(position);
    return true;
}

bool MediaPipelineServerInternal::getPosition(int64_t &position)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = getPositionInternal(position); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::getPositionInternal(int64_t &position)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get position - Gstreamer player has not been loaded");
        return false;
    }
    return m_gstPlayer->getPosition(position);
}

bool MediaPipelineServerInternal::setVideoWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = setVideoWindowInternal(x, y, width, height); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::setVideoWindowInternal(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set video window - Gstreamer player has not been loaded");
        return false;
    }

    m_gstPlayer->setVideoGeometry(x, y, width, height);
    return true;
}

bool MediaPipelineServerInternal::haveData(MediaSourceStatus status, uint32_t needDataRequestId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = haveDataInternal(status, needDataRequestId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::haveDataInternal(MediaSourceStatus status, uint32_t needDataRequestId)
{
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("HaveData failed - Gstreamer player has not been loaded");
        return false;
    }

    MediaSourceType mediaSourceType = m_activeRequests->getType(needDataRequestId);
    if (MediaSourceType::UNKNOWN == mediaSourceType)
    {
        RIALTO_SERVER_LOG_WARN("NeedData RequestID is not valid: %u", needDataRequestId);
        return true;
    }

    if (status != MediaSourceStatus::OK && status != MediaSourceStatus::EOS)
    {
        RIALTO_SERVER_LOG_WARN("Data request for needDataRequestId: %u received with wrong status: %s",
                               needDataRequestId, toString(status));
        m_activeRequests->erase(needDataRequestId);
        scheduleNotifyNeedMediaData(mediaSourceType);
        return true;
    }

    try
    {
        const IMediaPipeline::MediaSegmentVector &segments = m_activeRequests->getSegments(needDataRequestId);
        m_gstPlayer->attachSamples(segments);
    }
    catch (const std::runtime_error &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get segments %s", e.what());
        m_activeRequests->erase(needDataRequestId);
        return false;
    }

    m_activeRequests->erase(needDataRequestId);
    if (status == MediaSourceStatus::EOS)
    {
        m_gstPlayer->setEos(mediaSourceType);
    }

    return true;
}

bool MediaPipelineServerInternal::haveData(MediaSourceStatus status, uint32_t numFrames, uint32_t needDataRequestId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = haveDataInternal(status, numFrames, needDataRequestId); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::haveDataInternal(MediaSourceStatus status, uint32_t numFrames,
                                                   uint32_t needDataRequestId)
{
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("HaveData failed - Gstreamer player has not been loaded");
        return false;
    }
    MediaSourceType mediaSourceType = m_activeRequests->getType(needDataRequestId);
    if (MediaSourceType::UNKNOWN == mediaSourceType)
    {
        RIALTO_SERVER_LOG_WARN("NeedData RequestID is not valid: %u", needDataRequestId);
        return true;
    }
    m_activeRequests->erase(needDataRequestId);
    if (status != MediaSourceStatus::OK && status != MediaSourceStatus::EOS)
    {
        RIALTO_SERVER_LOG_WARN("Data request for needDataRequestId: %u received with wrong status", needDataRequestId);
        scheduleNotifyNeedMediaData(mediaSourceType);
        return true;
    }
    uint8_t *buffer = m_shmBuffer->getBuffer();
    if (!buffer)
    {
        RIALTO_SERVER_LOG_ERROR("No buffer available");
        notifyPlaybackState(PlaybackState::FAILURE);
        return false;
    }

    std::uint32_t regionOffset = 0;
    try
    {
        regionOffset =
            m_shmBuffer->getDataOffset(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_sessionId, mediaSourceType);
    }
    catch (const std::runtime_error &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get region's buffer offset, reason: %s", e.what());
        notifyPlaybackState(PlaybackState::FAILURE);
        return false;
    }

    if (0 != numFrames)
    {
        std::shared_ptr<IDataReader> dataReader =
            m_dataReaderFactory->createDataReader(mediaSourceType, buffer, regionOffset, numFrames);
        if (!dataReader)
        {
            RIALTO_SERVER_LOG_ERROR("Metadata version not supported for request id: %u", needDataRequestId);
            notifyPlaybackState(PlaybackState::FAILURE);
            return false;
        }
        m_gstPlayer->attachSamples(dataReader);
    }
    if (status == MediaSourceStatus::EOS)
    {
        m_gstPlayer->setEos(mediaSourceType);
    }

    return true;
}

bool MediaPipelineServerInternal::renderFrame()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = renderFrameInternal(); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::renderFrameInternal()
{
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("renderFrame failed - Gstreamer player has not been loaded");
        return false;
    }

    m_gstPlayer->renderFrame();
    return true;
}

bool MediaPipelineServerInternal::setVolume(double volume)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = setVolumeInternal(volume); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::setVolumeInternal(double volume)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set volume - Gstreamer player has not been loaded");
        return false;
    }
    m_gstPlayer->setVolume(volume);
    return true;
}

bool MediaPipelineServerInternal::getVolume(double &volume)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = getVolumeInternal(volume); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::getVolumeInternal(double &volume)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get volume - Gstreamer player has not been loaded");
        return false;
    }
    return m_gstPlayer->getVolume(volume);
}

AddSegmentStatus MediaPipelineServerInternal::addSegment(uint32_t needDataRequestId,
                                                         const std::unique_ptr<MediaSegment> &mediaSegment)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    AddSegmentStatus status;
    auto task = [&]() { status = addSegmentInternal(needDataRequestId, mediaSegment); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return status;
}

AddSegmentStatus MediaPipelineServerInternal::addSegmentInternal(uint32_t needDataRequestId,
                                                                 const std::unique_ptr<MediaSegment> &mediaSegment)
{
    AddSegmentStatus status = m_activeRequests->addSegment(needDataRequestId, mediaSegment);
    if (status != AddSegmentStatus::OK)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to add segment for request id: %u", needDataRequestId);
    }

    return status;
}

std::weak_ptr<IMediaPipelineClient> MediaPipelineServerInternal::getClient()
{
    return m_mediaPipelineClient;
}

void MediaPipelineServerInternal::notifyPlaybackState(PlaybackState state)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&, state]()
    {
        m_currentPlaybackState = state;
        if (m_mediaPipelineClient)
        {
            m_mediaPipelineClient->notifyPlaybackState(state);
        }
    };

    m_mainThread->enqueueTask(m_mainThreadClientId, task);
}

bool MediaPipelineServerInternal::notifyNeedMediaData(MediaSourceType mediaSourceType)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result;
    auto task = [&]() { result = notifyNeedMediaDataInternal(mediaSourceType); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    return result;
}

bool MediaPipelineServerInternal::notifyNeedMediaDataInternal(MediaSourceType mediaSourceType)
{
    m_needMediaDataTimers.erase(mediaSourceType);
    m_shmBuffer->clearData(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_sessionId, mediaSourceType);
    const auto kSourceIter = m_attachedSources.find(mediaSourceType);
    if (m_attachedSources.cend() == kSourceIter)
    {
        RIALTO_SERVER_LOG_WARN("NeedMediaData event sending failed - sourceId not found");
        return false;
    }
    NeedMediaData event{m_mediaPipelineClient, *m_activeRequests,   *m_shmBuffer,          m_sessionId,
                        mediaSourceType,       kSourceIter->second, m_currentPlaybackState};
    if (!event.send())
    {
        RIALTO_SERVER_LOG_WARN("NeedMediaData event sending failed");
        return false;
    }
    return true;
}

void MediaPipelineServerInternal::notifyPosition(std::int64_t position)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&, position]()
    {
        if (m_mediaPipelineClient)
        {
            m_mediaPipelineClient->notifyPosition(position);
        }
    };

    m_mainThread->enqueueTask(m_mainThreadClientId, task);
}

void MediaPipelineServerInternal::notifyNetworkState(NetworkState state)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&, state]()
    {
        if (m_mediaPipelineClient)
        {
            m_mediaPipelineClient->notifyNetworkState(state);
        }
    };

    m_mainThread->enqueueTask(m_mainThreadClientId, task);
}

void MediaPipelineServerInternal::clearActiveRequestsCache()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&]() { m_activeRequests->clear(); };

    m_mainThread->enqueueTask(m_mainThreadClientId, task);
}

void MediaPipelineServerInternal::invalidateActiveRequests(const MediaSourceType &type)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&, type]() { m_activeRequests->erase(type); };

    m_mainThread->enqueueTask(m_mainThreadClientId, task);
}

void MediaPipelineServerInternal::notifyQos(MediaSourceType mediaSourceType, const QosInfo &qosInfo)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&, mediaSourceType, qosInfo]()
    {
        if (m_mediaPipelineClient)
        {
            const auto kSourceIter = m_attachedSources.find(mediaSourceType);
            if (m_attachedSources.cend() == kSourceIter)
            {
                RIALTO_SERVER_LOG_WARN("Qos notification failed - sourceId not found");
                return;
            }
            m_mediaPipelineClient->notifyQos(kSourceIter->second, qosInfo);
        }
    };

    m_mainThread->enqueueTask(m_mainThreadClientId, task);
}

void MediaPipelineServerInternal::scheduleNotifyNeedMediaData(MediaSourceType mediaSourceType)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto timer = m_needMediaDataTimers.find(mediaSourceType);
    if (m_needMediaDataTimers.end() != timer && timer->second && timer->second->isActive())
    {
        RIALTO_SERVER_LOG_DEBUG("Skip scheduling need media data - it is already scheduled");
        return;
    }
    m_needMediaDataTimers[mediaSourceType] =
        m_timerFactory->createTimer(kNeedMediaDataResendTimeMs,
                                    [this, mediaSourceType]()
                                    {
                                        m_mainThread->enqueueTask(m_mainThreadClientId,
                                                                  [this, mediaSourceType]()
                                                                  {
                                                                      m_needMediaDataTimers.erase(mediaSourceType);
                                                                      if (!notifyNeedMediaDataInternal(mediaSourceType))
                                                                      {
                                                                          RIALTO_SERVER_LOG_WARN(
                                                                              "Scheduled Need media data sending "
                                                                              "failed. Scheduling again...");
                                                                          scheduleNotifyNeedMediaData(mediaSourceType);
                                                                      }
                                                                  });
                                    });
}
}; // namespace firebolt::rialto::server
