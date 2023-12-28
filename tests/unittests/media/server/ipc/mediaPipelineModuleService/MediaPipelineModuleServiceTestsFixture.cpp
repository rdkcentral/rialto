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

#include "MediaPipelineModuleServiceTestsFixture.h"
#include "MediaPipelineModuleService.h"
#include "MediaPipelineProtoUtils.h"
#include "MediaSourceUtil.h"
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <vector>

using testing::_;
using testing::ByRef;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SaveArg;
using testing::SetArgReferee;

namespace
{
constexpr firebolt::rialto::MediaSourceType kMediaSourceType{firebolt::rialto::MediaSourceType::AUDIO};
constexpr std::uint32_t kWidth{1920};
constexpr std::uint32_t kHeight{1080};
constexpr int kHardcodedSessionId{2};
const firebolt::rialto::MediaType kMediaType{firebolt::rialto::MediaType::MSE};
const std::string kMimeType{"exampleMimeType"};
constexpr uint32_t kDolbyProfile{5};
constexpr uint32_t kNumberOfChannels{6};
constexpr uint32_t kSampleRate{48000};
const std::string kCodecSpecificConfigStr("1243567");
const std::shared_ptr<firebolt::rialto::CodecData> kCodecData{std::make_shared<firebolt::rialto::CodecData>(
    firebolt::rialto::CodecData{std::vector<std::uint8_t>{'T', 'E', 'S', 'T'}, firebolt::rialto::CodecDataType::BUFFER})};
const std::string kUrl{"https://example.url.com"};
constexpr int64_t kPosition{2000000000};
constexpr std::uint32_t kRequestId{2};
const firebolt::rialto::MediaSourceStatus kMediaSourceStatus{firebolt::rialto::MediaSourceStatus::CODEC_CHANGED};
constexpr std::uint32_t kNumFrames{1};
constexpr int kX{30};
constexpr int kY{40};
constexpr std::int32_t kSourceId{12};
constexpr size_t kFrameCount{5};
constexpr std::uint32_t kMaxBytes{2};
constexpr std::uint32_t kNeedDataRequestId{32};
constexpr firebolt::rialto::MediaPlayerShmInfo kShmInfo{15, 16, 17};
constexpr firebolt::rialto::PlaybackState kPlaybackState{firebolt::rialto::PlaybackState::PLAYING};
constexpr firebolt::rialto::NetworkState kNetworkState{firebolt::rialto::NetworkState::BUFFERED};
constexpr firebolt::rialto::QosInfo kQosInfo{5u, 2u};
constexpr double kRate{1.5};
constexpr double kVolume{0.7};
constexpr bool kMute{false};
} // namespace

MATCHER_P(AttachedSourceMatcher, source, "")
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> &src = source.get();
    bool codecDataEqual = false;
    if (arg->getCodecData() && src->getCodecData())
    {
        codecDataEqual = arg->getCodecData()->data == src->getCodecData()->data &&
                         arg->getCodecData()->type == src->getCodecData()->type;
    }
    else
    {
        codecDataEqual = arg->getCodecData() == src->getCodecData();
    }

    bool baseCompare = arg->getConfigType() == src->getConfigType() && arg->getMimeType() == src->getMimeType() &&
                       arg->getHasDrm() == src->getHasDrm() && arg->getSegmentAlignment() == src->getSegmentAlignment() &&
                       codecDataEqual && arg->getStreamFormat() == src->getStreamFormat();

    bool extraCompare = true;

    if (arg->getConfigType() == firebolt::rialto::SourceConfigType::AUDIO)
    {
        firebolt::rialto::IMediaPipeline::MediaSourceAudio &audioArg =
            dynamic_cast<firebolt::rialto::IMediaPipeline::MediaSourceAudio &>(*arg);
        firebolt::rialto::IMediaPipeline::MediaSourceAudio &audioSrc =
            dynamic_cast<firebolt::rialto::IMediaPipeline::MediaSourceAudio &>(*src);

        extraCompare = audioArg.getStreamFormat() == audioSrc.getStreamFormat();
    }
    else if (arg->getConfigType() == firebolt::rialto::SourceConfigType::VIDEO)
    {
        firebolt::rialto::IMediaPipeline::MediaSourceVideo &videoArg =
            dynamic_cast<firebolt::rialto::IMediaPipeline::MediaSourceVideo &>(*arg);
        firebolt::rialto::IMediaPipeline::MediaSourceVideo &videoSrc =
            dynamic_cast<firebolt::rialto::IMediaPipeline::MediaSourceVideo &>(*src);

        extraCompare = videoArg.getWidth() == videoSrc.getWidth() && videoArg.getHeight() == videoSrc.getHeight();
    }
    else if (arg->getConfigType() == firebolt::rialto::SourceConfigType::VIDEO_DOLBY_VISION)
    {
        firebolt::rialto::IMediaPipeline::MediaSourceVideoDolbyVision &dolbyArg =
            dynamic_cast<firebolt::rialto::IMediaPipeline::MediaSourceVideoDolbyVision &>(*arg);
        firebolt::rialto::IMediaPipeline::MediaSourceVideoDolbyVision &dolbySrc =
            dynamic_cast<firebolt::rialto::IMediaPipeline::MediaSourceVideoDolbyVision &>(*src);

        extraCompare = dolbyArg.getDolbyVisionProfile() == dolbySrc.getDolbyVisionProfile() &&
                       dolbyArg.getWidth() == dolbySrc.getWidth() && dolbyArg.getHeight() == dolbySrc.getHeight();
    }

    return baseCompare && extraCompare;
}

