/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#ifndef CONTROL_MODULE_MOCK_H_
#define CONTROL_MODULE_MOCK_H_

#include "SchemaVersion.h"
#include "controlmodule.pb.h"
#include <gmock/gmock.h>

class ControlModuleMock : public ::firebolt::rialto::ControlModule
{
public:
    MOCK_METHOD(void, getSharedMemory,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetSharedMemoryRequest *request,
                 ::firebolt::rialto::GetSharedMemoryResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, registerClient,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::RegisterClientRequest *request,
                 ::firebolt::rialto::RegisterClientResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, ack,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::AckRequest *request,
                 ::firebolt::rialto::AckResponse *response, ::google::protobuf::Closure *done));

    void defaultReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        done->Run();
    }

    void failureReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        controller->SetFailed("Failed for some reason ...");
        done->Run();
    }

    ::firebolt::rialto::RegisterClientResponse
    getRegisterClientResponse(const int32_t controlId, const firebolt::rialto::common::SchemaVersion &schemaVersion)
    {
        firebolt::rialto::RegisterClientResponse response;
        response.set_control_handle(controlId);
        response.mutable_server_schema_version()->set_major(schemaVersion.major());
        response.mutable_server_schema_version()->set_minor(schemaVersion.minor());
        response.mutable_server_schema_version()->set_patch(schemaVersion.patch());
        return response;
    }

    ::firebolt::rialto::GetSharedMemoryResponse getSharedMemoryResponse(const int32_t fd, const uint32_t size)
    {
        firebolt::rialto::GetSharedMemoryResponse response;
        response.set_fd(fd);
        response.set_size(size);
        return response;
    }

    ControlModuleMock() {}
    virtual ~ControlModuleMock() = default;
};

#endif // CONTROL_MODULE_MOCK_H_
