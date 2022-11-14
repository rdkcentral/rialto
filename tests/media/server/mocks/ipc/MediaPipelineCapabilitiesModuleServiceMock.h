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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_MEDIA_PIPELINE_CAPABILITIES_MODULE_SERVICE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_IPC_MEDIA_PIPELINE_CAPABILITIES_MODULE_SERVICE_MOCK_H_

#include "IMediaPipelineCapabilitiesModuleService.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::server::ipc
{
class MediaPipelineCapabilitiesModuleServiceMock : public IMediaPipelineCapabilitiesModuleService
{
public:
    MediaPipelineCapabilitiesModuleServiceMock() = default;
    virtual ~MediaPipelineCapabilitiesModuleServiceMock() = default;

    MOCK_METHOD(void, clientConnected, (const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient), (override));
    MOCK_METHOD(void, clientDisconnected, (const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient),
                (override));
    MOCK_METHOD(void, getSupportedMimeTypes,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetSupportedMimeTypesRequest *request,
                 ::firebolt::rialto::GetSupportedMimeTypesResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, isMimeTypeSupported,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::IsMimeTypeSupportedRequest *request,
                 ::firebolt::rialto::IsMimeTypeSupportedResponse *response, ::google::protobuf::Closure *done),
                (override));
};
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_MEDIA_PIPELINE_CAPABILITIES_MODULE_SERVICE_MOCK_H_
