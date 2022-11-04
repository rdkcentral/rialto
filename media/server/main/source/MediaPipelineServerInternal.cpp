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
#include "ISharedMemoryBuffer.h"
#include "NeedMediaData.h"
#include "RialtoServerLogging.h"
#include <iostream>

namespace
{
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
                                                                  server::IGstPlayerFactory::getFactory(), sessionId,
                                                                  shmBuffer, std::make_unique<DataReaderFactory>(),
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
    const std::shared_ptr<IGstPlayerFactory> &gstPlayerFactory, int sessionId,
    const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer, std::unique_ptr<IDataReaderFactory> &&dataReaderFactory,
    std::unique_ptr<IActiveRequests> &&activeRequests, IDecryptionService &decryptionService)
    : m_mediaPipelineClient(client), m_kGstPlayerFactory(gstPlayerFactory), m_kVideoRequirements(videoRequirements),
      m_sessionId{sessionId}, m_shmBuffer{shmBuffer}, m_dataReaderFactory{std::move(dataReaderFactory)},
      m_activeRequests{std::move(activeRequests)}, m_decryptionService{decryptionService}
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    if (!m_shmBuffer->mapPartition(m_sessionId))
    {
        RIALTO_SERVER_LOG_ERROR("Unable to create a session with id: %d. Unable to map shm partition.", m_sessionId);
        throw std::runtime_error("Unable to map partition");
    }
}

MediaPipelineServerInternal::~MediaPipelineServerInternal()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    m_shmBuffer->unmapPartition(m_sessionId);
}

bool MediaPipelineServerInternal::load(MediaType type, const std::string &mimeType, const std::string &url)
{
    /* If gstreamer player already created, destroy the old one first */
    if (m_gstPlayer)
    {
        m_gstPlayer.reset();
    }

    m_gstPlayer = m_kGstPlayerFactory->createGstPlayer(this, m_decryptionService, type);
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to load gstreamer player");
        return false;
    }

    notifyNetworkState(NetworkState::BUFFERING);

    return true;
}

bool MediaPipelineServerInternal::attachSource(MediaSource &source)
{
    source.setId(-1);

    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Gstreamer player has not been loaded");
        return false;
    }

    if (source.getType() == MediaSourceType::UNKNOWN)
    {
        RIALTO_SERVER_LOG_ERROR("Media source type unknown");
        return false;
    }

    m_gstPlayer->attachSource(source.getType(), source.getCaps());
    source.setId(static_cast<int32_t>(source.getType()));

    return true;
}

bool MediaPipelineServerInternal::removeSource(int32_t id)
{
    RIALTO_SERVER_LOG_ERROR("Can't remove source with id: %d - operation not supported", id);
    return false;
}

bool MediaPipelineServerInternal::play()
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
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get position - Gstreamer player has not been loaded");
        return false;
    }
    return m_gstPlayer->getPosition(position);
}

bool MediaPipelineServerInternal::setVideoWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
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
        return notifyNeedMediaData(mediaSourceType); // Resend NeedMediaData
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
        return notifyNeedMediaData(mediaSourceType); // Resend NeedMediaData
    }
    uint8_t *data = m_shmBuffer->getBufferForSession(m_sessionId);
    if (!data)
    {
        RIALTO_SERVER_LOG_ERROR("No buffer available for session: %d", m_sessionId);
        notifyPlaybackState(PlaybackState::FAILURE);
        return false;
    }

    std::uint32_t regionOffset = 0;
    try
    {
        regionOffset = m_shmBuffer->getBufferOffset(m_sessionId, mediaSourceType);
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
            m_dataReaderFactory->createDataReader(mediaSourceType, data, regionOffset, numFrames);
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

AddSegmentStatus MediaPipelineServerInternal::addSegment(uint32_t needDataRequestId,
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

    if (m_mediaPipelineClient)
    {
        m_mediaPipelineClient->notifyPlaybackState(state);
    }
}

bool MediaPipelineServerInternal::notifyNeedMediaData(MediaSourceType mediaSourceType)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    m_shmBuffer->clearBuffer(m_sessionId, mediaSourceType);
    NeedMediaData event{m_mediaPipelineClient, *m_activeRequests, *m_shmBuffer, m_sessionId, mediaSourceType};
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

    if (m_mediaPipelineClient)
    {
        m_mediaPipelineClient->notifyPosition(position);
    }
}

void MediaPipelineServerInternal::notifyNetworkState(NetworkState state)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (m_mediaPipelineClient)
    {
        m_mediaPipelineClient->notifyNetworkState(state);
    }
}

void MediaPipelineServerInternal::clearActiveRequestsCache()
{
    m_activeRequests->clear();
}

void MediaPipelineServerInternal::notifyQos(MediaSourceType mediaSourceType, const QosInfo &qosInfo)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (m_mediaPipelineClient)
    {
        auto sourceId = static_cast<std::uint64_t>(mediaSourceType);
        m_mediaPipelineClient->notifyQos(sourceId, qosInfo);
    }
}

}; // namespace firebolt::rialto::server
