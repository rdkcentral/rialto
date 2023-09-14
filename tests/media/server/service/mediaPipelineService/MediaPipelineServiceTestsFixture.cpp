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

#include "MediaPipelineServiceTestsFixture.h"
#include "HeartbeatHandlerMock.h"
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
constexpr double volume{0.7};
constexpr bool mute{false};
} // namespace

namespace firebolt::rialto
{
bool operator==(const VideoRequirements &lhs, const VideoRequirements &rhs)
{
    return lhs.maxWidth == rhs.maxWidth && lhs.maxHeight == rhs.maxHeight;
}
} // namespace firebolt::rialto

MediaPipelineServiceTests::MediaPipelineServiceTests()
    : m_mediaPipelineFactoryMock{std::make_shared<
          StrictMock<firebolt::rialto::server::MediaPipelineServerInternalFactoryMock>>()},
      m_mediaPipelineCapabilitiesFactoryMock{
          std::make_shared<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesFactoryMock>>()},
      m_mediaPipelineCapabilities{std::make_unique<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock>>()},
      m_mediaPipelineCapabilitiesMock{dynamic_cast<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock> &>(
          *m_mediaPipelineCapabilities)},
      m_shmBuffer{std::make_shared<StrictMock<firebolt::rialto::server::SharedMemoryBufferMock>>()},
      m_shmBufferMock{dynamic_cast<StrictMock<firebolt::rialto::server::SharedMemoryBufferMock> &>(*m_shmBuffer)},
      m_mediaPipeline{std::make_unique<StrictMock<firebolt::rialto::server::MediaPipelineServerInternalMock>>()},
      m_mediaPipelineMock{
          dynamic_cast<StrictMock<firebolt::rialto::server::MediaPipelineServerInternalMock> &>(*m_mediaPipeline)},
      m_heartbeatProcedureMock{std::make_shared<StrictMock<firebolt::rialto::server::HeartbeatProcedureMock>>()}
{
}

