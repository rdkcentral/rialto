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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_WEB_AUDIO_PLAYER_MODULE_SERVICE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_IPC_WEB_AUDIO_PLAYER_MODULE_SERVICE_MOCK_H_

#include "IWebAudioPlayerModuleService.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::server::ipc
{
class WebAudioPlayerModuleServiceMock : public IWebAudioPlayerModuleService
{
public:
    WebAudioPlayerModuleServiceMock() = default;
    virtual ~WebAudioPlayerModuleServiceMock() = default;

    MOCK_METHOD(void, clientConnected, (const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient), (override));
    MOCK_METHOD(void, clientDisconnected, (const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient),
                (override));

    MOCK_METHOD(void, createWebAudioPlayer,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::CreateWebAudioPlayerRequest *request,
                 ::firebolt::rialto::CreateWebAudioPlayerResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, destroyWebAudioPlayer,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::DestroyWebAudioPlayerRequest *request,
                 ::firebolt::rialto::DestroyWebAudioPlayerResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, play,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::WebAudioPlayRequest *request,
                 ::firebolt::rialto::WebAudioPlayResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, pause,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::WebAudioPauseRequest *request,
                 ::firebolt::rialto::WebAudioPauseResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, setEos,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::WebAudioSetEosRequest *request,
                 ::firebolt::rialto::WebAudioSetEosResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, getBufferAvailable,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioGetBufferAvailableRequest *request,
                 ::firebolt::rialto::WebAudioGetBufferAvailableResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, getBufferDelay,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioGetBufferDelayRequest *request,
                 ::firebolt::rialto::WebAudioGetBufferDelayResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, writeBuffer,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioWriteBufferRequest *request,
                 ::firebolt::rialto::WebAudioWriteBufferResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, getDeviceInfo,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioGetDeviceInfoRequest *request,
                 ::firebolt::rialto::WebAudioGetDeviceInfoResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, setVolume,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioSetVolumeRequest *request,
                 ::firebolt::rialto::WebAudioSetVolumeResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, getVolume,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioGetVolumeRequest *request,
                 ::firebolt::rialto::WebAudioGetVolumeResponse *response, ::google::protobuf::Closure *done),
                (override));
};
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_WEB_AUDIO_PLAYER_MODULE_SERVICE_MOCK_H_
