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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_MOCK_MEDIA_PIPELINE_MODULE_SERVICE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_IPC_MOCK_MEDIA_PIPELINE_MODULE_SERVICE_MOCK_H_

#include "IMediaPipelineModuleService.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::server::ipc::mock
{
class MediaPipelineModuleServiceMock : public IMediaPipelineModuleService
{
public:
    MediaPipelineModuleServiceMock() = default;
    virtual ~MediaPipelineModuleServiceMock() = default;

    MOCK_METHOD(void, clientConnected, (const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient), (override));
    MOCK_METHOD(void, clientDisconnected, (const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient),
                (override));
    MOCK_METHOD(void, createSession,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::CreateSessionRequest *request,
                 ::firebolt::rialto::CreateSessionResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, destroySession,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::DestroySessionRequest *request,
                 ::firebolt::rialto::DestroySessionResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, load,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::LoadRequest *request,
                 ::firebolt::rialto::LoadResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, setVideoWindow,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetVideoWindowRequest *request,
                 ::firebolt::rialto::SetVideoWindowResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, attachSource,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::AttachSourceRequest *request,
                 ::firebolt::rialto::AttachSourceResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, removeSource,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::RemoveSourceRequest *request,
                 ::firebolt::rialto::RemoveSourceResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, play,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::PlayRequest *request,
                 ::firebolt::rialto::PlayResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, pause,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::PauseRequest *request,
                 ::firebolt::rialto::PauseResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, stop,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::StopRequest *request,
                 ::firebolt::rialto::StopResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, setPosition,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetPositionRequest *request,
                 ::firebolt::rialto::SetPositionResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, haveData,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::HaveDataRequest *request,
                 ::firebolt::rialto::HaveDataResponse *response, ::google::protobuf::Closure *done),
                (override));
};
} // namespace firebolt::rialto::server::ipc::mock

#endif // FIREBOLT_RIALTO_SERVER_IPC_MOCK_MEDIA_PIPELINE_MODULE_SERVICE_MOCK_H_
