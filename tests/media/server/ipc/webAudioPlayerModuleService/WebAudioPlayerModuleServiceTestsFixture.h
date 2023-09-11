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

#ifndef WEB_AUDIO_PLAYER_MODULE_SERVICE_TESTS_FIXTURE_H_
#define WEB_AUDIO_PLAYER_MODULE_SERVICE_TESTS_FIXTURE_H_

#include "ClosureMock.h"
#include "IWebAudioPlayerModuleService.h"
#include "IpcClientMock.h"
#include "IpcControllerMock.h"
#include "IpcServerMock.h"
#include "WebAudioPlayerServiceMock.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class WebAudioPlayerModuleServiceTests : public testing::Test
{
public:
    WebAudioPlayerModuleServiceTests();
    ~WebAudioPlayerModuleServiceTests() override;

    void testFactoryCreatesObject();

    void clientWillConnect();
    void clientWillDisconnect(int handle);
    void webAudioPlayerServiceWillCreateWebAudioPlayer();
    void webAudioPlayerServiceWillCreateWebAudioPlayerWithPcmConfig();
    void webAudioPlayerServiceWillFailToCreateWebAudioPlayer();
    void webAudioPlayerServiceWillDestroyWebAudioPlayer();
    void webAudioPlayerServiceWillFailToDestroyWebAudioPlayer();
    void webAudioPlayerServiceWillPlay();
    void webAudioPlayerServiceWillFailToPlay();
    void webAudioPlayerServiceWillPause();
    void webAudioPlayerServiceWillFailToPause();
    void webAudioPlayerServiceWillSetEos();
    void webAudioPlayerServiceWillFailToSetEos();
    void webAudioPlayerServiceWillGetBufferAvailable();
    void webAudioPlayerServiceWillFailToGetBufferAvailable();
    void webAudioPlayerServiceWillGetBufferDelay();
    void webAudioPlayerServiceWillFailToGetBufferDelay();
    void webAudioPlayerServiceWillWriteBuffer();
    void webAudioPlayerServiceWillFailToWriteBuffer();
    void webAudioPlayerServiceWillGetDeviceInfo();
    void webAudioPlayerServiceWillFailToGetDeviceInfo();
    void webAudioPlayerServiceWillSetVolume();
    void webAudioPlayerServiceWillFailToSetVolume();
    void webAudioPlayerServiceWillGetVolume();
    void webAudioPlayerServiceWillFailToGetVolume();

    void webAudioPlayerClientWillSendPlayerStateEvent();

    void sendClientConnected();
    void sendClientDisconnected();
    int sendCreateWebAudioPlayerRequestAndReceiveResponse();
    int sendCreateWebAudioPlayerRequestWithPcmConfigAndReceiveResponse();
    void sendCreateWebAudioPlayerRequestAndExpectFailure();
    void sendDestroyWebAudioPlayerRequestAndReceiveResponse();
    void sendPlayRequestAndReceiveResponse();
    void sendPauseRequestAndReceiveResponse();
    void sendSetEosRequestAndReceiveResponse();
    void sendGetBufferAvailableRequestAndReceiveResponse();
    void sendGetBufferAvailableRequestAndExpectFailure();
    void sendGetBufferDelayRequestAndReceiveResponse();
    void sendGetBufferDelayRequestAndExpectFailure();
    void sendWriteBufferRequestAndReceiveResponse();
    void sendGetDeviceInfoRequestAndReceiveResponse();
    void sendGetDeviceInfoRequestAndExpectFailure();
    void sendSetVolumeRequestAndReceiveResponse();
    void sendGetVolumeRequestAndReceiveResponse();
    void sendGetVolumeRequestAndExpectFailure();
    void sendPlayerStateEvent();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_clientMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ServerMock>> m_serverMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClosureMock>> m_closureMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ControllerMock>> m_controllerMock;
    StrictMock<firebolt::rialto::server::service::WebAudioPlayerServiceMock> m_webAudioPlayerServiceMock;
    std::shared_ptr<firebolt::rialto::IWebAudioPlayerClient> m_webAudioPlayerClient;
    std::shared_ptr<firebolt::rialto::WebAudioShmInfo> m_shmInfo;
    std::shared_ptr<firebolt::rialto::server::ipc::IWebAudioPlayerModuleService> m_service;

    void expectRequestSuccess();
    void expectRequestFailure();
};

#endif // WEB_AUDIO_PLAYER_MODULE_SERVICE_TESTS_FIXTURE_H_
