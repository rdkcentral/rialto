/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "MediaPipelineModuleStub.h"
#include <IIpcServer.h>
#include <IIpcServerFactory.h>
#include <gtest/gtest.h>

namespace
{
firebolt::rialto::PlaybackStateChangeEvent_PlaybackState
convertPlaybackState(const firebolt::rialto::PlaybackState &playbackState)
{
    switch (playbackState)
    {
    case firebolt::rialto::PlaybackState::UNKNOWN:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_UNKNOWN;
    }
    case firebolt::rialto::PlaybackState::IDLE:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_IDLE;
    }
    case firebolt::rialto::PlaybackState::PLAYING:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PLAYING;
    }
    case firebolt::rialto::PlaybackState::PAUSED:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PAUSED;
    }
    case firebolt::rialto::PlaybackState::SEEKING:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_SEEKING;
    }
    case firebolt::rialto::PlaybackState::FLUSHED:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_FLUSHED;
    }
    case firebolt::rialto::PlaybackState::STOPPED:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_STOPPED;
    }
    case firebolt::rialto::PlaybackState::END_OF_STREAM:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_END_OF_STREAM;
    }
    case firebolt::rialto::PlaybackState::FAILURE:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_FAILURE;
    }
    }
    return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_UNKNOWN;
}

firebolt::rialto::NetworkStateChangeEvent_NetworkState
convertNetworkState(const firebolt::rialto::NetworkState &networkState)
{
    switch (networkState)
    {
    case firebolt::rialto::NetworkState::UNKNOWN:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_UNKNOWN;
    }
    case firebolt::rialto::NetworkState::IDLE:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_IDLE;
    }
    case firebolt::rialto::NetworkState::BUFFERING:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERING;
    }
    case firebolt::rialto::NetworkState::BUFFERING_PROGRESS:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERING_PROGRESS;
    }
    case firebolt::rialto::NetworkState::BUFFERED:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED;
    }
    case firebolt::rialto::NetworkState::STALLED:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_STALLED;
    }
    case firebolt::rialto::NetworkState::FORMAT_ERROR:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_FORMAT_ERROR;
    }
    case firebolt::rialto::NetworkState::NETWORK_ERROR:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_NETWORK_ERROR;
    }
    case firebolt::rialto::NetworkState::DECODE_ERROR:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_DECODE_ERROR;
    }
    }
    return firebolt::rialto::NetworkStateChangeEvent_NetworkState_UNKNOWN;
}
} // namespace

namespace firebolt::rialto::componenttest::stub
{
MediaPipelineModuleStub::MediaPipelineModuleStub(const std::shared_ptr<::firebolt::rialto::MediaPipelineModule>& mediaPipelineModuleMock)
{
    m_mediaPipelineModuleMock = mediaPipelineModuleMock;
}

MediaPipelineModuleStub::~MediaPipelineModuleStub()
{
}

void MediaPipelineModuleStub::notifyPlaybackStateChangeEvent(int sessionId, PlaybackState state)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::PlaybackStateChangeEvent>();
    event->set_session_id(sessionId);
    event->set_state(convertPlaybackState(state));
    getClient()->sendEvent(event);
}

void MediaPipelineModuleStub::notifyNetworkStateChangeEvent(int sessionId, NetworkState state)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::NetworkStateChangeEvent>();
    event->set_session_id(sessionId);
    event->set_state(convertNetworkState(state));
    getClient()->sendEvent(event);
}

void MediaPipelineModuleStub::notifyPositionChangeEvent(int sessionId, int64_t position)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::PositionChangeEvent>();
    event->set_session_id(sessionId);
    event->set_position(position);
    getClient()->sendEvent(event);
}

void MediaPipelineModuleStub::notifyNeedMediaDataEvent(int sessionId, int32_t sourceId, size_t frameCount, uint32_t needDataRequestId,
                                              const std::shared_ptr<MediaPlayerShmInfo> &shmInfo)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::NeedMediaDataEvent>();
    event->set_session_id(sessionId);
    event->set_source_id(sourceId);
    event->set_request_id(needDataRequestId);
    event->set_frame_count(frameCount);
    event->mutable_shm_info()->set_max_metadata_bytes(shmInfo->maxMetadataBytes);
    event->mutable_shm_info()->set_metadata_offset(shmInfo->metadataOffset);
    event->mutable_shm_info()->set_media_data_offset(shmInfo->mediaDataOffset);
    event->mutable_shm_info()->set_max_media_bytes(shmInfo->maxMediaBytes);

    getClient()->sendEvent(event);
}

void MediaPipelineModuleStub::notifyQosEvent(int sessionId, int32_t sourceId, const QosInfo &qosInfo)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::QosEvent>();
    event->set_session_id(sessionId);
    event->set_source_id(sourceId);
    event->mutable_qos_info()->set_processed(qosInfo.processed);
    event->mutable_qos_info()->set_dropped(qosInfo.dropped);

    getClient()->sendEvent(event);
}


void MediaPipelineModuleStub::notifyBufferUnderflowEvent(int sessionId, int32_t sourceId)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::BufferUnderflowEvent>();
    event->set_session_id(sessionId);
    event->set_source_id(sourceId);

    getClient()->sendEvent(event);
}


void MediaPipelineModuleStub::notifySetLogLevelsEvent(RIALTO_DEBUG_LEVEL defaultLogLevels, RIALTO_DEBUG_LEVEL clientLogLevels,
                                                      RIALTO_DEBUG_LEVEL ipcLogLevels, RIALTO_DEBUG_LEVEL commonLogLevels)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::SetLogLevelsEvent>();
    event->set_defaultloglevels(static_cast<std::uint32_t>(defaultLogLevels));
    event->set_clientloglevels(static_cast<std::uint32_t>(clientLogLevels));
    event->set_ipcloglevels(static_cast<std::uint32_t>(ipcLogLevels));
    event->set_commonloglevels(static_cast<std::uint32_t>(commonLogLevels));

    getClient()->sendEvent(event);
}

} // namespace firebolt::rialto::componenttests
