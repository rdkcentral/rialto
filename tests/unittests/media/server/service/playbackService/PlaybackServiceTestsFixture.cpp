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

#include "PlaybackServiceTestsFixture.h"
#include "MediaCommon.h"
#include <string>
#include <utility>
#include <vector>

using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::Return;
using testing::Throw;

namespace
{
constexpr std::int32_t kMaxPlaybacks{2};
constexpr std::int32_t kMaxWebAudioPlayers{2};
const std::string kClientDisplayName{"westeros-rialto"};
} // namespace

PlaybackServiceTests::PlaybackServiceTests()
    : m_mediaPipelineFactoryMock{std::make_shared<
          StrictMock<firebolt::rialto::server::MediaPipelineServerInternalFactoryMock>>()},
      m_mediaPipelineCapabilitiesFactoryMock{
          std::make_shared<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesFactoryMock>>()},
      m_webAudioPlayerFactoryMock{
          std::make_shared<StrictMock<firebolt::rialto::server::WebAudioPlayerServerInternalFactoryMock>>()},
      m_mediaPipelineCapabilities{std::make_unique<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock>>()},
      m_mediaPipelineCapabilitiesMock{dynamic_cast<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock> &>(
          *m_mediaPipelineCapabilities)},
      m_shmBufferFactory{std::make_shared<StrictMock<firebolt::rialto::server::SharedMemoryBufferFactoryMock>>()},
      m_shmBufferFactoryMock{
          dynamic_cast<StrictMock<firebolt::rialto::server::SharedMemoryBufferFactoryMock> &>(*m_shmBufferFactory)},
      m_heartbeatProcedureMock{std::make_shared<StrictMock<firebolt::rialto::server::HeartbeatProcedureMock>>()}
{
}

void PlaybackServiceTests::createPlaybackServiceShouldSuccess()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesFactoryMock, createMediaPipelineCapabilities())
        .WillOnce(Return(ByMove(std::move(m_mediaPipelineCapabilities))));
    m_sut = std::make_unique<firebolt::rialto::server::service::PlaybackService>(m_mediaPipelineFactoryMock,
                                                                                 m_mediaPipelineCapabilitiesFactoryMock,
                                                                                 m_webAudioPlayerFactoryMock,
                                                                                 std::move(m_shmBufferFactory),
                                                                                 m_decryptionServiceMock);
}

void PlaybackServiceTests::triggerSwitchToActive()
{
    m_sut->switchToActive();
}

void PlaybackServiceTests::triggerSwitchToInactive()
{
    m_sut->switchToInactive();
}

void PlaybackServiceTests::triggerSetMaxPlaybacks()
{
    m_sut->setMaxPlaybacks(kMaxPlaybacks);
}

void PlaybackServiceTests::triggerSetMaxWebAudioPlayers()
{
    m_sut->setMaxWebAudioPlayers(kMaxWebAudioPlayers);
}

void PlaybackServiceTests::triggerSetClientDisplayName()
{
    m_sut->setClientDisplayName(kClientDisplayName);
}

void PlaybackServiceTests::triggerPing()
{
    m_sut->ping(m_heartbeatProcedureMock);
}

void PlaybackServiceTests::getMaxPlaybacksShouldSucceed()
{
    EXPECT_EQ(m_sut->getMaxPlaybacks(), kMaxPlaybacks);
}

void PlaybackServiceTests::getMaxWebAudioPlayersShouldSucceed()
{
    EXPECT_EQ(m_sut->getMaxWebAudioPlayers(), kMaxWebAudioPlayers);
}

void PlaybackServiceTests::getPrivateMetricsServiceShouldSucceed()
{
    EXPECT_NO_THROW(m_sut->getPrivateMetricsService());
}

void PlaybackServiceTests::clientDisplayNameShouldBeSet()
{
    EXPECT_EQ(std::string(getenv("WAYLAND_DISPLAY")), kClientDisplayName);
    unsetenv("WAYLAND_DISPLAY");
}