MATCHER_P6(NeedMediaDataEventMatcher, sessionId, kSourceId, kNeedDataRequestId, kFrameCount, maxMediaBytes, kShmInfo, "")
{
    std::shared_ptr<firebolt::rialto::NeedMediaDataEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::NeedMediaDataEvent>(arg);

    return ((sessionId == event->session_id()) && (kSourceId == event->source_id()) &&
            (kNeedDataRequestId == event->request_id()) && (kFrameCount == event->frame_count()) &&
            (kShmInfo->maxMetadataBytes == event->shm_info().max_metadata_bytes()) &&
            (kShmInfo->metadataOffset == event->shm_info().metadata_offset()) &&
            (kShmInfo->mediaDataOffset == event->shm_info().media_data_offset()) &&
            (kShmInfo->maxMediaBytes == event->shm_info().max_media_bytes()));
}

MATCHER_P(PositionChangeEventMatcher, kPosition, "")
{
    std::shared_ptr<firebolt::rialto::PositionChangeEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::PositionChangeEvent>(arg);
    return (kPosition == event->position());
}

MATCHER_P3(QosEventMatcher, kSourceId, processed, dropped, "")
{
    std::shared_ptr<firebolt::rialto::QosEvent> event = std::dynamic_pointer_cast<firebolt::rialto::QosEvent>(arg);
    return ((kSourceId == event->source_id()) && (processed == event->qos_info().processed()) &&
            (dropped == event->qos_info().dropped()));
}

MATCHER_P(PlaybackStateChangeEventMatcher, kPlaybackState, "")
{
    std::shared_ptr<firebolt::rialto::PlaybackStateChangeEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::PlaybackStateChangeEvent>(arg);
    return (kPlaybackState == event->state());
}

MATCHER_P(NetworkStateChangeEventMatcher, kNetworkState, "")
{
    std::shared_ptr<firebolt::rialto::NetworkStateChangeEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::NetworkStateChangeEvent>(arg);
    return (kNetworkState == event->state());
}

MediaPipelineModuleServiceTests::MediaPipelineModuleServiceTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_serverMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ServerMock>>()},
      m_closureMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClosureMock>>()},
      m_controllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ControllerMock>>()}
{
    m_service = std::make_shared<firebolt::rialto::server::ipc::MediaPipelineModuleService>(m_mediaPipelineServiceMock);
}

MediaPipelineModuleServiceTests::~MediaPipelineModuleServiceTests() {}

void MediaPipelineModuleServiceTests::clientWillConnect()
{
    EXPECT_CALL(*m_clientMock, exportService(_));
}

