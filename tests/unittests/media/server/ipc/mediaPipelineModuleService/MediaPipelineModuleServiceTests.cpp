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

TEST_F(MediaPipelineModuleServiceTests, shouldAttachAudioSourceWithAdditionalData)
{
    mediaPipelineServiceWillAttachAudioSourceWithAdditionaldata();
    sendAttachAudioSourceWithAdditionalDataRequestAndReceiveResponse();
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

TEST_F(MediaPipelineModuleServiceTests, shouldSendSourceFlushedEvent)
{
    mediaPipelineServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendSourceFlushedEvent();
    sendSourceFlushedEvent();
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
