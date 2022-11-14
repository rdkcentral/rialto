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

#ifndef FIREBOLT_RIALTO_IPC_TEST_MODULE_MOCK_H_
#define FIREBOLT_RIALTO_IPC_TEST_MODULE_MOCK_H_

#include "testmodule.pb.h"
#include <gmock/gmock.h>
#include <string>

namespace firebolt::rialto::ipc
{
class TestModuleMock : public ::firebolt::rialto::TestModule
{
public:
    MOCK_METHOD(void, TestRequestSingleVar,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::TestSingleVar *request,
                 ::firebolt::rialto::TestNoVar *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, TestRequestMultiVar,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::TestMultiVar *request,
                 ::firebolt::rialto::TestNoVar *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, TestResponseSingleVar,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::TestNoVar *request,
                 ::firebolt::rialto::TestSingleVar *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, TestResponseMultiVar,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::TestNoVar *request,
                 ::firebolt::rialto::TestMultiVar *response, ::google::protobuf::Closure *done));

    void defaultReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        done->Run();
    }

    void failureReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        controller->SetFailed("Failed for some reason ...");
        done->Run();
    }

    ::firebolt::rialto::TestSingleVar getSingleVarResponse(int32_t var1)
    {
        firebolt::rialto::TestSingleVar response;
        response.set_var1(var1);
        return response;
    }

    ::firebolt::rialto::TestMultiVar getMultiVarResponse(int32_t var1, uint32_t var2,
                                                         firebolt::rialto::TestMultiVar_TestType var3, std::string var4)
    {
        firebolt::rialto::TestMultiVar response;
        response.set_var1(var1);
        response.set_var2(var2);
        response.set_var3(var3);
        response.set_var4(var4);
        return response;
    }

    TestModuleMock() {}
    virtual ~TestModuleMock() = default;
};
} // namespace firebolt::rialto::ipc

#endif // FIREBOLT_RIALTO_IPC_TEST_MODULE_MOCK_H_
