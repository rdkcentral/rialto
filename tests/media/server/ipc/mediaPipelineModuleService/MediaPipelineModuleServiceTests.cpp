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

TEST_F(MediaPipelineModuleServiceTests, shouldCreateSession)
{
    playbackServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToCreateSession)
{
    playbackServiceWillFailToCreateSession();
    sendCreateSessionRequestAndExpectFailure();
}

TEST_F(MediaPipelineModuleServiceTests, shouldDestroySessionWhenDisconnectClient)
{
    clientWillConnect();
    sendClientConnected();
    playbackServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    clientWillDisconnect();
    sendClientDisconnected();
}

TEST_F(MediaPipelineModuleServiceTests, shouldDestroySession)
{
    playbackServiceWillDestroySession();
    sendDestroySessionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToDestroySession)
{
    playbackServiceWillFailToDestroySession();
    sendDestroySessionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldLoadSession)
{
    playbackServiceWillLoadSession();
    sendLoadRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToLoadSession)
{
    playbackServiceWillFailToLoadSession();
    sendLoadRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldAttachSource)
{
    playbackServiceWillAttachSource();
    sendAttachSourceRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldAttachAudioSourceWithAdditionalData)
{
    playbackServiceWillAttachAudioSourceWithAdditionaldata();
    sendAttachAudioSourceWithAdditionalDataRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToAttachSource)
{
    playbackServiceWillFailToAttachSource();
    sendAttachSourceRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldPlay)
{
    playbackServiceWillPlay();
    sendPlayRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToPlay)
{
    playbackServiceWillFailToPlay();
    sendPlayRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldPause)
{
    playbackServiceWillPause();
    sendPauseRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToPause)
{
    playbackServiceWillFailToPause();
    sendPauseRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldStop)
{
    playbackServiceWillStop();
    sendStopRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToStop)
{
    playbackServiceWillFailToStop();
    sendStopRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetPosition)
{
    playbackServiceWillSetPosition();
    sendSetPositionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetPosition)
{
    playbackServiceWillFailToSetPosition();
    sendSetPositionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldHaveData)
{
    playbackServiceWillHaveData();
    sendHaveDataRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailWhenHaveDataIsReceived)
{
    playbackServiceWillFailToHaveData();
    sendHaveDataRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetPlaybackRate)
{
    playbackServiceWillSetPlaybackRate();
    sendSetPlaybackRateRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailWhenSetPlaybackRateIsReceived)
{
    playbackServiceWillFailToSetPlaybackRate();
    sendSetPlaybackRateRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSetVideoWindow)
{
    playbackServiceWillSetVideoWindow();
    sendSetVideoWindowRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToSetVideoWindow)
{
    playbackServiceWillFailToSetVideoWindow();
    sendSetVideoWindowRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldGetPosition)
{
    playbackServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    playbackServiceWillGetPosition();
    sendGetPositionRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, shouldFailToGetPosition)
{
    playbackServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    playbackServiceWillFailToGetPosition();
    sendGetPositionRequestAndReceiveResponseWithoutPositionMatch();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendPlaybackStateChangedEvent)
{
    playbackServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendPlaybackStateChangedEvent();
    sendPlaybackStateChangedEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendNetworkStateChangedEvent)
{
    playbackServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendNetworkStateChangedEvent();
    sendNetworkStateChangedEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendNeedMediaDataEvent)
{
    playbackServiceWillCreateSession();
    int sessionId = sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendNeedMediaDataEvent(sessionId);
    sendNeedMediaDataEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendPositionChangeEvent)
{
    playbackServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendPostionChangeEvent();
    sendPostionChangeEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldSendQosEvent)
{
    playbackServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    mediaClientWillSendQosEvent();
    sendQosEvent();
}

TEST_F(MediaPipelineModuleServiceTests, shouldRenderFrame)
{
    playbackServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    playbackServiceWillRenderFrame();
    sendRenderFrameRequestAndReceiveResponse();
}

TEST_F(MediaPipelineModuleServiceTests, renderFrameFails)
{
    playbackServiceWillCreateSession();
    sendCreateSessionRequestAndReceiveResponse();
    playbackServiceWillFailToRenderFrame();
    sendRenderFrameRequestAndReceiveResponse();
}