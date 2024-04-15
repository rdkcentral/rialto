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

#ifndef RIALTO_SERVER_MANAGER_MODULE_SERVICE_TESTS_FIXTURE_H_
#define RIALTO_SERVER_MANAGER_MODULE_SERVICE_TESTS_FIXTURE_H_

#include "ClientMock.h"
#include "ClosureMock.h"
#include "IpcControllerMock.h"
#include "IpcServerMock.h"
#include "RpcControllerMock.h"
#include "SessionServerCommon.h"
#include "SessionServerManagerMock.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

namespace rialto
{
class ServerManagerModule;
} // namespace rialto

class ServerManagerModuleServiceTests : public testing::Test
{
public:
    ServerManagerModuleServiceTests();
    ~ServerManagerModuleServiceTests() override;

    void sessionServerManagerWillHandleRequestSuccess();
    void sessionServerManagerWillHandleRequestFailure();
    void sessionServerManagerWillHandleRequestFailureWithInvalidController();
    void sessionServerManagerWillSetConfiguration(const firebolt::rialto::common::SessionServerState &state);
    void sessionServerManagerWillSetState(const firebolt::rialto::common::SessionServerState &state);
    void sessionServerManagerWillSetLogLevels();
    void sessionServerManagerWillFailToSetConfiguration(const firebolt::rialto::common::SessionServerState &state);
    void sessionServerManagerWillFailToSetState(const firebolt::rialto::common::SessionServerState &state);
    void sessionServerManagerWillPing();
    void sessionServerManagerWillFailToPing();

    void sendSetConfiguration(const firebolt::rialto::common::SessionServerState &state);
    void sendSetState(const firebolt::rialto::common::SessionServerState &state);
    void sendSetLogLevels();
    void sendPing();
    void sendPingWithInvalidController();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_clientMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ServerMock>> m_serverMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClosureMock>> m_closureMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ControllerMock>> m_controllerMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::RpcControllerMock>> m_invalidControllerMock;
    StrictMock<firebolt::rialto::server::service::SessionServerManagerMock> m_sessionServerManagerMock;
    std::shared_ptr<::rialto::ServerManagerModule> m_sut;
};

#endif // RIALTO_SERVER_MANAGER_MODULE_SERVICE_TESTS_FIXTURE_H_
