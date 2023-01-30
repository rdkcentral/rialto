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

#include "ApplicationManagementServerTestsFixture.h"
#include "ApplicationManagementServer.h"
#include "IpcClientMock.h"
#include "IpcServerFactoryMock.h"
#include "ServerManagerModuleServiceFactoryMock.h"

using testing::_;
using testing::Return;

namespace
{
constexpr int socket{2};

rialto::SessionServerState convertSessionServerState(const firebolt::rialto::common::SessionServerState &state)
{
    switch (state)
    {
    case firebolt::rialto::common::SessionServerState::UNINITIALIZED:
        return rialto::SessionServerState::UNINITIALIZED;
    case firebolt::rialto::common::SessionServerState::INACTIVE:
        return rialto::SessionServerState::INACTIVE;
    case firebolt::rialto::common::SessionServerState::ACTIVE:
        return rialto::SessionServerState::ACTIVE;
    case firebolt::rialto::common::SessionServerState::NOT_RUNNING:
        return rialto::SessionServerState::NOT_RUNNING;
    case firebolt::rialto::common::SessionServerState::ERROR:
        return rialto::SessionServerState::ERROR;
    }
    return rialto::SessionServerState::ERROR;
}
} // namespace

MATCHER_P(StateChangedEventMatcher, state, "")
{
    std::shared_ptr<rialto::StateChangedEvent> event = std::dynamic_pointer_cast<rialto::StateChangedEvent>(arg);
    return (state == event->sessionserverstate());
}

ApplicationManagementServerTests::ApplicationManagementServerTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_serverMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ServerMock>>()},
      m_serverManagerModuleMock{
          std::make_shared<StrictMock<firebolt::rialto::server::ipc::ServerManagerModuleServiceMock>>()}
{
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ServerFactoryMock>> serverFactoryMock =
        std::make_shared<StrictMock<firebolt::rialto::ipc::ServerFactoryMock>>();
    EXPECT_CALL(*serverFactoryMock, create(_)).WillOnce(Return(m_serverMock));
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::ServerManagerModuleServiceFactoryMock>>
        serverManagerModuleFactoryMock =
            std::make_shared<StrictMock<firebolt::rialto::server::ipc::ServerManagerModuleServiceFactoryMock>>();
    EXPECT_CALL(*serverManagerModuleFactoryMock, create(_)).WillOnce(Return(m_serverManagerModuleMock));
    m_sut = std::make_unique<firebolt::rialto::server::ipc::ApplicationManagementServer>(serverFactoryMock,
                                                                                         serverManagerModuleFactoryMock,
                                                                                         m_sessionServerManagerMock);
}

ApplicationManagementServerTests::~ApplicationManagementServerTests() {}

void ApplicationManagementServerTests::clientWillBeInitialized()
{
    EXPECT_CALL(*m_serverMock, addClient(socket, _)).WillOnce(Return(m_clientMock));
    EXPECT_CALL(*m_clientMock, exportService(_));
}

void ApplicationManagementServerTests::clientWillFailToInitialized()
{
    EXPECT_CALL(*m_serverMock, addClient(_, _)).WillOnce(Return(nullptr));
}

void ApplicationManagementServerTests::clientWillReceiveStateChangedEvent(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(*m_clientMock, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*m_clientMock, sendEvent(StateChangedEventMatcher(convertSessionServerState(state))));
}

void ApplicationManagementServerTests::clientWillNotBeConnected()
{
    EXPECT_CALL(*m_clientMock, isConnected()).WillOnce(Return(false));
}

void ApplicationManagementServerTests::serverThreadWillStart()
{
    EXPECT_CALL(*m_serverMock, process()).WillOnce(Return(false));
}

void ApplicationManagementServerTests::clientWillBeDisconnected()
{
    EXPECT_CALL(*m_clientMock, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*m_clientMock, disconnect());
}

void ApplicationManagementServerTests::initializeApplicationManager()
{
    EXPECT_TRUE(m_sut->initialize(socket));
}

void ApplicationManagementServerTests::initializeApplicationManagerAndExpectFailure()
{
    EXPECT_FALSE(m_sut->initialize(socket));
}

void ApplicationManagementServerTests::sendStateChangedEvent(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_TRUE(m_sut->sendStateChangedEvent(state));
}

void ApplicationManagementServerTests::sendStateChangedEventAndExpectFailure(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_FALSE(m_sut->sendStateChangedEvent(state));
}

void ApplicationManagementServerTests::startApplicationManager()
{
    m_sut->start();
}

void ApplicationManagementServerTests::stopApplicationManager()
{
    m_sut->stop();
}
