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
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

TEST_F(MediaPipelineModuleServiceTests, shouldConnectClient)
{
    clientWillConnect();
    sendClientConnected();
}

/**
 * Test the factory
 */
TEST_F(MediaPipelineModuleServiceTests, FactoryCreatesObject)
{
    testFactoryCreatesObject();
}

TEST_F(MediaPipelineModuleServiceTests, shouldCreateSession)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToCreateSession)
{
    mediaPipelineServiceWillFailToCreateSession();
    sendCreateSessionRequestAndExpectFailure();
}

TEST_F(MediaPipelineModuleServiceTests, shouldDestroySessionWhenDisconnectClient)
{
    clientWillConnect();
    sendClientConnected();
    mediaPipelineServiceWillCreateSession();
    int sessionId = sendCreateSessionRequestAndReceiveResponse();
    clientWillDisconnect(sessionId);
    sendClientDisconnected();
}

TEST_F(MediaPipelineModuleServiceTests, shouldDestroySession)
{
    mediaPipelineServiceWillDestroySession();
    sendDestroySessionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToDestroySession)
{
    mediaPipelineServiceWillFailToDestroySession();
    sendDestroySessionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldLoadSession)
{
    mediaPipelineServiceWillLoadSession();
    sendLoadRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToLoadSession)
{
    mediaPipelineServiceWillFailToLoadSession();
    sendLoadRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldAttachSource)
{
    mediaPipelineServiceWillAttachSource();
    sendAttachSourceRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldAttachVideoSource)
{
    mediaPipelineServiceWillAttachVideoSource();
    sendAttachVideoSourceRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldAttachDolbySource)
{
    mediaPipelineServiceWillAttachDolbySource();
    sendAttachDolbySourceRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldAttachSubtitleSource)
{
    mediaPipelineServiceWillAttachSubtitleSource();
    sendAttachSubtitleSourceRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailAttachUnknownSource)
{
    mediaPipelineServiceWillFailToAttachUnknownSource();
    sendAttachUnknownSourceRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldAttachAudioSourceWithAdditionalData)
{
    mediaPipelineServiceWillAttachAudioSourceWithAdditionaldata();
    sendAttachAudioSourceWithAdditionalDataRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSwitchSource)
{
    mediaPipelineServiceWillSwitchSource();
    sendAttachSourceRequestWithSwitchSourceAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSwitchSource)
{
    mediaPipelineServiceWillFailToSwitchSource();
    sendAttachSourceRequestWithSwitchSourceAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToAttachSource)
{
    mediaPipelineServiceWillFailToAttachSource();
    sendAttachSourceRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSucceedAllSourcesAttached)
{
    mediaPipelineServiceWillSucceedAllSourcesAttached();
    sendAllSourcesAttachedRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailAllSourcesAttached)
{
    mediaPipelineServiceWillFailAllSourcesAttached();
    sendAllSourcesAttachedRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldPlay)
{
    mediaPipelineServiceWillPlay();
    sendPlayRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToPlay)
{
    mediaPipelineServiceWillFailToPlay();
    sendPlayRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldPause)
{
    mediaPipelineServiceWillPause();
    sendPauseRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToPause)
{
    mediaPipelineServiceWillFailToPause();
    sendPauseRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldStop)
{
    mediaPipelineServiceWillStop();
    sendStopRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToStop)
{
    mediaPipelineServiceWillFailToStop();
    sendStopRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetPosition)
{
    mediaPipelineServiceWillSetPosition();
    sendSetPositionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetPosition)
{
    mediaPipelineServiceWillFailToSetPosition();
    sendSetPositionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldHaveData)
{
    mediaPipelineServiceWillHaveData();
    sendHaveDataRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailWhenHaveDataIsReceived)
{
    mediaPipelineServiceWillFailToHaveData();
    sendHaveDataRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetPlaybackRate)
{
    mediaPipelineServiceWillSetPlaybackRate();
    sendSetPlaybackRateRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailWhenSetPlaybackRateIsReceived)
{
    mediaPipelineServiceWillFailToSetPlaybackRate();
    sendSetPlaybackRateRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetVideoWindow)
{
    mediaPipelineServiceWillSetVideoWindow();
    sendSetVideoWindowRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetVideoWindow)
{
    mediaPipelineServiceWillFailToSetVideoWindow();
    sendSetVideoWindowRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetPosition)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillGetPosition();
    sendGetPositionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetPosition)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillFailToGetPosition();
    sendGetPositionRequestAndReceiveResponseWithoutPositionMatch();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetDuration)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillGetDuration();
    sendGetDurationRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetDuration)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillFailToGetDuration();
    sendGetDurationRequestAndReceiveResponseWithoutDurationMatch();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetImmediateOutput)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillSetImmediateOutput();
    sendSetImmediateOutputRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetImmediateOutputForAudioSourceInSeparateSession)
{
    mediaPipelineServiceWillCreateSession();
    const int videoSessionId = sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillCreateSession();
    const int audioSessionId = sendCreateSessionRequestAndReceiveResponse();

    constexpr int32_t kVideoSourceId{21};
    constexpr int32_t kAudioSourceId{22};
    sendAttachSourceRequestAndReceiveResponse(videoSessionId, firebolt::rialto::MediaSourceType::VIDEO,
                                              kVideoSourceId);
    sendAttachSourceRequestAndReceiveResponse(audioSessionId, firebolt::rialto::MediaSourceType::AUDIO,
                                              kAudioSourceId);

    mediaPipelineServiceWillSetImmediateOutputForVideoOnly(videoSessionId, kVideoSourceId);
    EXPECT_CALL(m_mediaPipelineServiceMock, setImmediateOutput(audioSessionId, kAudioSourceId, kImmediateOutputVal1))
        .WillOnce(Return(true));
    sendSetImmediateOutputRequestAndReceiveResponse(videoSessionId, kVideoSourceId);
}