void MediaPipelineModuleServiceTests::clientWillDisconnect(int sessionId)
{
    EXPECT_CALL(m_mediaPipelineServiceMock, destroySession(sessionId));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillCreateSession()
{
    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).Times(2).WillRepeatedly(Return(m_clientMock));
    EXPECT_CALL(m_mediaPipelineServiceMock, createSession(_, _, kWidth, kHeight))
        .WillOnce(DoAll(SaveArg<1>(&m_mediaPipelineClient), Return(true)));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToCreateSession()
{
    expectRequestFailure();
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_mediaPipelineServiceMock, createSession(_, _, kWidth, kHeight)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillDestroySession()
{
    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_mediaPipelineServiceMock, destroySession(kHardcodedSessionId)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToDestroySession()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, destroySession(kHardcodedSessionId)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillLoadSession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, load(kHardcodedSessionId, kMediaType, kMimeType, kUrl)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToLoadSession()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, load(kHardcodedSessionId, kMediaType, kMimeType, kUrl)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillAttachSource()
{
    m_source = std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>(kMimeType);
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, attachSource(kHardcodedSessionId, AttachedSourceMatcher(ByRef(m_source))))
        .WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillAttachVideoSource()
{
    m_source = std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>(kMimeType, true, kWidth, kHeight);
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, attachSource(kHardcodedSessionId, AttachedSourceMatcher(ByRef(m_source))))
        .WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillAttachDolbySource()
{
    m_source = std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideoDolbyVision>(kMimeType, kDolbyProfile,
                                                                                               true, kWidth, kHeight);
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, attachSource(kHardcodedSessionId, AttachedSourceMatcher(ByRef(m_source))))
        .WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillAttachAudioSourceWithAdditionaldata()
{
    std::vector<uint8_t> codecSpecificConfig;
    codecSpecificConfig.assign(kCodecSpecificConfigStr.begin(), kCodecSpecificConfigStr.end());
    firebolt::rialto::AudioConfig audioConfig{kNumberOfChannels, kSampleRate, codecSpecificConfig};
    m_source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>(kMimeType, true, audioConfig,
                                                                             firebolt::rialto::SegmentAlignment::UNDEFINED,
                                                                             firebolt::rialto::StreamFormat::RAW,
                                                                             kCodecData);
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, attachSource(kHardcodedSessionId, AttachedSourceMatcher(ByRef(m_source))))
        .WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToAttachSource()
{
    m_source = std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>(kMimeType);
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, attachSource(kHardcodedSessionId, AttachedSourceMatcher(ByRef(m_source))))
        .WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillSucceedAllSourcesAttached()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, allSourcesAttached(kHardcodedSessionId)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailAllSourcesAttached()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, allSourcesAttached(kHardcodedSessionId)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillPlay()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, play(kHardcodedSessionId)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToPlay()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, play(kHardcodedSessionId)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillPause()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, pause(kHardcodedSessionId)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToPause()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, pause(kHardcodedSessionId)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillStop()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, stop(kHardcodedSessionId)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToStop()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, stop(kHardcodedSessionId)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillSetPosition()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, setPosition(kHardcodedSessionId, kPosition)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToSetPosition()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, setPosition(kHardcodedSessionId, kPosition)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillSetVideoWindow()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, setVideoWindow(kHardcodedSessionId, kX, kY, kWidth, kHeight))
        .WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToSetVideoWindow()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, setVideoWindow(kHardcodedSessionId, kX, kY, kWidth, kHeight))
        .WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillHaveData()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, haveData(kHardcodedSessionId, kMediaSourceStatus, kNumFrames, kRequestId))
        .WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToHaveData()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, haveData(kHardcodedSessionId, kMediaSourceStatus, kNumFrames, kRequestId))
        .WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillSetPlaybackRate()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, setPlaybackRate(kHardcodedSessionId, kRate)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToSetPlaybackRate()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, setPlaybackRate(kHardcodedSessionId, kRate)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillGetPosition()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, getPosition(kHardcodedSessionId, _))
        .WillOnce(Invoke(
            [&](int, std::int64_t &pos)
            {
                pos = kPosition;
                return true;
            }));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToGetPosition()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, getPosition(kHardcodedSessionId, _)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillRenderFrame()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, renderFrame(kHardcodedSessionId)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToRenderFrame()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, renderFrame(kHardcodedSessionId)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillSetVolume()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, setVolume(kHardcodedSessionId, kVolume)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToSetVolume()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, setVolume(kHardcodedSessionId, kVolume)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillGetVolume()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, getVolume(kHardcodedSessionId, _))
        .WillOnce(Invoke(
            [&](int, double &vol)
            {
                vol = kVolume;
                return true;
            }));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToGetVolume()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, getVolume(kHardcodedSessionId, _)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillSetMute()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, setMute(kHardcodedSessionId, kMute)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToSetMute()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, setMute(kHardcodedSessionId, kMute)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillGetMute()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, getMute(kHardcodedSessionId, _))
        .WillOnce(Invoke(
            [&](int, bool &mut)
            {
                mut = kMute;
                return true;
            }));
}

