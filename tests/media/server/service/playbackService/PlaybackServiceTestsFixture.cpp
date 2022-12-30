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
constexpr std::int32_t shmFd{234};
constexpr std::uint32_t shmSize{2048};
constexpr std::int32_t maxPlaybacks{2};
constexpr std::int32_t maxWebAudioInstances{2};
} // namespace

PlaybackServiceTests::PlaybackServiceTests()
    : m_mediaPipelineFactoryMock{std::make_shared<
          StrictMock<firebolt::rialto::server::MediaPipelineServerInternalFactoryMock>>()},
      m_mediaPipelineCapabilitiesFactoryMock{
          std::make_shared<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesFactoryMock>>()},
      m_webAudioPlayerFactoryMock{
          std::make_shared<StrictMock<firebolt::rialto::server::WebAudioPlayerFactoryMock>>()},
      m_mediaPipelineCapabilities{std::make_unique<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock>>()},
      m_mediaPipelineCapabilitiesMock{dynamic_cast<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock> &>(
          *m_mediaPipelineCapabilities)},
      m_shmBufferFactory{std::make_unique<StrictMock<firebolt::rialto::server::SharedMemoryBufferFactoryMock>>()},
      m_shmBufferFactoryMock{
          dynamic_cast<StrictMock<firebolt::rialto::server::SharedMemoryBufferFactoryMock> &>(*m_shmBufferFactory)},
      m_shmBuffer{std::make_shared<StrictMock<firebolt::rialto::server::SharedMemoryBufferMock>>()},
      m_shmBufferMock{dynamic_cast<StrictMock<firebolt::rialto::server::SharedMemoryBufferMock> &>(*m_shmBuffer)}
{
}

void PlaybackServiceTests::sharedMemoryBufferWillBeInitialized()
{
    EXPECT_CALL(m_shmBufferFactoryMock, createSharedMemoryBuffer(maxPlaybacks))
        .WillOnce(Return(ByMove(std::move(m_shmBuffer))));
}

void PlaybackServiceTests::sharedMemoryBufferWillFailToInitialize()
{
    EXPECT_CALL(m_shmBufferFactoryMock, createSharedMemoryBuffer(maxPlaybacks))
        .WillOnce(Throw(std::runtime_error("Buffer creation failed")));
}

void PlaybackServiceTests::sharedMemoryBufferWillReturnFdAndSize()
{
    EXPECT_CALL(m_shmBufferMock, getFd()).WillOnce(Return(shmFd));
    EXPECT_CALL(m_shmBufferMock, getSize()).WillOnce(Return(shmSize));
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
    m_sut->setMaxPlaybacks(maxPlaybacks);
}

void PlaybackServiceTests::triggerSetMaxWebAudioInstances()
{
    m_sut->setMaxWebAudioInstances(maxWebAudioInstances);
}

void PlaybackServiceTests::getSharedMemoryShouldSucceed()
{
    int32_t returnedFd = 0;
    uint32_t returnedSize = 0;
    EXPECT_TRUE(m_sut->getSharedMemory(returnedFd, returnedSize));
    EXPECT_EQ(returnedFd, shmFd);
    EXPECT_EQ(returnedSize, shmSize);
}

void PlaybackServiceTests::getSharedMemoryShouldFail()
{
    int32_t returnedFd = 0;
    uint32_t returnedSize = 0;
    EXPECT_FALSE(m_sut->getSharedMemory(returnedFd, returnedSize));
    EXPECT_EQ(returnedFd, 0);
    EXPECT_EQ(returnedSize, 0);
}

void PlaybackServiceTests::getShmBufferShouldSucceed()
{
    EXPECT_NE(m_sut->getShmBuffer(), nullptr);
}

void PlaybackServiceTests::getShmBufferShouldFail()
{
    EXPECT_EQ(m_sut->getShmBuffer(), nullptr);
}

void PlaybackServiceTests::getMaxPlaybacksShouldSucceed()
{
    EXPECT_EQ(m_sut->getMaxPlaybacks(), maxPlaybacks);
}

void PlaybackServiceTests::getMaxWebAudioInstancesShouldSucceed()
{
    EXPECT_EQ(m_sut->getMaxWebAudioInstances(), maxWebAudioInstances);
}