TEST_F(MediaPipelineModuleServiceTests, shouldApplyPendingImmediateOutputWhenPeerAudioSourceAttachesLater)
{
    mediaPipelineServiceWillCreateSession();
    const int videoSessionId = sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillCreateSession();
    const int audioSessionId = sendCreateSessionRequestAndReceiveResponse();

    constexpr int32_t kVideoSourceId{28};
    constexpr int32_t kAudioSourceId{29};
    sendAttachSourceRequestAndReceiveResponse(videoSessionId, firebolt::rialto::MediaSourceType::VIDEO,
                                              kVideoSourceId);

    mediaPipelineServiceWillSetImmediateOutputForVideoOnly(videoSessionId, kVideoSourceId, kImmediateOutputVal2);
    sendSetImmediateOutputRequestAndReceiveResponse(videoSessionId, kVideoSourceId, kImmediateOutputVal2);

    EXPECT_CALL(m_mediaPipelineServiceMock, setImmediateOutput(audioSessionId, kAudioSourceId, kImmediateOutputVal2))
        .WillOnce(Return(true));
    sendAttachSourceRequestAndReceiveResponse(audioSessionId, firebolt::rialto::MediaSourceType::AUDIO,
                                              kAudioSourceId);
}

TEST_F(MediaPipelineModuleServiceTests, shouldNotGuessAudioSessionWhenMultiplePeerAudioSessionsExist)
{
    mediaPipelineServiceWillCreateSession();
    const int videoSessionId = sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillCreateSession();
    const int audioSessionId1 = sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillCreateSession();
    const int audioSessionId2 = sendCreateSessionRequestAndReceiveResponse();

    constexpr int32_t kVideoSourceId{23};
    constexpr int32_t kAudioSourceId1{24};
    constexpr int32_t kAudioSourceId2{27};
    sendAttachSourceRequestAndReceiveResponse(videoSessionId, firebolt::rialto::MediaSourceType::VIDEO,
                                              kVideoSourceId);
    sendAttachSourceRequestAndReceiveResponse(audioSessionId1, firebolt::rialto::MediaSourceType::AUDIO,
                                              kAudioSourceId1);
    sendAttachSourceRequestAndReceiveResponse(audioSessionId2, firebolt::rialto::MediaSourceType::AUDIO,
                                              kAudioSourceId2);

    mediaPipelineServiceWillSetImmediateOutputForVideoOnly(videoSessionId, kVideoSourceId);
    sendSetImmediateOutputRequestAndReceiveResponse(videoSessionId, kVideoSourceId);
}

