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
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgReferee;
using testing::StrEq;
using testing::Throw;

using firebolt::rialto::MediaSourceType;

namespace
{
constexpr int kSessionId{0};
const std::shared_ptr<firebolt::rialto::IMediaPipelineClient> kMediaPipelineClient; // nullptr as it's not used anywhere in tests
constexpr std::uint32_t kWidth{1920};
constexpr std::uint32_t kHeight{1080};
constexpr firebolt::rialto::VideoRequirements kRequirements{kWidth, kHeight};
constexpr firebolt::rialto::MediaType kType{firebolt::rialto::MediaType::MSE};
const std::string kMimeType{"exampleMimeType"};
const std::string kUrl{"http://example.url.com"};
constexpr std::int32_t kSourceId{8};
constexpr double kRate{0.7};
constexpr int64_t kPosition{4200000000};
constexpr std::uint32_t x{3};
constexpr std::uint32_t y{7};
constexpr firebolt::rialto::MediaSourceStatus kStatus{firebolt::rialto::MediaSourceStatus::CODEC_CHANGED};
constexpr std::uint32_t kNeedDataRequestId{17};
constexpr std::uint32_t kNumFrames{1};
constexpr double kVolume{0.7};
constexpr uint32_t kVolumeDuration{1000};
constexpr firebolt::rialto::EaseType kEaseType{firebolt::rialto::EaseType::EASE_LINEAR};
constexpr bool kMute{false};
constexpr bool kResetTime{true};
constexpr double kAppliedRate{2.0};
constexpr uint64_t kRenderedFrames{987654};
constexpr uint64_t kDroppedFrames{321};
constexpr uint32_t kDuration{35};
constexpr int64_t kDiscontinuityGap{1};
constexpr bool kIsAudioAac{false};
const std::string kTextTrackIdentifier{"TextTrackIdentifier"};
constexpr uint32_t kBufferingLimit{4324};
constexpr bool kUseBuffering{true};
constexpr uint64_t kStopPosition{23412};
constexpr uint32_t kQueuedFrames{123};
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
    EXPECT_CALL(m_mediaPipelineMock, load(kType, kMimeType, kUrl)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToLoad()
{
    EXPECT_CALL(m_mediaPipelineMock, load(kType, kMimeType, kUrl)).WillOnce(Return(false));
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
    EXPECT_CALL(m_mediaPipelineMock, removeSource(kSourceId)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToRemoveSource()
{
    EXPECT_CALL(m_mediaPipelineMock, removeSource(kSourceId)).WillOnce(Return(false));
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
    EXPECT_CALL(m_mediaPipelineMock, play(_)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToPlay()
{
    EXPECT_CALL(m_mediaPipelineMock, play(_)).WillOnce(Return(false));
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
    EXPECT_CALL(m_mediaPipelineMock, setPlaybackRate(kRate)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetPlaybackRate()
{
    EXPECT_CALL(m_mediaPipelineMock, setPlaybackRate(kRate)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, setPosition(kPosition)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, setPosition(kPosition)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetVideoWindow()
{
    EXPECT_CALL(m_mediaPipelineMock, setVideoWindow(x, y, kWidth, kHeight)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetVideoWindow()
{
    EXPECT_CALL(m_mediaPipelineMock, setVideoWindow(x, y, kWidth, kHeight)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillHaveData()
{
    EXPECT_CALL(m_mediaPipelineMock, haveData(kStatus, kNumFrames, kNeedDataRequestId)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToHaveData()
{
    EXPECT_CALL(m_mediaPipelineMock, haveData(kStatus, kNumFrames, kNeedDataRequestId)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, getPosition(_))
        .WillOnce(Invoke(
            [&](int64_t &pos)
            {
                pos = kPosition;
                return true;
            }));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetPosition()
{
    EXPECT_CALL(m_mediaPipelineMock, getPosition(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetImmediateOutput()
{
    EXPECT_CALL(m_mediaPipelineMock, setImmediateOutput(_, _)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetImmediateOutput()
{
    EXPECT_CALL(m_mediaPipelineMock, setImmediateOutput(_, _)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetImmediateOutput()
{
    EXPECT_CALL(m_mediaPipelineMock, getImmediateOutput(_, _)).WillOnce(DoAll(SetArgReferee<1>(true), Return(true)));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetImmediateOutput()
{
    EXPECT_CALL(m_mediaPipelineMock, getImmediateOutput(_, _)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetReportDecodeErrors()
{
    EXPECT_CALL(m_mediaPipelineMock, setReportDecodeErrors(_, _)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetReportDecodeErrors()
{
    EXPECT_CALL(m_mediaPipelineMock, setReportDecodeErrors(_, _)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetQueuedFrames()
{
    EXPECT_CALL(m_mediaPipelineMock, getQueuedFrames(_, _)).WillOnce(DoAll(SetArgReferee<1>(123), Return(true)));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetQueuedFrames()
{
    EXPECT_CALL(m_mediaPipelineMock, getQueuedFrames(_, _)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetStats()
{
    EXPECT_CALL(m_mediaPipelineMock, getStats(_, _, _))
        .WillOnce(Invoke(
            [&](int32_t sourceId, uint64_t &renderedFrames, uint64_t &droppedFrames)
            {
                renderedFrames = kRenderedFrames;
                droppedFrames = kDroppedFrames;
                return true;
            }));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetStats()
{
    EXPECT_CALL(m_mediaPipelineMock, getStats(_, _, _)).WillOnce(Return(false));
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
    EXPECT_CALL(m_mediaPipelineMock, setVolume(_, _, _)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetVolume()
{
    EXPECT_CALL(m_mediaPipelineMock, setVolume(_, _, _)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetVolume()
{
    EXPECT_CALL(m_mediaPipelineMock, getVolume(_))
        .WillOnce(Invoke(
            [&](double &vol)
            {
                vol = kVolume;
                return true;
            }));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetVolume()
{
    EXPECT_CALL(m_mediaPipelineMock, getVolume(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetMute()
{
    EXPECT_CALL(m_mediaPipelineMock, setMute(kSourceId, _)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetMute()
{
    EXPECT_CALL(m_mediaPipelineMock, setMute(kSourceId, _)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetLowLatency()
{
    EXPECT_CALL(m_mediaPipelineMock, setLowLatency(_)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetLowLatency()
{
    EXPECT_CALL(m_mediaPipelineMock, setLowLatency(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetSync()
{
    EXPECT_CALL(m_mediaPipelineMock, setSync(_)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetSync()
{
    EXPECT_CALL(m_mediaPipelineMock, setSync(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetSync()
{
    EXPECT_CALL(m_mediaPipelineMock, getSync(_)).WillOnce(DoAll(SetArgReferee<0>(true), Return(true)));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetSync()
{
    EXPECT_CALL(m_mediaPipelineMock, getSync(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetSyncOff()
{
    EXPECT_CALL(m_mediaPipelineMock, setSyncOff(_)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetSyncOff()
{
    EXPECT_CALL(m_mediaPipelineMock, setSyncOff(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetStreamSyncMode()
{
    EXPECT_CALL(m_mediaPipelineMock, setStreamSyncMode(kSourceId, _)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetStreamSyncMode()
{
    EXPECT_CALL(m_mediaPipelineMock, setStreamSyncMode(kSourceId, _)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetStreamSyncMode()
{
    EXPECT_CALL(m_mediaPipelineMock, getStreamSyncMode(_)).WillOnce(DoAll(SetArgReferee<0>(1), Return(true)));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetStreamSyncMode()
{
    EXPECT_CALL(m_mediaPipelineMock, getStreamSyncMode(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetMute()
{
    EXPECT_CALL(m_mediaPipelineMock, getMute(kSourceId, _))
        .WillOnce(Invoke(
            [&](int32_t sourceId, bool &mut)
            {
                mut = kMute;
                return true;
            }));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetMute()
{
    EXPECT_CALL(m_mediaPipelineMock, getMute(kSourceId, _)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillFlush()
{
    EXPECT_CALL(m_mediaPipelineMock, flush(kSourceId, kResetTime, _)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToFlush()
{
    EXPECT_CALL(m_mediaPipelineMock, flush(kSourceId, kResetTime, _)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetSourcePosition()
{
    EXPECT_CALL(m_mediaPipelineMock, setSourcePosition(kSourceId, kPosition, kResetTime, kAppliedRate, kStopPosition))
        .WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetSourcePosition()
{
    EXPECT_CALL(m_mediaPipelineMock, setSourcePosition(kSourceId, kPosition, kResetTime, kAppliedRate, kStopPosition))
        .WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillProcessAudioGap()
{
    EXPECT_CALL(m_mediaPipelineMock, processAudioGap(kPosition, kDuration, kDiscontinuityGap, kIsAudioAac))
        .WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToProcessAudioGap()
{
    EXPECT_CALL(m_mediaPipelineMock, processAudioGap(kPosition, kDuration, kDiscontinuityGap, kIsAudioAac))
        .WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetTextTrackIdentifier()
{
    EXPECT_CALL(m_mediaPipelineMock, setTextTrackIdentifier(kTextTrackIdentifier)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetTextTrackIdentifier()
{
    EXPECT_CALL(m_mediaPipelineMock, setTextTrackIdentifier(kTextTrackIdentifier)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetTextTrackIdentifier()
{
    EXPECT_CALL(m_mediaPipelineMock, getTextTrackIdentifier(_))
        .WillOnce(DoAll(SetArgReferee<0>(kTextTrackIdentifier), Return(true)));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetTextTrackIdentifier()
{
    EXPECT_CALL(m_mediaPipelineMock, getTextTrackIdentifier(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetBufferingLimit()
{
    EXPECT_CALL(m_mediaPipelineMock, setBufferingLimit(kBufferingLimit)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetBufferingLimit()
{
    EXPECT_CALL(m_mediaPipelineMock, setBufferingLimit(kBufferingLimit)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetBufferingLimit()
{
    EXPECT_CALL(m_mediaPipelineMock, getBufferingLimit(_)).WillOnce(DoAll(SetArgReferee<0>(kBufferingLimit), Return(true)));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetBufferingLimit()
{
    EXPECT_CALL(m_mediaPipelineMock, getBufferingLimit(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSetUseBuffering()
{
    EXPECT_CALL(m_mediaPipelineMock, setUseBuffering(kUseBuffering)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSetUseBuffering()
{
    EXPECT_CALL(m_mediaPipelineMock, setUseBuffering(kUseBuffering)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillGetUseBuffering()
{
    EXPECT_CALL(m_mediaPipelineMock, getUseBuffering(_)).WillOnce(DoAll(SetArgReferee<0>(kUseBuffering), Return(true)));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToGetUseBuffering()
{
    EXPECT_CALL(m_mediaPipelineMock, getUseBuffering(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillSwitchSource()
{
    EXPECT_CALL(m_mediaPipelineMock, switchSource(_)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToSwitchSource()
{
    EXPECT_CALL(m_mediaPipelineMock, switchSource(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillCheckIfVideoIsMaster()
{
    EXPECT_CALL(m_mediaPipelineCapabilitiesMock, isVideoMaster(_)).WillOnce(Return(true));
}

void MediaPipelineServiceTests::mediaPipelineWillFailToCheckIfVideoIsMaster()
{
    EXPECT_CALL(m_mediaPipelineCapabilitiesMock, isVideoMaster(_)).WillOnce(Return(false));
}

void MediaPipelineServiceTests::mediaPipelineWillPing()
{
    EXPECT_CALL(*m_heartbeatProcedureMock, createHandler())
        .WillOnce(Return(ByMove(std::make_unique<StrictMock<firebolt::rialto::server::HeartbeatHandlerMock>>())));
    EXPECT_CALL(m_mediaPipelineMock, ping(_));
}

void MediaPipelineServiceTests::mediaPipelineFactoryWillCreateMediaPipeline()
{
    EXPECT_CALL(*m_mediaPipelineFactoryMock, createMediaPipelineServerInternal(_, kRequirements, _, _, _))
        .WillOnce(Return(ByMove(std::move(m_mediaPipeline))));
}

void MediaPipelineServiceTests::mediaPipelineFactoryWillReturnNullptr()
{
    EXPECT_CALL(*m_mediaPipelineFactoryMock, createMediaPipelineServerInternal(_, kRequirements, _, _, _))
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
        .WillOnce(Return(ByMove(std::unique_ptr<firebolt::rialto::IMediaPipelineCapabilities>())));
    EXPECT_THROW(m_sut =
                     std::make_unique<firebolt::rialto::server::service::MediaPipelineService>(m_playbackServiceMock,
                                                                                               m_mediaPipelineFactoryMock,
                                                                                               m_mediaPipelineCapabilitiesFactoryMock,
                                                                                               m_decryptionServiceMock),
                 std::runtime_error);
}

void MediaPipelineServiceTests::createSessionShouldSucceed()
{
    EXPECT_TRUE(m_sut->createSession(kSessionId, kMediaPipelineClient, kWidth, kHeight));
}

void MediaPipelineServiceTests::createSessionShouldFail()
{
    EXPECT_FALSE(m_sut->createSession(kSessionId, kMediaPipelineClient, kWidth, kHeight));
}

void MediaPipelineServiceTests::destroySessionShouldSucceed()
{
    EXPECT_TRUE(m_sut->destroySession(kSessionId));
}

void MediaPipelineServiceTests::destroySessionShouldFail()
{
    EXPECT_FALSE(m_sut->destroySession(kSessionId));
}

void MediaPipelineServiceTests::loadShouldSucceed()
{
    EXPECT_TRUE(m_sut->load(kSessionId, kType, kMimeType, kUrl));
}

void MediaPipelineServiceTests::loadShouldFail()
{
    EXPECT_FALSE(m_sut->load(kSessionId, kType, kMimeType, kUrl));
}

void MediaPipelineServiceTests::attachSourceShouldSucceed()
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264");
    EXPECT_TRUE(m_sut->attachSource(kSessionId, mediaSource));
}

void MediaPipelineServiceTests::attachSourceShouldFail()
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264");
    EXPECT_FALSE(m_sut->attachSource(kSessionId, mediaSource));
}

void MediaPipelineServiceTests::removeSourceShouldSucceed()
{
    EXPECT_TRUE(m_sut->removeSource(kSessionId, kSourceId));
}

void MediaPipelineServiceTests::removeSourceShouldFail()
{
    EXPECT_FALSE(m_sut->removeSource(kSessionId, kSourceId));
}

void MediaPipelineServiceTests::allSourcesAttachedShouldSucceed()
{
    EXPECT_TRUE(m_sut->allSourcesAttached(kSessionId));
}

void MediaPipelineServiceTests::allSourcesAttachedShouldFail()
{
    EXPECT_FALSE(m_sut->allSourcesAttached(kSessionId));
}

void MediaPipelineServiceTests::playShouldSucceed()
{
    bool isAsync{false};
    EXPECT_TRUE(m_sut->play(kSessionId, isAsync));
}

void MediaPipelineServiceTests::playShouldFail()
{
    bool isAsync{false};
    EXPECT_FALSE(m_sut->play(kSessionId, isAsync));
}

void MediaPipelineServiceTests::pauseShouldSucceed()
{
    EXPECT_TRUE(m_sut->pause(kSessionId));
}

void MediaPipelineServiceTests::pauseShouldFail()
{
    EXPECT_FALSE(m_sut->pause(kSessionId));
}

void MediaPipelineServiceTests::stopShouldSucceed()
{
    EXPECT_TRUE(m_sut->stop(kSessionId));
}

void MediaPipelineServiceTests::stopShouldFail()
{
    EXPECT_FALSE(m_sut->stop(kSessionId));
}

void MediaPipelineServiceTests::setPlaybackRateShouldSucceed()
{
    EXPECT_TRUE(m_sut->setPlaybackRate(kSessionId, kRate));
}

void MediaPipelineServiceTests::setPlaybackRateShouldFail()
{
    EXPECT_FALSE(m_sut->setPlaybackRate(kSessionId, kRate));
}

void MediaPipelineServiceTests::setPositionShouldSucceed()
{
    EXPECT_TRUE(m_sut->setPosition(kSessionId, kPosition));
}

void MediaPipelineServiceTests::setPositionShouldFail()
{
    EXPECT_FALSE(m_sut->setPosition(kSessionId, kPosition));
}

void MediaPipelineServiceTests::setVideoWindowShouldSucceed()
{
    EXPECT_TRUE(m_sut->setVideoWindow(kSessionId, x, y, kWidth, kHeight));
}

void MediaPipelineServiceTests::setVideoWindowShouldFail()
{
    EXPECT_FALSE(m_sut->setVideoWindow(kSessionId, x, y, kWidth, kHeight));
}

void MediaPipelineServiceTests::haveDataShouldSucceed()
{
    EXPECT_TRUE(m_sut->haveData(kSessionId, kStatus, kNumFrames, kNeedDataRequestId));
}

void MediaPipelineServiceTests::haveDataShouldFail()
{
    EXPECT_FALSE(m_sut->haveData(kSessionId, kStatus, kNumFrames, kNeedDataRequestId));
}

void MediaPipelineServiceTests::getPositionShouldSucceed()
{
    std::int64_t targetPosition{};
    EXPECT_TRUE(m_sut->getPosition(kSessionId, targetPosition));
    EXPECT_EQ(targetPosition, kPosition);
}

void MediaPipelineServiceTests::getPositionShouldFail()
{
    std::int64_t targetPosition{};
    EXPECT_FALSE(m_sut->getPosition(kSessionId, targetPosition));
}

void MediaPipelineServiceTests::getStatsShouldSucceed()
{
    std::uint64_t renderedFrames;
    std::uint64_t droppedFrames;
    EXPECT_TRUE(m_sut->getStats(kSessionId, kSourceId, renderedFrames, droppedFrames));
    EXPECT_EQ(renderedFrames, kRenderedFrames);
    EXPECT_EQ(droppedFrames, kDroppedFrames);
}

void MediaPipelineServiceTests::getStatsShouldFail()
{
    std::uint64_t renderedFrames;
    std::uint64_t droppedFrames;
    EXPECT_FALSE(m_sut->getStats(kSessionId, kSourceId, renderedFrames, droppedFrames));
}

void MediaPipelineServiceTests::setImmediateOutputShouldSucceed()
{
    EXPECT_TRUE(m_sut->setImmediateOutput(kSessionId, kSourceId, true));
}

void MediaPipelineServiceTests::setImmediateOutputShouldFail()
{
    EXPECT_FALSE(m_sut->setImmediateOutput(kSessionId, kSourceId, true));
}

void MediaPipelineServiceTests::getImmediateOutputShouldSucceed()
{
    bool immOp;
    EXPECT_TRUE(m_sut->getImmediateOutput(kSessionId, kSourceId, immOp));
    EXPECT_TRUE(immOp);
}

void MediaPipelineServiceTests::getImmediateOutputShouldFail()
{
    bool immOp;
    EXPECT_FALSE(m_sut->getImmediateOutput(kSessionId, kSourceId, immOp));
}

void MediaPipelineServiceTests::setReportDecodeErrorsShouldSucceed()
{
    EXPECT_TRUE(m_sut->setReportDecodeErrors(kSessionId, kSourceId, true));
}

void MediaPipelineServiceTests::setReportDecodeErrorsShouldFail()
{
    EXPECT_FALSE(m_sut->setReportDecodeErrors(kSessionId, kSourceId, true));
}

void MediaPipelineServiceTests::getQueuedFramesShouldSucceed()
{
    uint32_t queuedFr;
    EXPECT_TRUE(m_sut->getQueuedFrames(kSessionId, kSourceId, queuedFr));
    EXPECT_EQ(queuedFr, kQueuedFrames);
}

void MediaPipelineServiceTests::getQueuedFramesShouldFail()
{
    uint32_t queuedFr;
    EXPECT_FALSE(m_sut->getQueuedFrames(kSessionId, kSourceId, queuedFr));
}

void MediaPipelineServiceTests::getSupportedMimeTypesSucceed()
{
    MediaSourceType type = MediaSourceType::VIDEO;
    std::vector<std::string> mimeTypes = {"video/h264", "video/h265"};
    EXPECT_CALL(m_mediaPipelineCapabilitiesMock, getSupportedMimeTypes(type)).WillOnce(Return(mimeTypes));
    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), mimeTypes);
}

void MediaPipelineServiceTests::isMimeTypeSupportedSucceed()
{
    std::string mimeType = "video/h264";
    EXPECT_CALL(m_mediaPipelineCapabilitiesMock, isMimeTypeSupported(mimeType)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->isMimeTypeSupported(mimeType));
}

void MediaPipelineServiceTests::getSupportedPropertiesSucceed()
{
    const MediaSourceType kMediaType{MediaSourceType::VIDEO};
    const std::vector<std::string> kPropertyNames{"testing", "test-prop"};
    EXPECT_CALL(m_mediaPipelineCapabilitiesMock, getSupportedProperties(kMediaType, _)).WillOnce(Return(kPropertyNames));
    std::vector<std::string> supportedProperties{m_sut->getSupportedProperties(kMediaType, kPropertyNames)};
    EXPECT_EQ(supportedProperties, kPropertyNames);
}

void MediaPipelineServiceTests::renderFrameShouldSucceed()
{
    EXPECT_TRUE(m_sut->renderFrame(kSessionId));
}

void MediaPipelineServiceTests::renderFrameShouldFail()
{
    EXPECT_FALSE(m_sut->renderFrame(kSessionId));
}

void MediaPipelineServiceTests::setVolumeShouldSucceed()
{
    EXPECT_TRUE(m_sut->setVolume(kSessionId, kVolume, kVolumeDuration, kEaseType));
}

void MediaPipelineServiceTests::setVolumeShouldFail()
{
    EXPECT_FALSE(m_sut->setVolume(kSessionId, kVolume, kVolumeDuration, kEaseType));
}

void MediaPipelineServiceTests::getVolumeShouldSucceed()
{
    double currentVolume{};
    EXPECT_TRUE(m_sut->getVolume(kSessionId, currentVolume));
    EXPECT_EQ(currentVolume, kVolume);
}

void MediaPipelineServiceTests::getVolumeShouldFail()
{
    double currentVolume{};
    EXPECT_FALSE(m_sut->getVolume(kSessionId, currentVolume));
}

void MediaPipelineServiceTests::setMuteShouldSucceed()
{
    EXPECT_TRUE(m_sut->setMute(kSessionId, kSourceId, kMute));
}

void MediaPipelineServiceTests::setMuteShouldFail()
{
    EXPECT_FALSE(m_sut->setMute(kSessionId, kSourceId, kMute));
}

void MediaPipelineServiceTests::getMuteShouldSucceed()
{
    bool targetMute{};
    EXPECT_TRUE(m_sut->getMute(kSessionId, kSourceId, targetMute));
    EXPECT_EQ(targetMute, kMute);
}

void MediaPipelineServiceTests::getMuteShouldFail()
{
    bool targetMute{};
    EXPECT_FALSE(m_sut->getMute(kSessionId, kSourceId, targetMute));
}

void MediaPipelineServiceTests::setLowLatencyShouldSucceed()
{
    EXPECT_TRUE(m_sut->setLowLatency(kSessionId, true));
}

void MediaPipelineServiceTests::setLowLatencyShouldFail()
{
    EXPECT_FALSE(m_sut->setLowLatency(kSessionId, true));
}

void MediaPipelineServiceTests::setSyncShouldSucceed()
{
    EXPECT_TRUE(m_sut->setSync(kSessionId, true));
}

void MediaPipelineServiceTests::setSyncShouldFail()
{
    EXPECT_FALSE(m_sut->setSync(kSessionId, true));
}

void MediaPipelineServiceTests::getSyncShouldSucceed()
{
    bool sync;
    EXPECT_TRUE(m_sut->getSync(kSessionId, sync));
    EXPECT_TRUE(sync);
}

void MediaPipelineServiceTests::getSyncShouldFail()
{
    bool sync;
    EXPECT_FALSE(m_sut->getSync(kSessionId, sync));
}

void MediaPipelineServiceTests::setSyncOffShouldSucceed()
{
    EXPECT_TRUE(m_sut->setSyncOff(kSessionId, true));
}

void MediaPipelineServiceTests::setSyncOffShouldFail()
{
    EXPECT_FALSE(m_sut->setSyncOff(kSessionId, true));
}

void MediaPipelineServiceTests::setStreamSyncModeShouldSucceed()
{
    EXPECT_TRUE(m_sut->setStreamSyncMode(kSessionId, kSourceId, true));
}

void MediaPipelineServiceTests::setStreamSyncModeShouldFail()
{
    EXPECT_FALSE(m_sut->setStreamSyncMode(kSessionId, kSourceId, true));
}

void MediaPipelineServiceTests::getStreamSyncModeShouldSucceed()
{
    int32_t streamSyncMode;
    EXPECT_TRUE(m_sut->getStreamSyncMode(kSessionId, streamSyncMode));
    EXPECT_EQ(streamSyncMode, 1);
}

void MediaPipelineServiceTests::getStreamSyncModeShouldFail()
{
    int32_t streamSyncMode;
    EXPECT_FALSE(m_sut->getStreamSyncMode(kSessionId, streamSyncMode));
}

void MediaPipelineServiceTests::flushShouldSucceed()
{
    bool isAsync{false};
    EXPECT_TRUE(m_sut->flush(kSessionId, kSourceId, kResetTime, isAsync));
}

void MediaPipelineServiceTests::flushShouldFail()
{
    bool isAsync{false};
    EXPECT_FALSE(m_sut->flush(kSessionId, kSourceId, kResetTime, isAsync));
}

void MediaPipelineServiceTests::setSourcePositionShouldSucceed()
{
    EXPECT_TRUE(m_sut->setSourcePosition(kSessionId, kSourceId, kPosition, kResetTime, kAppliedRate, kStopPosition));
}

void MediaPipelineServiceTests::setSourcePositionShouldFail()
{
    EXPECT_FALSE(m_sut->setSourcePosition(kSessionId, kSourceId, kPosition, kResetTime, kAppliedRate, kStopPosition));
}

void MediaPipelineServiceTests::processAudioGapShouldSucceed()
{
    EXPECT_TRUE(m_sut->processAudioGap(kSessionId, kPosition, kDuration, kDiscontinuityGap, kIsAudioAac));
}

void MediaPipelineServiceTests::processAudioGapShouldFail()
{
    EXPECT_FALSE(m_sut->processAudioGap(kSessionId, kPosition, kDuration, kDiscontinuityGap, kIsAudioAac));
}

void MediaPipelineServiceTests::setTextTrackIdentifierShouldSucceed()
{
    EXPECT_TRUE(m_sut->setTextTrackIdentifier(kSessionId, kTextTrackIdentifier));
}

void MediaPipelineServiceTests::setTextTrackIdentifierShouldFail()
{
    EXPECT_FALSE(m_sut->setTextTrackIdentifier(kSessionId, kTextTrackIdentifier));
}

void MediaPipelineServiceTests::getTextTrackIdentifierShouldSucceed()
{
    std::string textTrackIdentifier;
    EXPECT_TRUE(m_sut->getTextTrackIdentifier(kSessionId, textTrackIdentifier));
    EXPECT_EQ(kTextTrackIdentifier, textTrackIdentifier);
}

void MediaPipelineServiceTests::getTextTrackIdentifierShouldFail()
{
    std::string textTrackIdentifier;
    EXPECT_FALSE(m_sut->getTextTrackIdentifier(kSessionId, textTrackIdentifier));
}

void MediaPipelineServiceTests::setBufferingLimitShouldSucceed()
{
    EXPECT_TRUE(m_sut->setBufferingLimit(kSessionId, kBufferingLimit));
}

void MediaPipelineServiceTests::setBufferingLimitShouldFail()
{
    EXPECT_FALSE(m_sut->setBufferingLimit(kSessionId, kBufferingLimit));
}

void MediaPipelineServiceTests::getBufferingLimitShouldSucceed()
{
    uint32_t bufferingLimit{0};
    EXPECT_TRUE(m_sut->getBufferingLimit(kSessionId, bufferingLimit));
    EXPECT_EQ(kBufferingLimit, bufferingLimit);
}

void MediaPipelineServiceTests::getBufferingLimitShouldFail()
{
    uint32_t bufferingLimit{0};
    EXPECT_FALSE(m_sut->getBufferingLimit(kSessionId, bufferingLimit));
}

void MediaPipelineServiceTests::setUseBufferingShouldSucceed()
{
    EXPECT_TRUE(m_sut->setUseBuffering(kSessionId, kUseBuffering));
}

void MediaPipelineServiceTests::setUseBufferingShouldFail()
{
    EXPECT_FALSE(m_sut->setUseBuffering(kSessionId, kUseBuffering));
}

void MediaPipelineServiceTests::getUseBufferingShouldSucceed()
{
    bool useBuffering{false};
    EXPECT_TRUE(m_sut->getUseBuffering(kSessionId, useBuffering));
    EXPECT_EQ(kUseBuffering, useBuffering);
}

void MediaPipelineServiceTests::getUseBufferingShouldFail()
{
    bool useBuffering{false};
    EXPECT_FALSE(m_sut->getUseBuffering(kSessionId, useBuffering));
}

void MediaPipelineServiceTests::switchSourceShouldSucceed()
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264");
    EXPECT_TRUE(m_sut->switchSource(kSessionId, mediaSource));
}

void MediaPipelineServiceTests::switchSourceShouldFail()
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264");
    EXPECT_FALSE(m_sut->switchSource(kSessionId, mediaSource));
}

void MediaPipelineServiceTests::isVideoMasterShouldSucceed()
{
    bool isMaster{false};
    EXPECT_TRUE(m_sut->isVideoMaster(isMaster));
}

void MediaPipelineServiceTests::isVideoMasterShouldFail()
{
    bool isMaster{false};
    EXPECT_FALSE(m_sut->isVideoMaster(isMaster));
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
