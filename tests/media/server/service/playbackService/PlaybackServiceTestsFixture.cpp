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
constexpr int sessionId{0};
const std::shared_ptr<firebolt::rialto::IMediaPipelineClient> mediaPipelineClient; // nullptr as it's not used anywhere in tests
constexpr std::uint32_t width{1920};
constexpr std::uint32_t height{1080};
constexpr firebolt::rialto::VideoRequirements requirements{width, height};
constexpr firebolt::rialto::MediaType type{firebolt::rialto::MediaType::MSE};
const std::string mimeType{"exampleMimeType"};
const std::string url{"http://example.url.com"};
constexpr std::int32_t sourceId{8};
constexpr double rate{0.7};
constexpr int64_t position{4200000000};
constexpr std::uint32_t x{3};
constexpr std::uint32_t y{7};
constexpr firebolt::rialto::MediaSourceStatus status{firebolt::rialto::MediaSourceStatus::CODEC_CHANGED};
constexpr std::uint32_t needDataRequestId{17};
constexpr std::uint32_t numFrames{1};
constexpr std::int32_t shmFd{234};
constexpr std::uint32_t shmSize{2048};
} // namespace

namespace firebolt::rialto
{
bool operator==(const VideoRequirements &lhs, const VideoRequirements &rhs)
{
    return lhs.maxWidth == rhs.maxWidth && lhs.maxHeight == rhs.maxHeight;
}
} // namespace firebolt::rialto

PlaybackServiceTests::PlaybackServiceTests()
    : m_mediaPipelineFactoryMock{std::make_shared<
          StrictMock<firebolt::rialto::server::MediaPipelineServerInternalFactoryMock>>()},
      m_mediaPipelineCapabilitiesFactoryMock{
          std::make_shared<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesFactoryMock>>()},
      m_mediaPipelineCapabilities{std::make_unique<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock>>()},
      m_mediaPipelineCapabilitiesMock{dynamic_cast<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock> &>(
          *m_mediaPipelineCapabilities)},
      m_shmBufferFactory{std::make_unique<StrictMock<firebolt::rialto::server::SharedMemoryBufferFactoryMock>>()},
      m_shmBufferFactoryMock{
          dynamic_cast<StrictMock<firebolt::rialto::server::SharedMemoryBufferFactoryMock> &>(*m_shmBufferFactory)},
      m_shmBuffer{std::make_shared<StrictMock<firebolt::rialto::server::SharedMemoryBufferMock>>()},
      m_shmBufferMock{dynamic_cast<StrictMock<firebolt::rialto::server::SharedMemoryBufferMock> &>(*m_shmBuffer)},
      m_mediaPipeline{std::make_unique<StrictMock<firebolt::rialto::server::MediaPipelineServerInternalMock>>()},
      m_mediaPipelineMock{
          dynamic_cast<StrictMock<firebolt::rialto::server::MediaPipelineServerInternalMock> &>(*m_mediaPipeline)}
{
}

void PlaybackServiceTests::sharedMemoryBufferWillBeInitialized(int maxPlaybacks)
{
    EXPECT_CALL(m_shmBufferFactoryMock, createSharedMemoryBuffer(maxPlaybacks))
        .WillOnce(Return(ByMove(std::move(m_shmBuffer))));
}

void PlaybackServiceTests::sharedMemoryBufferWillFailToInitialize(int maxPlaybacks)
{
    EXPECT_CALL(m_shmBufferFactoryMock, createSharedMemoryBuffer(maxPlaybacks))
        .WillOnce(Throw(std::runtime_error("Buffer creation failed")));
}

void PlaybackServiceTests::sharedMemoryBufferWillReturnFdAndSize()
{
    EXPECT_CALL(m_shmBufferMock, getFd()).WillOnce(Return(shmFd));
    EXPECT_CALL(m_shmBufferMock, getSize()).WillOnce(Return(shmSize));
}

