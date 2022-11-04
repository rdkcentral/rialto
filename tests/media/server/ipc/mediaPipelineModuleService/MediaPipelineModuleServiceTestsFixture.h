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

#ifndef MEDIA_PIPELINE_MODULE_SERVICE_TESTS_FIXTURE_H_
#define MEDIA_PIPELINE_MODULE_SERVICE_TESTS_FIXTURE_H_

#include "ClosureMock.h"
#include "IMediaPipelineModuleService.h"
#include "IpcClientMock.h"
#include "IpcControllerMock.h"
#include "IpcServerMock.h"
#include "PlaybackServiceMock.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class MediaPipelineModuleServiceTests : public testing::Test
{
public:
    MediaPipelineModuleServiceTests();
    ~MediaPipelineModuleServiceTests() override;

    void clientWillConnect();
    void clientWillDisconnect();
    void playbackServiceWillCreateSession();
    void playbackServiceWillFailToCreateSession();
    void playbackServiceWillDestroySession();
    void playbackServiceWillFailToDestroySession();
    void playbackServiceWillLoadSession();
    void playbackServiceWillFailToLoadSession();
    void playbackServiceWillAttachSource();
    void playbackServiceWillFailToAttachSource();
    void playbackServiceWillPlay();
    void playbackServiceWillFailToPlay();
    void playbackServiceWillPause();
    void playbackServiceWillFailToPause();
    void playbackServiceWillStop();
    void playbackServiceWillFailToStop();
    void playbackServiceWillSetPosition();
    void playbackServiceWillFailToSetPosition();
    void playbackServiceWillSetVideoWindow();
    void playbackServiceWillFailToSetVideoWindow();
    void playbackServiceWillHaveData();
    void playbackServiceWillFailToHaveData();
    void playbackServiceWillSetPlaybackRate();
    void playbackServiceWillFailToSetPlaybackRate();
    void playbackServiceWillGetPosition();
    void playbackServiceWillFailToGetPosition();
    void mediaClientWillSendPlaybackStateChangedEvent();
    void mediaClientWillSendNetworkStateChangedEvent();
    void mediaClientWillSendNeedMediaDataEvent(int sessionId);
    void mediaClientWillSendPostionChangeEvent();
    void mediaClientWillSendQosEvent();

    void sendClientConnected();
    void sendClientDisconnected();
    int sendCreateSessionRequestAndReceiveResponse();
    void sendCreateSessionRequestAndExpectFailure();
    void sendDestroySessionRequestAndReceiveResponse();
    void sendLoadRequestAndReceiveResponse();
    void sendAttachSourceRequestAndReceiveResponse();
    void sendPlayRequestAndReceiveResponse();
    void sendPauseRequestAndReceiveResponse();
    void sendStopRequestAndReceiveResponse();
    void sendSetPositionRequestAndReceiveResponse();
    void sendGetPositionRequestAndReceiveResponse();
    void sendGetPositionRequestAndReceiveResponseWithoutPositionMatch();
    void sendHaveDataRequestAndReceiveResponse();
    void sendSetPlaybackRateRequestAndReceiveResponse();
    void sendSetVideoWindowRequestAndReceiveResponse();
    void sendPlaybackStateChangedEvent();
    void sendNetworkStateChangedEvent();
    void sendNeedMediaDataEvent();
    void sendPostionChangeEvent();
    void sendQosEvent();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_clientMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ServerMock>> m_serverMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClosureMock>> m_closureMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ControllerMock>> m_controllerMock;
    StrictMock<firebolt::rialto::server::service::PlaybackServiceMock> m_playbackServiceMock;
    std::shared_ptr<firebolt::rialto::IMediaPipelineClient> m_mediaPipelineClient;
    std::shared_ptr<firebolt::rialto::server::ipc::IMediaPipelineModuleService> m_service;

    void expectRequestSuccess();
    void expectRequestFailure();
};

#endif // MEDIA_PIPELINE_MODULE_SERVICE_TESTS_FIXTURE_H_
