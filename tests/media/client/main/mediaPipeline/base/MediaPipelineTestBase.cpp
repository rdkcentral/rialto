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

#include "MediaPipelineTestBase.h"
#include <memory>
#include <utility>

void MediaPipelineTestBase::SetUp() // NOLINT(build/function_format)
{
    // Create StrictMocks
    m_mediaPipelineClientMock = std::make_shared<StrictMock<MediaPipelineClientMock>>();
    m_mediaPipelineIpcFactoryMock = std::make_shared<StrictMock<MediaPipelineIpcFactoryMock>>();
    m_mediaFrameWriterFactoryMock = std::make_shared<StrictMock<MediaFrameWriterFactoryMock>>();
    m_sharedMemoryManagerFactoryMock = std::make_shared<StrictMock<SharedMemoryManagerFactoryMock>>();
    m_sharedMemoryManagerMock = std::make_shared<StrictMock<SharedMemoryManagerMock>>();
}

void MediaPipelineTestBase::TearDown() // NOLINT(build/function_format)
{
    // Destroy StrictMocks
    m_sharedMemoryManagerMock.reset();
    m_sharedMemoryManagerFactoryMock.reset();
    m_mediaFrameWriterFactoryMock.reset();
    m_mediaPipelineIpcMock = nullptr;
    m_mediaPipelineIpcFactoryMock.reset();
    m_mediaPipelineClientMock.reset();
}

void MediaPipelineTestBase::createMediaPipeline()
{
    VideoRequirements videoReq = {};
    std::unique_ptr<StrictMock<MediaPipelineIpcMock>> mediaPipelineIpcMock =
        std::make_unique<StrictMock<MediaPipelineIpcMock>>();

    // Save a raw pointer to the unique object for use when testing mocks
    // Object shall be freed by the holder of the unique ptr on destruction
    m_mediaPipelineIpcMock = mediaPipelineIpcMock.get();

    EXPECT_CALL(*m_sharedMemoryManagerFactoryMock, getSharedMemoryManager()).WillOnce(Return(m_sharedMemoryManagerMock));
    EXPECT_CALL(*m_sharedMemoryManagerMock, registerClient(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mediaPipelineIpcFactoryMock, createMediaPipelineIpc(_, _))
        .WillOnce(DoAll(SaveArg<0>(&m_mediaPipelineCallback), Return(ByMove(std::move(mediaPipelineIpcMock)))));

    EXPECT_NO_THROW(m_mediaPipeline = std::make_unique<MediaPipeline>(m_mediaPipelineClientMock, videoReq,
                                                                      m_mediaPipelineIpcFactoryMock,
                                                                      m_mediaFrameWriterFactoryMock,
                                                                      m_sharedMemoryManagerFactoryMock));
    EXPECT_NE(m_mediaPipeline, nullptr);
}

void MediaPipelineTestBase::destroyMediaPipeline()
{
    EXPECT_CALL(*m_sharedMemoryManagerMock, unregisterClient(_)).WillOnce(Return(true));

    m_mediaPipeline.reset();
    m_mediaPipelineCallback = nullptr;
}

void MediaPipelineTestBase::setPlaybackState(PlaybackState state)
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(state));
    m_mediaPipelineCallback->notifyPlaybackState(state);
}

void MediaPipelineTestBase::setNetworkState(NetworkState state)
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(state));
    m_mediaPipelineCallback->notifyNetworkState(state);
}

void MediaPipelineTestBase::needData(int32_t sourceId, size_t frameCount, uint32_t requestId,
                                     const std::shared_ptr<MediaPlayerShmInfo> &shmInfo)
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNeedMediaData(sourceId, frameCount, requestId, IsNull()))
        .RetiresOnSaturation();

    m_mediaPipelineCallback->notifyNeedMediaData(sourceId, frameCount, requestId, shmInfo);
}