void MediaPipelineModuleServiceTests::mediaPipelineServiceWillFailToGetMute()
{
    expectRequestFailure();
    EXPECT_CALL(m_mediaPipelineServiceMock, getMute(kHardcodedSessionId, _)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaClientWillSendPlaybackStateChangedEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(PlaybackStateChangeEventMatcher(convertPlaybackState(kPlaybackState))));
}

void MediaPipelineModuleServiceTests::mediaClientWillSendNetworkStateChangedEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(NetworkStateChangeEventMatcher(convertNetworkState(kNetworkState))));
}

void MediaPipelineModuleServiceTests::mediaClientWillSendNeedMediaDataEvent(int sessionId)
{
    std::shared_ptr<firebolt::rialto::MediaPlayerShmInfo> shmInfoPtr{
        std::make_shared<firebolt::rialto::MediaPlayerShmInfo>(kShmInfo)};

    EXPECT_CALL(*m_clientMock, sendEvent(NeedMediaDataEventMatcher(sessionId, kSourceId, kNeedDataRequestId,
                                                                   kFrameCount, kMaxBytes, shmInfoPtr)));
}

void MediaPipelineModuleServiceTests::mediaClientWillSendPostionChangeEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(PositionChangeEventMatcher(kPosition)));
}

void MediaPipelineModuleServiceTests::mediaClientWillSendQosEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(QosEventMatcher(kSourceId, kQosInfo.processed, kQosInfo.dropped)));
}

void MediaPipelineModuleServiceTests::sendClientConnected()
{
    m_service->clientConnected(m_clientMock);
}

void MediaPipelineModuleServiceTests::sendClientDisconnected()
{
    m_service->clientDisconnected(m_clientMock);
}

int MediaPipelineModuleServiceTests::sendCreateSessionRequestAndReceiveResponse()
{
    firebolt::rialto::CreateSessionRequest request;
    firebolt::rialto::CreateSessionResponse response;

    // Set an invalid sessionId in the response
    response.set_session_id(-1);

    request.set_max_width(kWidth);
    request.set_max_height(kHeight);

    m_service->createSession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_GE(response.session_id(), 0);

    return response.session_id();
}

