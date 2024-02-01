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
#include "MediaPipelineProtoUtils.h"
#include <IIpcServer.h>
#include <IIpcServerFactory.h>
#include <gtest/gtest.h>
#include <memory>

namespace firebolt::rialto::client::ct
{
MediaPipelineModuleStub::MediaPipelineModuleStub(
    const std::shared_ptr<::firebolt::rialto::MediaPipelineModule> &mediaPipelineModuleMock)
    : m_mediaPipelineModuleMock{mediaPipelineModuleMock}
{
}

MediaPipelineModuleStub::~MediaPipelineModuleStub() {}

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

void MediaPipelineModuleStub::notifyNeedMediaDataEvent(int sessionId, int32_t sourceId, size_t frameCount,
                                                       uint32_t needDataRequestId,
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

void MediaPipelineModuleStub::notifyPositionChangeEvent(int sessionId, int64_t position)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::PositionChangeEvent>();
    event->set_session_id(sessionId);
    event->set_position(position);
    getClient()->sendEvent(event);
}

void MediaPipelineModuleStub::notifyQosEvent(int sessionId, int32_t sourceId, const ::firebolt::rialto::QosInfo &qosInfo)
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

void MediaPipelineModuleStub::notifyPlaybackErrorEvent(int sessionId, int32_t sourceId, PlaybackError error)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::PlaybackErrorEvent>();
    event->set_session_id(sessionId);
    event->set_source_id(sourceId);
    event->set_error(convertPlaybackError(error));
    getClient()->sendEvent(event);
}

} // namespace firebolt::rialto::client::ct
