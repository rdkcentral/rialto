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

#ifndef APPLICATION_MANAGEMENT_SERVER_TESTS_FIXTURE_H_
#define APPLICATION_MANAGEMENT_SERVER_TESTS_FIXTURE_H_

#include "IApplicationManagementServer.h"
#include "IpcClientMock.h"
#include "IpcServerMock.h"
#include "ServerManagerModuleServiceMock.h"
#include "SessionServerManagerMock.h"
#include "SessionServerCommon.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class ApplicationManagementServerTests : public testing::Test
{
public:
    ApplicationManagementServerTests();
    ~ApplicationManagementServerTests() override;

    void clientWillBeInitialized();
    void clientWillFailToInitialized();
    void clientWillReceiveStateChangedEvent(const firebolt::rialto::common::SessionServerState &state);
    void clientWillNotBeConnected();
    void serverThreadWillStart();
    void clientWillBeDisconnected();

    void initializeApplicationManager();
    void initializeApplicationManagerAndExpectFailure();
    void sendStateChangedEvent(const firebolt::rialto::common::SessionServerState &state);
    void sendStateChangedEventAndExpectFailure(const firebolt::rialto::common::SessionServerState &state);
    void startApplicationManager();
    void stopApplicationManager();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_clientMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ServerMock>> m_serverMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::ServerManagerModuleServiceMock>> m_serverManagerModuleMock;
    StrictMock<firebolt::rialto::server::service::SessionServerManagerMock> m_sessionServerManagerMock;
    std::unique_ptr<firebolt::rialto::server::ipc::IApplicationManagementServer> m_sut;
};

#endif // APPLICATION_MANAGEMENT_SERVER_TESTS_FIXTURE_H_