void PlaybackServiceTests::mediaPipelineWillLoad()
{
    EXPECT_CALL(m_mediaPipelineMock, load(type, mimeType, url)).WillOnce(Return(true));
}

void PlaybackServiceTests::mediaPipelineWillFailToLoad()
{
    EXPECT_CALL(m_mediaPipelineMock, load(type, mimeType, url)).WillOnce(Return(false));
}

void PlaybackServiceTests::mediaPipelineWillAttachSource()
{
    EXPECT_CALL(m_mediaPipelineMock, attachSource(_)).WillOnce(Return(true));
}

void PlaybackServiceTests::mediaPipelineWillFailToAttachSource()
{
    EXPECT_CALL(m_mediaPipelineMock, attachSource(_)).WillOnce(Return(false));
}

void PlaybackServiceTests::mediaPipelineWillRemoveSource()
{
    EXPECT_CALL(m_mediaPipelineMock, removeSource(sourceId)).WillOnce(Return(true));
}

void PlaybackServiceTests::mediaPipelineWillFailToRemoveSource()
{
    EXPECT_CALL(m_mediaPipelineMock, removeSource(sourceId)).WillOnce(Return(false));
}

void PlaybackServiceTests::mediaPipelineWillPlay()
{
    EXPECT_CALL(m_mediaPipelineMock, play()).WillOnce(Return(true));
}

void PlaybackServiceTests::mediaPipelineWillFailToPlay()
{
    EXPECT_CALL(m_mediaPipelineMock, play()).WillOnce(Return(false));
}

void PlaybackServiceTests::mediaPipelineWillPause()
{
    EXPECT_CALL(m_mediaPipelineMock, pause()).WillOnce(Return(true));
}

void PlaybackServiceTests::mediaPipelineWillFailToPause()
{
    EXPECT_CALL(m_mediaPipelineMock, pause()).WillOnce(Return(false));
}

void PlaybackServiceTests::mediaPipelineWillStop()
{
    EXPECT_CALL(m_mediaPipelineMock, stop()).WillOnce(Return(true));
}

void PlaybackServiceTests::mediaPipelineWillFailToStop()
{
    EXPECT_CALL(m_mediaPipelineMock, stop()).WillOnce(Return(false));
}

void PlaybackServiceTests::mediaPipelineWillSetPlaybackRate()
{
    EXPECT_CALL(m_mediaPipelineMock, setPlaybackRate(rate)).WillOnce(Return(true));
}

void PlaybackServiceTests::mediaPipelineWillFailToSetPlaybackRate()
{
    EXPECT_CALL(m_mediaPipelineMock, setPlaybackRate(rate)).WillOnce(Return(false));
}

void PlaybackServiceTests::mediaPipelineWillSetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, setPosition(position)).WillOnce(Return(true));
}

void PlaybackServiceTests::mediaPipelineWillFailToSetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, setPosition(position)).WillOnce(Return(false));
}

void PlaybackServiceTests::mediaPipelineWillSetVideoWindow()
{
    EXPECT_CALL(m_mediaPipelineMock, setVideoWindow(x, y, width, height)).WillOnce(Return(true));
}

void PlaybackServiceTests::mediaPipelineWillFailToSetVideoWindow()
{
    EXPECT_CALL(m_mediaPipelineMock, setVideoWindow(x, y, width, height)).WillOnce(Return(false));
}

void PlaybackServiceTests::mediaPipelineWillHaveData()
{
    EXPECT_CALL(m_mediaPipelineMock, haveData(status, numFrames, needDataRequestId)).WillOnce(Return(true));
}

void PlaybackServiceTests::mediaPipelineWillFailToHaveData()
{
    EXPECT_CALL(m_mediaPipelineMock, haveData(status, numFrames, needDataRequestId)).WillOnce(Return(false));
}