TEST_F(MediaPipelineModuleServiceTests, shouldApplyPendingImmediateOutputWhenAudioSourceAttachesLater)
{
    mediaPipelineServiceWillCreateSession();
    const int sessionId = sendCreateSessionRequestAndReceiveResponse();
    constexpr int32_t kVideoSourceId{31};
    sendAttachSourceRequestAndReceiveResponse(sessionId, firebolt::rialto::MediaSourceType::VIDEO,
                                              kVideoSourceId);

    mediaPipelineServiceWillSetImmediateOutputForVideoOnly(sessionId, kVideoSourceId, kImmediateOutputVal2);
    sendSetImmediateOutputRequestAndReceiveResponse(sessionId, kVideoSourceId, kImmediateOutputVal2);

    constexpr int32_t kAudioSourceId{32};
    EXPECT_CALL(m_mediaPipelineServiceMock, setImmediateOutput(sessionId, kAudioSourceId, kImmediateOutputVal2))
        .WillOnce(Return(true));
    sendAttachSourceRequestAndReceiveResponse(sessionId, firebolt::rialto::MediaSourceType::AUDIO,
                                              kAudioSourceId);
}

TEST_F(MediaPipelineModuleServiceTests, shouldPreserveTrackedSourceIdWhenSwitchingAudioSource)
{
    mediaPipelineServiceWillCreateSession();
    const int sessionId = sendCreateSessionRequestAndReceiveResponse();
    constexpr int32_t kVideoSourceId{33};
    constexpr int32_t kAudioSourceId{34};
    sendAttachSourceRequestAndReceiveResponse(sessionId, firebolt::rialto::MediaSourceType::VIDEO,
                                              kVideoSourceId);
    sendAttachSourceRequestAndReceiveResponse(sessionId, firebolt::rialto::MediaSourceType::AUDIO,
                                              kAudioSourceId);

    m_source = std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>(kMimeType);
    firebolt::rialto::AttachSourceRequest switchRequest;
    firebolt::rialto::AttachSourceResponse switchResponse;
    switchRequest.set_session_id(sessionId);
    switchRequest.set_config_type(firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_AUDIO);
    switchRequest.set_mime_type(kMimeType);
    switchRequest.set_has_drm(true);
    switchRequest.set_switch_source(true);
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, switchSource(sessionId, AttachedSourceMatcher(ByRef(m_source))))
        .WillOnce(Invoke(
            [kAudioSourceId](int, const std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> &source)
            {
                EXPECT_EQ(source->getId(), kAudioSourceId);
                return true;
            }));
    m_service->attachSource(m_controllerMock.get(), &switchRequest, &switchResponse, m_closureMock.get());

    mediaPipelineServiceWillSetImmediateOutputForVideoOnly(sessionId, kVideoSourceId);
    EXPECT_CALL(m_mediaPipelineServiceMock, setImmediateOutput(sessionId, kAudioSourceId, kImmediateOutputVal1))
        .WillOnce(Return(true));
    sendSetImmediateOutputRequestAndReceiveResponse(sessionId, kVideoSourceId);
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetVideoImmediateOutputWhenMultipleAudioSourcesExist)
{
    mediaPipelineServiceWillCreateSession();
    const int sessionId = sendCreateSessionRequestAndReceiveResponse();

    constexpr int32_t kVideoSourceId{41};
    constexpr int32_t kAudioSourceId1{42};
    constexpr int32_t kAudioSourceId2{43};
    sendAttachSourceRequestAndReceiveResponse(sessionId, firebolt::rialto::MediaSourceType::VIDEO,
                                              kVideoSourceId);
    sendAttachSourceRequestAndReceiveResponse(sessionId, firebolt::rialto::MediaSourceType::AUDIO,
                                              kAudioSourceId1);
    sendAttachSourceRequestAndReceiveResponse(sessionId, firebolt::rialto::MediaSourceType::AUDIO,
                                              kAudioSourceId2);

    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_mediaPipelineServiceMock, setImmediateOutput(sessionId, kVideoSourceId, kImmediateOutputVal1))
        .WillOnce(Return(true));
    EXPECT_CALL(m_mediaPipelineServiceMock, setImmediateOutput(sessionId, kAudioSourceId1, kImmediateOutputVal1))
        .WillOnce(Return(true));
    EXPECT_CALL(m_mediaPipelineServiceMock, setImmediateOutput(sessionId, kAudioSourceId2, kImmediateOutputVal1))
        .WillOnce(Return(true));
    sendSetImmediateOutputRequestAndReceiveResponse(sessionId, kVideoSourceId);
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetImmediateOutput)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillFailToSetImmediateOutput();
    sendSetImmediateOutputRequestAndReceiveFail();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetImmediateOutput)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillGetImmediateOutput();
    sendGetImmediateOutputRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetImmediateOutput)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillFailToGetImmediateOutput();
    sendGetImmediateOutputRequestAndReceiveFail();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetReportDecodeErrors)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillSetReportDecodeErrors();
    sendSetReportDecodeErrorsRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetReportDecodeErrors)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillFailToSetReportDecodeErrors();
    sendSetReportDecodeErrorsRequestAndReceiveFail();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetQueuedFrames)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillGetQueuedFrames();
    sendGetQueuedFramesRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetQueuedFrames)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillFailToGetQueuedFrames();
    sendGetQueuedFramesRequestAndReceiveFail();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetStats)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillGetStats();
    sendGetStatsRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetStats)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillFailToGetStats();
    sendGetStatsRequestAndReceiveResponseWithoutStatsMatch();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendPlaybackStateChangedEvent)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendPlaybackStateChangedEvent();
    sendPlaybackStateChangedEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendNetworkStateChangedEvent)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendNetworkStateChangedEvent();
    sendNetworkStateChangedEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendNeedMediaDataEvent)
{
    mediaPipelineServiceWillCreateSession();
    int sessionId = sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendNeedMediaDataEvent(sessionId);
    sendNeedMediaDataEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendPositionChangeEvent)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendPostionChangeEvent();
    sendPostionChangeEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendQosEvent)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendQosEvent();
    sendQosEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendPlaybackErrorEvent)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendPlaybackErrorEvent();
    sendPlaybackErrorEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendFirstFrameReceivedEvent)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendFirstFrameReceivedEvent();
    sendFirstFrameReceivedEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendSourceFlushedEvent)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendSourceFlushedEvent();
    sendSourceFlushedEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendPlaybackInfoEvent)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendPlaybackInfoEvent();
    sendPlaybackInfoEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldRenderFrame)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillRenderFrame();
    sendRenderFrameRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, renderFrameFails)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaPipelineServiceWillFailToRenderFrame();
    sendRenderFrameRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetVolume)
{
    mediaPipelineServiceWillSetVolume();
    sendSetVolumeRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetVolume)
{
    mediaPipelineServiceWillFailToSetVolume();
    sendSetVolumeRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetVolume)
{
    mediaPipelineServiceWillGetVolume();
    sendGetVolumeRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetVolume)
{
    mediaPipelineServiceWillFailToGetVolume();
    sendGetVolumeRequestAndReceiveResponseWithoutVolumeMatch();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetMute)
{
    mediaPipelineServiceWillSetMute();
    sendSetMuteRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetMute)
{
    mediaPipelineServiceWillFailToSetMute();
    sendSetMuteRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetMute)
{
    mediaPipelineServiceWillGetMute();
    sendGetMuteRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetMute)
{
    mediaPipelineServiceWillFailToGetMute();
    sendGetMuteRequestAndReceiveResponseWithoutMuteMatch();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetLowLatency)
{
    mediaPipelineServiceWillSetLowLatency();
    sendSetLowLatencyRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetLowLatency)
{
    mediaPipelineServiceWillFailToSetLowLatency();
    sendSetLowLatencyRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetSync)
{
    mediaPipelineServiceWillSetSync();
    sendSetSyncRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetSync)
{
    mediaPipelineServiceWillFailToSetSync();
    sendSetSyncRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetSync)
{
    mediaPipelineServiceWillGetSync();
    sendGetSyncRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetSync)
{
    mediaPipelineServiceWillFailToGetSync();
    sendGetSyncRequestAndReceiveResponseWithoutSyncMatch();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetSyncOff)
{
    mediaPipelineServiceWillSetSyncOff();
    sendSetSyncOffRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetSyncOff)
{
    mediaPipelineServiceWillFailToSetSyncOff();
    sendSetSyncOffRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetStreamSyncMode)
{
    mediaPipelineServiceWillSetStreamSyncMode();
    sendSetStreamSyncModeRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetStreamSyncMode)
{
    mediaPipelineServiceWillFailToSetStreamSyncMode();
    sendSetStreamSyncModeRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetStreamSyncMode)
{
    mediaPipelineServiceWillGetStreamSyncMode();
    sendGetStreamSyncModeRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetStreamSyncMode)
{
    mediaPipelineServiceWillFailToGetStreamSyncMode();
    sendGetStreamSyncModeRequestAndReceiveResponseWithoutStreamSyncModeMatch();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFlush)
{
    mediaPipelineServiceWillFlush();
    sendFlushRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToFlush)
{
    mediaPipelineServiceWillFailToFlush();
    sendFlushRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetSourcePosition)
{
    mediaPipelineServiceWillSetSourcePosition();
    sendSetSourcePositionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetSourcePosition)
{
    mediaPipelineServiceWillFailToSetSourcePosition();
    sendSetSourcePositionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldProcessAudioGap)
{
    mediaPipelineServiceWillProcessAudioGap();
    sendProcessAudioGapRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToProcessAudioGap)
{
    mediaPipelineServiceWillFailToProcessAudioGap();
    sendProcessAudioGapRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetTextTrackIdentifier)
{
    mediaPipelineServiceWillSetTextTrackIdentifier();
    sendSetTextTrackIdentifierRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetTextTrackIdentifier)
{
    mediaPipelineServiceWillFailToSetTextTrackIdentifier();
    sendSetTextTrackIdentifierRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetTextTrackIdentifier)
{
    mediaPipelineServiceWillGetTextTrackIdentifier();
    sendGetTextTrackIdentifierRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetTextTrackIdentifier)
{
    mediaPipelineServiceWillFailToGetTextTrackIdentifier();
    sendGetTextTrackIdentifierRequestAndReceiveResponseWithoutMatch();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetBufferingLimit)
{
    mediaPipelineServiceWillSetBufferingLimit();
    sendSetBufferingLimitRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetBufferingLimit)
{
    mediaPipelineServiceWillFailToSetBufferingLimit();
    sendSetBufferingLimitRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetBufferingLimit)
{
    mediaPipelineServiceWillGetBufferingLimit();
    sendGetBufferingLimitRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetBufferingLimit)
{
    mediaPipelineServiceWillFailToGetBufferingLimit();
    sendGetBufferingLimitRequestAndReceiveResponseWithoutMatch();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetUseBuffering)
{
    mediaPipelineServiceWillSetUseBuffering();
    sendSetUseBufferingRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetUseBuffering)
{
    mediaPipelineServiceWillFailToSetUseBuffering();
    sendSetUseBufferingRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetUseBuffering)
{
    mediaPipelineServiceWillGetUseBuffering();
    sendGetUseBufferingRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetUseBuffering)
{
    mediaPipelineServiceWillFailToGetUseBuffering();
    sendGetUseBufferingRequestAndReceiveResponseWithoutMatch();
}