void MediaPipelineModuleServiceTests::sendCreateSessionRequestAndExpectFailure()
{
    firebolt::rialto::CreateSessionRequest request;
    firebolt::rialto::CreateSessionResponse response;

    request.set_max_width(kWidth);
    request.set_max_height(kHeight);

    m_service->createSession(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendDestroySessionRequestAndReceiveResponse()
{
    firebolt::rialto::DestroySessionRequest request;
    firebolt::rialto::DestroySessionResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->destroySession(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendLoadRequestAndReceiveResponse()
{
    firebolt::rialto::LoadRequest request;
    firebolt::rialto::LoadResponse response;

    request.set_session_id(kHardcodedSessionId);
    request.set_type(convertMediaType(kMediaType));
    request.set_mime_type(kMimeType);
    request.set_url(kUrl);

    m_service->load(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendAttachSourceRequestAndReceiveResponse()
{
    firebolt::rialto::AttachSourceRequest request;
    firebolt::rialto::AttachSourceResponse response;

    request.set_session_id(kHardcodedSessionId);
    request.set_config_type(firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_AUDIO);
    request.set_mime_type(kMimeType);
    request.set_has_drm(true);

    m_service->attachSource(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendAttachVideoSourceRequestAndReceiveResponse()
{
    firebolt::rialto::AttachSourceRequest request;
    firebolt::rialto::AttachSourceResponse response;

    request.set_session_id(kHardcodedSessionId);
    request.set_config_type(firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_VIDEO);
    request.set_mime_type(kMimeType);
    request.set_has_drm(true);
    request.set_width(kWidth);
    request.set_height(kHeight);

    m_service->attachSource(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendAttachDolbySourceRequestAndReceiveResponse()
{
    firebolt::rialto::AttachSourceRequest request;
    firebolt::rialto::AttachSourceResponse response;

    request.set_session_id(kHardcodedSessionId);
    request.set_config_type(firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_VIDEO_DOLBY_VISION);
    request.set_mime_type(kMimeType);
    request.set_has_drm(true);
    request.set_width(kWidth);
    request.set_height(kHeight);
    request.set_dolby_vision_profile(kDolbyProfile);

    m_service->attachSource(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendAttachAudioSourceWithAdditionalDataRequestAndReceiveResponse()
{
    firebolt::rialto::AttachSourceRequest request;
    firebolt::rialto::AttachSourceResponse response;

    request.set_session_id(kHardcodedSessionId);
    request.set_config_type(firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_AUDIO);
    request.set_mime_type(kMimeType);
    request.set_has_drm(true);
    request.mutable_audio_config()->set_number_of_channels(kNumberOfChannels);
    request.mutable_audio_config()->set_sample_rate(kSampleRate);
    request.mutable_audio_config()->set_codec_specific_config(kCodecSpecificConfigStr);
    request.mutable_codec_data()->set_data(kCodecData->data.data(), kCodecData->data.size());
    request.mutable_codec_data()->set_type(firebolt::rialto::AttachSourceRequest_CodecData_Type_BUFFER);
    request.set_stream_format(convertStreamFormat(firebolt::rialto::StreamFormat::RAW));

    m_service->attachSource(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendAllSourcesAttachedRequestAndReceiveResponse()
{
    firebolt::rialto::AllSourcesAttachedRequest request;
    firebolt::rialto::AllSourcesAttachedResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->allSourcesAttached(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendPlayRequestAndReceiveResponse()
{
    firebolt::rialto::PlayRequest request;
    firebolt::rialto::PlayResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->play(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendPauseRequestAndReceiveResponse()
{
    firebolt::rialto::PauseRequest request;
    firebolt::rialto::PauseResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->pause(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendStopRequestAndReceiveResponse()
{
    firebolt::rialto::StopRequest request;
    firebolt::rialto::StopResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->stop(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendSetPositionRequestAndReceiveResponse()
{
    firebolt::rialto::SetPositionRequest request;
    firebolt::rialto::SetPositionResponse response;

    request.set_session_id(kHardcodedSessionId);
    request.set_position(kPosition);

    m_service->setPosition(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendGetPositionRequestAndReceiveResponse()
{
    firebolt::rialto::GetPositionRequest request;
    firebolt::rialto::GetPositionResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->getPosition(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ(response.position(), kPosition);
}

void MediaPipelineModuleServiceTests::sendGetPositionRequestAndReceiveResponseWithoutPositionMatch()
{
    firebolt::rialto::GetPositionRequest request;
    firebolt::rialto::GetPositionResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->getPosition(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendHaveDataRequestAndReceiveResponse()
{
    firebolt::rialto::HaveDataRequest request;
    firebolt::rialto::HaveDataResponse response;

    request.set_session_id(kHardcodedSessionId);
    request.set_status(convertMediaSourceStatus(kMediaSourceStatus));
    request.set_num_frames(kNumFrames);
    request.set_request_id(kRequestId);

    m_service->haveData(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendSetPlaybackRateRequestAndReceiveResponse()
{
    firebolt::rialto::SetPlaybackRateRequest request;
    firebolt::rialto::SetPlaybackRateResponse response;

    request.set_session_id(kHardcodedSessionId);
    request.set_rate(kRate);

    m_service->setPlaybackRate(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendSetVideoWindowRequestAndReceiveResponse()
{
    firebolt::rialto::SetVideoWindowRequest request;
    firebolt::rialto::SetVideoWindowResponse response;

    request.set_session_id(kHardcodedSessionId);
    request.set_x(kX);
    request.set_y(kY);
    request.set_width(kWidth);
    request.set_height(kHeight);

    m_service->setVideoWindow(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendSetVolumeRequestAndReceiveResponse()
{
    firebolt::rialto::SetVolumeRequest request;
    firebolt::rialto::SetVolumeResponse response;

    request.set_session_id(kHardcodedSessionId);
    request.set_volume(kVolume);

    m_service->setVolume(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendGetVolumeRequestAndReceiveResponse()
{
    firebolt::rialto::GetVolumeRequest request;
    firebolt::rialto::GetVolumeResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->getVolume(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ(response.volume(), kVolume);
}

void MediaPipelineModuleServiceTests::sendGetVolumeRequestAndReceiveResponseWithoutVolumeMatch()
{
    firebolt::rialto::GetVolumeRequest request;
    firebolt::rialto::GetVolumeResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->getVolume(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendSetMuteRequestAndReceiveResponse()
{
    firebolt::rialto::SetMuteRequest request;
    firebolt::rialto::SetMuteResponse response;

    request.set_session_id(kHardcodedSessionId);
    request.set_mute(kMute);

    m_service->setMute(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendGetMuteRequestAndReceiveResponse()
{
    firebolt::rialto::GetMuteRequest request;
    firebolt::rialto::GetMuteResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->getMute(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ(response.mute(), kMute);
}

void MediaPipelineModuleServiceTests::sendGetMuteRequestAndReceiveResponseWithoutMuteMatch()
{
    firebolt::rialto::GetMuteRequest request;
    firebolt::rialto::GetMuteResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->getMute(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendPlaybackStateChangedEvent()
{
    ASSERT_TRUE(m_mediaPipelineClient);
    m_mediaPipelineClient->notifyPlaybackState(kPlaybackState);
}

void MediaPipelineModuleServiceTests::sendNetworkStateChangedEvent()
{
    ASSERT_TRUE(m_mediaPipelineClient);
    m_mediaPipelineClient->notifyNetworkState(kNetworkState);
}

void MediaPipelineModuleServiceTests::sendNeedMediaDataEvent()
{
    ASSERT_TRUE(m_mediaPipelineClient);
    std::shared_ptr<firebolt::rialto::MediaPlayerShmInfo> shmInfoPtr{
        std::make_shared<firebolt::rialto::MediaPlayerShmInfo>(kShmInfo)};
    m_mediaPipelineClient->notifyNeedMediaData(kSourceId, kFrameCount, kNeedDataRequestId, shmInfoPtr);
}

void MediaPipelineModuleServiceTests::sendPostionChangeEvent()
{
    ASSERT_TRUE(m_mediaPipelineClient);
    m_mediaPipelineClient->notifyPosition(kPosition);
}

void MediaPipelineModuleServiceTests::sendQosEvent()
{
    ASSERT_TRUE(m_mediaPipelineClient);
    m_mediaPipelineClient->notifyQos(kSourceId, kQosInfo);
}

void MediaPipelineModuleServiceTests::expectRequestSuccess()
{
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaPipelineModuleServiceTests::expectRequestFailure()
{
    EXPECT_CALL(*m_controllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaPipelineModuleServiceTests::sendRenderFrameRequestAndReceiveResponse()
{
    firebolt::rialto::RenderFrameRequest request;
    firebolt::rialto::RenderFrameResponse response;

    request.set_session_id(kHardcodedSessionId);

    m_service->renderFrame(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::testFactoryCreatesObject()
{
    std::shared_ptr<firebolt::rialto::server::ipc::IMediaPipelineModuleServiceFactory> factory =
        firebolt::rialto::server::ipc::IMediaPipelineModuleServiceFactory::createFactory();
    EXPECT_NE(factory, nullptr);
    EXPECT_NE(factory->create(m_mediaPipelineServiceMock), nullptr);
}