void PlaybackServiceTests::mediaPipelineWillGetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, getPosition(_)).WillOnce(Invoke([&](int64_t &pos) {
        pos = position;
        return true;
    }));
}

void PlaybackServiceTests::mediaPipelineWillFailToGetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, getPosition(_)).WillOnce(Return(false));
}

void PlaybackServiceTests::mediaPipelineFactoryWillCreateMediaPipeline()
{
    EXPECT_CALL(*m_mediaPipelineFactoryMock, createMediaPipelineServerInternal(_, requirements, _, _, _))
        .WillOnce(Return(ByMove(std::move(m_mediaPipeline))));
}

void PlaybackServiceTests::mediaPipelineFactoryWillReturnNullptr()
{
    EXPECT_CALL(*m_mediaPipelineFactoryMock, createMediaPipelineServerInternal(_, requirements, _, _, _))
        .WillOnce(Return(ByMove(std::unique_ptr<firebolt::rialto::server::IMediaPipelineServerInternal>())));
}

void PlaybackServiceTests::mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesFactoryMock, createMediaPipelineCapabilities())
        .WillOnce(Return(ByMove(std::move(m_mediaPipelineCapabilities))));
    m_sut = std::make_unique<firebolt::rialto::server::service::PlaybackService>(m_mainThreadMock,
                                                                                 m_mediaPipelineFactoryMock,
                                                                                 m_mediaPipelineCapabilitiesFactoryMock,
                                                                                 std::move(m_shmBufferFactory),
                                                                                 m_decryptionServiceMock);
}

void PlaybackServiceTests::mediaPipelineCapabilitiesFactoryWillReturnNullptr()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesFactoryMock, createMediaPipelineCapabilities())
        .WillOnce(Return(ByMove(std::move(std::unique_ptr<firebolt::rialto::IMediaPipelineCapabilities>()))));
    EXPECT_THROW(m_sut =
                     std::make_unique<firebolt::rialto::server::service::PlaybackService>(m_mainThreadMock,
                                                                                          m_mediaPipelineFactoryMock,
                                                                                          m_mediaPipelineCapabilitiesFactoryMock,
                                                                                          std::move(m_shmBufferFactory),
                                                                                          m_decryptionServiceMock),
                 std::runtime_error);
}

void PlaybackServiceTests::triggerSwitchToActive()
{
    m_sut->switchToActive();
}

void PlaybackServiceTests::triggerSwitchToInactive()
{
    m_sut->switchToInactive();
}

void PlaybackServiceTests::triggerSetMaxPlaybacks(int maxPlaybacks)
{
    m_sut->setMaxPlaybacks(maxPlaybacks);
}

void PlaybackServiceTests::createSessionShouldSucceed()
{
    EXPECT_TRUE(m_sut->createSession(sessionId, mediaPipelineClient, width, height));
}

void PlaybackServiceTests::createSessionShouldFail()
{
    EXPECT_FALSE(m_sut->createSession(sessionId, mediaPipelineClient, width, height));
}

void PlaybackServiceTests::destroySessionShouldSucceed()
{
    EXPECT_TRUE(m_sut->destroySession(sessionId));
}

void PlaybackServiceTests::destroySessionShouldFail()
{
    EXPECT_FALSE(m_sut->destroySession(sessionId));
}

void PlaybackServiceTests::loadShouldSucceed()
{
    EXPECT_TRUE(m_sut->load(sessionId, type, mimeType, url));
}

void PlaybackServiceTests::loadShouldFail()
{
    EXPECT_FALSE(m_sut->load(sessionId, type, mimeType, url));
}

void PlaybackServiceTests::attachSourceShouldSucceed()
{
    firebolt::rialto::IMediaPipeline::MediaSource mediaSource{};
    EXPECT_TRUE(m_sut->attachSource(sessionId, mediaSource));
}