void MediaPipelineServiceTests::mediaPipelineWillLoad()
{
    EXPECT_CALL(m_mediaPipelineMock, load(type, mimeType, url)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToLoad()
{
    EXPECT_CALL(m_mediaPipelineMock, load(type, mimeType, url)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillAttachSource()
{
    EXPECT_CALL(m_mediaPipelineMock, attachSource(_)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToAttachSource()
{
    EXPECT_CALL(m_mediaPipelineMock, attachSource(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillRemoveSource()
{
    EXPECT_CALL(m_mediaPipelineMock, removeSource(sourceId)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToRemoveSource()
{
    EXPECT_CALL(m_mediaPipelineMock, removeSource(sourceId)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillAllSourcesAttached()
{
    EXPECT_CALL(m_mediaPipelineMock, allSourcesAttached()).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToAllSourcesAttached()
{
    EXPECT_CALL(m_mediaPipelineMock, allSourcesAttached()).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillPlay()
{
    EXPECT_CALL(m_mediaPipelineMock, play()).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToPlay()
{
    EXPECT_CALL(m_mediaPipelineMock, play()).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillPause()
{
    EXPECT_CALL(m_mediaPipelineMock, pause()).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToPause()
{
    EXPECT_CALL(m_mediaPipelineMock, pause()).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillStop()
{
    EXPECT_CALL(m_mediaPipelineMock, stop()).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToStop()
{
    EXPECT_CALL(m_mediaPipelineMock, stop()).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetPlaybackRate()
{
    EXPECT_CALL(m_mediaPipelineMock, setPlaybackRate(rate)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetPlaybackRate()
{
    EXPECT_CALL(m_mediaPipelineMock, setPlaybackRate(rate)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, setPosition(position)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, setPosition(position)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetVideoWindow()
{
    EXPECT_CALL(m_mediaPipelineMock, setVideoWindow(x, y, width, height)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetVideoWindow()
{
    EXPECT_CALL(m_mediaPipelineMock, setVideoWindow(x, y, width, height)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillHaveData()
{
    EXPECT_CALL(m_mediaPipelineMock, haveData(status, numFrames, needDataRequestId)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToHaveData()
{
    EXPECT_CALL(m_mediaPipelineMock, haveData(status, numFrames, needDataRequestId)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, getPosition(_))
        .WillOnce(Invoke(
            [&](int64_t &pos)
            {
                pos = position;
                return true;
            }));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, getPosition(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillRenderFrame()
{
    EXPECT_CALL(m_mediaPipelineMock, renderFrame()).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToRenderFrame()
{
    EXPECT_CALL(m_mediaPipelineMock, renderFrame()).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetVolume()
{
    EXPECT_CALL(m_mediaPipelineMock, setVolume(_)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetVolume()
{
    EXPECT_CALL(m_mediaPipelineMock, setVolume(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetVolume()
{
    EXPECT_CALL(m_mediaPipelineMock, getVolume(_))
        .WillOnce(Invoke(
            [&](double &vol)
            {
                vol = volume;
                return true;
            }));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetVolume()
{
    EXPECT_CALL(m_mediaPipelineMock, getVolume(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetMute()
{
    EXPECT_CALL(m_mediaPipelineMock, setMute(_)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetMute()
{
    EXPECT_CALL(m_mediaPipelineMock, setMute(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetMute()
{
    EXPECT_CALL(m_mediaPipelineMock, getMute(_))
        .WillOnce(Invoke(
            [&](bool &mut)
            {
                mut = mute;
                return true;
            }));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetMute()
{
    EXPECT_CALL(m_mediaPipelineMock, getMute(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillPing()
{
    EXPECT_CALL(*m_heartbeatProcedureMock, createHandler())
        .WillOnce(Return(ByMove(std::make_unique<StrictMock<firebolt::rialto::server::HeartbeatHandlerMock>>())));
    EXPECT_CALL(m_mediaPipelineMock, ping(_));
}

void MediaPipelineServiceTests::mediaPipelineFactoryWillCreateMediaPipeline()
{
    EXPECT_CALL(*m_mediaPipelineFactoryMock, createMediaPipelineServerInternal(_, requirements, _, _, _))
        .WillOnce(Return(ByMove(std::move(m_mediaPipeline))));
}

void MediaPipelineServiceTests::mediaPipelineFactoryWillReturnNullptr()
{
    EXPECT_CALL(*m_mediaPipelineFactoryMock, createMediaPipelineServerInternal(_, requirements, _, _, _))
        .WillOnce(Return(ByMove(std::unique_ptr<firebolt::rialto::server::IMediaPipelineServerInternal>())));
}

void MediaPipelineServiceTests::playbackServiceWillReturnActive()
{
    EXPECT_CALL(m_playbackServiceMock, isActive()).WillOnce(Return(true)).RetiresOnSaturation();
}

void MediaPipelineServiceTests::playbackServiceWillReturnInactive()
{
    EXPECT_CALL(m_playbackServiceMock, isActive()).WillOnce(Return(false)).RetiresOnSaturation();
}

void MediaPipelineServiceTests::playbackServiceWillReturnMaxPlaybacks(int maxPlaybacks)
{
    EXPECT_CALL(m_playbackServiceMock, getMaxPlaybacks()).WillOnce(Return(maxPlaybacks)).RetiresOnSaturation();
}

void MediaPipelineServiceTests::playbackServiceWillReturnSharedMemoryBuffer()
{
    EXPECT_CALL(m_playbackServiceMock, getShmBuffer()).WillOnce(Return(m_shmBuffer)).RetiresOnSaturation();
}

void MediaPipelineServiceTests::createMediaPipelineShouldSuccess()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesFactoryMock, createMediaPipelineCapabilities())
        .WillOnce(Return(ByMove(std::move(m_mediaPipelineCapabilities))));
    m_sut =
        std::make_unique<firebolt::rialto::server::service::MediaPipelineService>(m_playbackServiceMock,
                                                                                  m_mediaPipelineFactoryMock,
                                                                                  m_mediaPipelineCapabilitiesFactoryMock,
                                                                                  m_decryptionServiceMock);
}

void MediaPipelineServiceTests::createMediaPipelineShouldFailWhenMediaPipelineCapabilitiesFactoryReturnsNullptr()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesFactoryMock, createMediaPipelineCapabilities())
        .WillOnce(Return(ByMove(std::move(std::unique_ptr<firebolt::rialto::IMediaPipelineCapabilities>()))));
    EXPECT_THROW(m_sut =
                     std::make_unique<firebolt::rialto::server::service::MediaPipelineService>(m_playbackServiceMock,
                                                                                               m_mediaPipelineFactoryMock,
                                                                                               m_mediaPipelineCapabilitiesFactoryMock,
                                                                                               m_decryptionServiceMock),
                 std::runtime_error);
}

void MediaPipelineServiceTests::createSessionShouldSucceed()
{
    EXPECT_TRUE(m_sut->createSession(sessionId, mediaPipelineClient, width, height));
}

void MediaPipelineServiceTests::createSessionShouldFail()
{
    EXPECT_FALSE(m_sut->createSession(sessionId, mediaPipelineClient, width, height));
}

void MediaPipelineServiceTests::destroySessionShouldSucceed()
{
    EXPECT_TRUE(m_sut->destroySession(sessionId));
}

void MediaPipelineServiceTests::destroySessionShouldFail()
{
    EXPECT_FALSE(m_sut->destroySession(sessionId));
}

void MediaPipelineServiceTests::loadShouldSucceed()
{
    EXPECT_TRUE(m_sut->load(sessionId, type, mimeType, url));
}

void MediaPipelineServiceTests::loadShouldFail()
{
    EXPECT_FALSE(m_sut->load(sessionId, type, mimeType, url));
}

void MediaPipelineServiceTests::attachSourceShouldSucceed()
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264");
    EXPECT_TRUE(m_sut->attachSource(sessionId, mediaSource));
}

void MediaPipelineServiceTests::attachSourceShouldFail()
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264");
    EXPECT_FALSE(m_sut->attachSource(sessionId, mediaSource));
}

void MediaPipelineServiceTests::removeSourceShouldSucceed()
{
    EXPECT_TRUE(m_sut->removeSource(sessionId, sourceId));
}

void MediaPipelineServiceTests::removeSourceShouldFail()
{
    EXPECT_FALSE(m_sut->removeSource(sessionId, sourceId));
}

void MediaPipelineServiceTests::allSourcesAttachedShouldSucceed()
{
    EXPECT_TRUE(m_sut->allSourcesAttached(sessionId));
}

void MediaPipelineServiceTests::allSourcesAttachedShouldFail()
{
    EXPECT_FALSE(m_sut->allSourcesAttached(sessionId));
}

void MediaPipelineServiceTests::playShouldSucceed()
{
    EXPECT_TRUE(m_sut->play(sessionId));
}

void MediaPipelineServiceTests::playShouldFail()
{
    EXPECT_FALSE(m_sut->play(sessionId));
}

void MediaPipelineServiceTests::pauseShouldSucceed()
{
    EXPECT_TRUE(m_sut->pause(sessionId));
}

void MediaPipelineServiceTests::pauseShouldFail()
{
    EXPECT_FALSE(m_sut->pause(sessionId));
}

void MediaPipelineServiceTests::stopShouldSucceed()
{
    EXPECT_TRUE(m_sut->stop(sessionId));
}

void MediaPipelineServiceTests::stopShouldFail()
{
    EXPECT_FALSE(m_sut->stop(sessionId));
}

void MediaPipelineServiceTests::setPlaybackRateShouldSucceed()
{
    EXPECT_TRUE(m_sut->setPlaybackRate(sessionId, rate));
}

void MediaPipelineServiceTests::setPlaybackRateShouldFail()
{
    EXPECT_FALSE(m_sut->setPlaybackRate(sessionId, rate));
}

void MediaPipelineServiceTests::setPositionShouldSucceed()
{
    EXPECT_TRUE(m_sut->setPosition(sessionId, position));
}

void MediaPipelineServiceTests::setPositionShouldFail()
{
    EXPECT_FALSE(m_sut->setPosition(sessionId, position));
}

void MediaPipelineServiceTests::setVideoWindowShouldSucceed()
{
    EXPECT_TRUE(m_sut->setVideoWindow(sessionId, x, y, width, height));
}

void MediaPipelineServiceTests::setVideoWindowShouldFail()
{
    EXPECT_FALSE(m_sut->setVideoWindow(sessionId, x, y, width, height));
}

void MediaPipelineServiceTests::haveDataShouldSucceed()
{
    EXPECT_TRUE(m_sut->haveData(sessionId, status, numFrames, needDataRequestId));
}

void MediaPipelineServiceTests::haveDataShouldFail()
{
    EXPECT_FALSE(m_sut->haveData(sessionId, status, numFrames, needDataRequestId));
}

void MediaPipelineServiceTests::getPositionShouldSucceed()
{
    std::int64_t targetPosition{};
    EXPECT_TRUE(m_sut->getPosition(sessionId, targetPosition));
    EXPECT_EQ(targetPosition, position);
}

void MediaPipelineServiceTests::getPositionShouldFail()
{
    std::int64_t targetPosition{};
    EXPECT_FALSE(m_sut->getPosition(sessionId, targetPosition));
}

void MediaPipelineServiceTests::getSupportedMimeTypesSucceed()
{
    firebolt::rialto::MediaSourceType type = firebolt::rialto::MediaSourceType::VIDEO;
    std::vector<std::string> mimeTypes = {"video/h264", "video/h265"};
    EXPECT_CALL(m_mediaPipelineCapabilitiesMock, getSupportedMimeTypes(type)).WillOnce(Return(mimeTypes));
    EXPECT_THAT(m_sut->getSupportedMimeTypes(firebolt::rialto::MediaSourceType::VIDEO), mimeTypes);
}

void MediaPipelineServiceTests::isMimeTypeSupportedSucceed()
{
    std::string mimeType = "video/h264";
    EXPECT_CALL(m_mediaPipelineCapabilitiesMock, isMimeTypeSupported(mimeType)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->isMimeTypeSupported(mimeType));
}

void MediaPipelineServiceTests::renderFrameShouldSucceed()
{
    EXPECT_TRUE(m_sut->renderFrame(sessionId));
}

void MediaPipelineServiceTests::renderFrameShouldFail()
{
    EXPECT_FALSE(m_sut->renderFrame(sessionId));
}

void MediaPipelineServiceTests::setVolumeShouldSucceed()
{
    EXPECT_TRUE(m_sut->setVolume(sessionId, volume));
}

void MediaPipelineServiceTests::setVolumeShouldFail()
{
    EXPECT_FALSE(m_sut->setVolume(sessionId, volume));
}

void MediaPipelineServiceTests::getVolumeShouldSucceed()
{
    double targetVolume{};
    EXPECT_TRUE(m_sut->getVolume(sessionId, targetVolume));
    EXPECT_EQ(targetVolume, volume);
}

void MediaPipelineServiceTests::getVolumeShouldFail()
{
    double targetVolume{};
    EXPECT_FALSE(m_sut->getVolume(sessionId, targetVolume));
}

void MediaPipelineServiceTests::setMuteShouldSucceed()
{
    EXPECT_TRUE(m_sut->setMute(sessionId, mute));
}

void MediaPipelineServiceTests::setMuteShouldFail()
{
    EXPECT_FALSE(m_sut->setMute(sessionId, mute));
}

void MediaPipelineServiceTests::getMuteShouldSucceed()
{
    bool targetMute{};
    EXPECT_TRUE(m_sut->getMute(sessionId, targetMute));
    EXPECT_EQ(targetMute, mute);
}

void MediaPipelineServiceTests::getMuteShouldFail()
{
    bool targetMute{};
    EXPECT_FALSE(m_sut->getMute(sessionId, targetMute));
}
void MediaPipelineServiceTests::clearMediaPipelines()
{
    m_sut->clearMediaPipelines();
}

void MediaPipelineServiceTests::initSession()
{
    createMediaPipelineShouldSuccess();
    playbackServiceWillReturnActive();
    playbackServiceWillReturnMaxPlaybacks(1);
    playbackServiceWillReturnSharedMemoryBuffer();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
}

void MediaPipelineServiceTests::triggerPing()
{
    m_sut->ping(m_heartbeatProcedureMock);
}
