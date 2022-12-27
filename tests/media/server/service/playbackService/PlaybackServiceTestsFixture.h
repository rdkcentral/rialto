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

#ifndef PLAYBACK_SERVICE_TESTS_FIXTURE_H_
#define PLAYBACK_SERVICE_TESTS_FIXTURE_H_

#include "DecryptionServiceMock.h"
#include "MediaPipelineCapabilitiesFactoryMock.h"
#include "MediaPipelineCapabilitiesMock.h"
#include "MediaPipelineServerInternalFactoryMock.h"
#include "MediaPipelineServerInternalMock.h"
#include "PlaybackService.h"
#include "SharedMemoryBufferFactoryMock.h"
#include "SharedMemoryBufferMock.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class PlaybackServiceTests : public testing::Test
{
public:
    PlaybackServiceTests();
    ~PlaybackServiceTests() = default;

    void sharedMemoryBufferWillBeInitialized(int maxPlaybacks = 1);
    void sharedMemoryBufferWillFailToInitialize(int maxPlaybacks = 1);
    void sharedMemoryBufferWillReturnFdAndSize();

    void mediaPipelineWillLoad();
    void mediaPipelineWillFailToLoad();
    void mediaPipelineWillAttachSource();
    void mediaPipelineWillFailToAttachSource();
    void mediaPipelineWillRemoveSource();
    void mediaPipelineWillFailToRemoveSource();
    void mediaPipelineWillPlay();
    void mediaPipelineWillFailToPlay();
    void mediaPipelineWillPause();
    void mediaPipelineWillFailToPause();
    void mediaPipelineWillStop();
    void mediaPipelineWillFailToStop();
    void mediaPipelineWillSetPlaybackRate();
    void mediaPipelineWillFailToSetPlaybackRate();
    void mediaPipelineWillSetPosition();
    void mediaPipelineWillFailToSetPosition();
    void mediaPipelineWillSetVideoWindow();
    void mediaPipelineWillFailToSetVideoWindow();
    void mediaPipelineWillHaveData();
    void mediaPipelineWillFailToHaveData();
    void mediaPipelineWillGetPosition();
    void mediaPipelineWillFailToGetPosition();
    void mediaPipelineWillSetVolume();
    void mediaPipelineWillFailToSetVolume();
    void mediaPipelineWillGetVolume();
    void mediaPipelineWillFailToGetVolume();

    void mediaPipelineFactoryWillCreateMediaPipeline();
    void mediaPipelineFactoryWillReturnNullptr();

    void mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    void mediaPipelineCapabilitiesFactoryWillReturnNullptr();

    void triggerSwitchToActive();
    void triggerSwitchToInactive();
    void triggerSetMaxPlaybacks(int maxPlaybacks = 1);

    void createSessionShouldSucceed();
    void createSessionShouldFail();
    void destroySessionShouldSucceed();
    void destroySessionShouldFail();
    void loadShouldSucceed();
    void loadShouldFail();
    void attachSourceShouldSucceed();
    void attachSourceShouldFail();
    void removeSourceShouldSucceed();
    void removeSourceShouldFail();
    void playShouldSucceed();
    void playShouldFail();
    void pauseShouldSucceed();
    void pauseShouldFail();
    void stopShouldSucceed();
    void stopShouldFail();
    void setPlaybackRateShouldSucceed();
    void setPlaybackRateShouldFail();
    void setPositionShouldSucceed();
    void setPositionShouldFail();
    void setVideoWindowShouldSucceed();
    void setVideoWindowShouldFail();
    void haveDataShouldSucceed();
    void haveDataShouldFail();
    void getSharedMemoryShouldSucceed();
    void getSharedMemoryShouldFail();
    void getPositionShouldSucceed();
    void getPositionShouldFail();
    void getSupportedMimeTypesSucceed();
    void isMimeTypeSupportedSucceed();
    void renderFrameSucceed();
    void setVolumeShouldSucceed();
    void setVolumeShouldFail();
    void getVolumeShouldSucceed();
    void getVolumeShouldFail();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::server::MediaPipelineServerInternalFactoryMock>> m_mediaPipelineFactoryMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesFactoryMock>>
        m_mediaPipelineCapabilitiesFactoryMock;
    std::unique_ptr<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock>> m_mediaPipelineCapabilities;
    StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock> &m_mediaPipelineCapabilitiesMock;
    std::unique_ptr<firebolt::rialto::server::ISharedMemoryBufferFactory> m_shmBufferFactory;
    StrictMock<firebolt::rialto::server::SharedMemoryBufferFactoryMock> &m_shmBufferFactoryMock;
    std::shared_ptr<firebolt::rialto::server::ISharedMemoryBuffer> m_shmBuffer;
    StrictMock<firebolt::rialto::server::SharedMemoryBufferMock> &m_shmBufferMock;
    std::unique_ptr<firebolt::rialto::server::IMediaPipelineServerInternal> m_mediaPipeline;
    StrictMock<firebolt::rialto::server::MediaPipelineServerInternalMock> &m_mediaPipelineMock;
    StrictMock<firebolt::rialto::server::DecryptionServiceMock> m_decryptionServiceMock;
    std::unique_ptr<firebolt::rialto::server::service::PlaybackService> m_sut;
};

#endif // PLAYBACK_SERVICE_TESTS_FIXTURE_H_
