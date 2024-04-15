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
#include "HeartbeatProcedureMock.h"
#include "MediaPipelineCapabilitiesFactoryMock.h"
#include "MediaPipelineCapabilitiesMock.h"
#include "MediaPipelineServerInternalFactoryMock.h"
#include "PlaybackService.h"
#include "SharedMemoryBufferFactoryMock.h"
#include "SharedMemoryBufferMock.h"
#include "WebAudioPlayerServerInternalFactoryMock.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class PlaybackServiceTests : public testing::Test
{
public:
    PlaybackServiceTests();
    ~PlaybackServiceTests() = default;

    void sharedMemoryBufferWillBeInitialized();
    void sharedMemoryBufferWillReturnFdAndSize();

    void triggerSwitchToActive();
    void triggerSwitchToInactive();
    void triggerSetMaxPlaybacks();
    void triggerSetMaxWebAudioPlayers();
    void triggerSetClientDisplayName();
    void triggerPing();

    void createPlaybackServiceShouldSuccess();
    void getSharedMemoryShouldSucceed();
    void getSharedMemoryShouldFail();
    void getShmBufferShouldSucceed();
    void getShmBufferShouldFail();
    void getMaxPlaybacksShouldSucceed();
    void getMaxWebAudioPlayersShouldSucceed();
    void clientDisplayNameShouldBeSet();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::server::MediaPipelineServerInternalFactoryMock>> m_mediaPipelineFactoryMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesFactoryMock>>
        m_mediaPipelineCapabilitiesFactoryMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::WebAudioPlayerServerInternalFactoryMock>> m_webAudioPlayerFactoryMock;
    std::unique_ptr<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock>> m_mediaPipelineCapabilities;
    StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock> &m_mediaPipelineCapabilitiesMock;
    std::unique_ptr<firebolt::rialto::server::ISharedMemoryBufferFactory> m_shmBufferFactory;
    StrictMock<firebolt::rialto::server::SharedMemoryBufferFactoryMock> &m_shmBufferFactoryMock;
    std::shared_ptr<firebolt::rialto::server::ISharedMemoryBuffer> m_shmBuffer;
    StrictMock<firebolt::rialto::server::SharedMemoryBufferMock> &m_shmBufferMock;
    StrictMock<firebolt::rialto::server::DecryptionServiceMock> m_decryptionServiceMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::HeartbeatProcedureMock>> m_heartbeatProcedureMock;
    std::unique_ptr<firebolt::rialto::server::service::PlaybackService> m_sut;
};

#endif // PLAYBACK_SERVICE_TESTS_FIXTURE_H_