void PlaybackServiceTests::attachSourceShouldFail()
{
    firebolt::rialto::IMediaPipeline::MediaSource mediaSource{};
    EXPECT_FALSE(m_sut->attachSource(sessionId, mediaSource));
}

void PlaybackServiceTests::removeSourceShouldSucceed()
{
    EXPECT_TRUE(m_sut->removeSource(sessionId, sourceId));
}

void PlaybackServiceTests::removeSourceShouldFail()
{
    EXPECT_FALSE(m_sut->removeSource(sessionId, sourceId));
}

void PlaybackServiceTests::playShouldSucceed()
{
    EXPECT_TRUE(m_sut->play(sessionId));
}

void PlaybackServiceTests::playShouldFail()
{
    EXPECT_FALSE(m_sut->play(sessionId));
}

void PlaybackServiceTests::pauseShouldSucceed()
{
    EXPECT_TRUE(m_sut->pause(sessionId));
}

void PlaybackServiceTests::pauseShouldFail()
{
    EXPECT_FALSE(m_sut->pause(sessionId));
}

void PlaybackServiceTests::stopShouldSucceed()
{
    EXPECT_TRUE(m_sut->stop(sessionId));
}

void PlaybackServiceTests::stopShouldFail()
{
    EXPECT_FALSE(m_sut->stop(sessionId));
}

void PlaybackServiceTests::setPlaybackRateShouldSucceed()
{
    EXPECT_TRUE(m_sut->setPlaybackRate(sessionId, rate));
}

void PlaybackServiceTests::setPlaybackRateShouldFail()
{
    EXPECT_FALSE(m_sut->setPlaybackRate(sessionId, rate));
}

void PlaybackServiceTests::setPositionShouldSucceed()
{
    EXPECT_TRUE(m_sut->setPosition(sessionId, position));
}

void PlaybackServiceTests::setPositionShouldFail()
{
    EXPECT_FALSE(m_sut->setPosition(sessionId, position));
}

void PlaybackServiceTests::setVideoWindowShouldSucceed()
{
    EXPECT_TRUE(m_sut->setVideoWindow(sessionId, x, y, width, height));
}

void PlaybackServiceTests::setVideoWindowShouldFail()
{
    EXPECT_FALSE(m_sut->setVideoWindow(sessionId, x, y, width, height));
}

void PlaybackServiceTests::haveDataShouldSucceed()
{
    EXPECT_TRUE(m_sut->haveData(sessionId, status, numFrames, needDataRequestId));
}

void PlaybackServiceTests::haveDataShouldFail()
{
    EXPECT_FALSE(m_sut->haveData(sessionId, status, numFrames, needDataRequestId));
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

void PlaybackServiceTests::getPositionShouldSucceed()
{
    std::int64_t targetPosition{};
    EXPECT_TRUE(m_sut->getPosition(sessionId, targetPosition));
    EXPECT_EQ(targetPosition, position);
}

void PlaybackServiceTests::getPositionShouldFail()
{
    std::int64_t targetPosition{};
    EXPECT_FALSE(m_sut->getPosition(sessionId, targetPosition));
}

void PlaybackServiceTests::getSupportedMimeTypesSucceed()
{
    firebolt::rialto::MediaSourceType type = firebolt::rialto::MediaSourceType::VIDEO;
    std::vector<std::string> mimeTypes = {"video/h264", "video/h265"};
    EXPECT_CALL(m_mediaPipelineCapabilitiesMock, getSupportedMimeTypes(type)).WillOnce(Return(mimeTypes));
    EXPECT_THAT(m_sut->getSupportedMimeTypes(firebolt::rialto::MediaSourceType::VIDEO), mimeTypes);
}

void PlaybackServiceTests::isMimeTypeSupportedSucceed()
{
    std::string mimeType = "video/h264";
    EXPECT_CALL(m_mediaPipelineCapabilitiesMock, isMimeTypeSupported(mimeType)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->isMimeTypeSupported(mimeType));
}
