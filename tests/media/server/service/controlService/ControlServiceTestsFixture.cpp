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

#include "ControlServiceTestsFixture.h"
#include <utility>

using testing::_;
using testing::Return;

namespace
{
constexpr int kPingId{7};
constexpr firebolt::rialto::ApplicationState kAppState{firebolt::rialto::ApplicationState::INACTIVE};
} // namespace

ControlServiceTests::ControlServiceTests()
    : m_controlServerInternalFactoryMock{std::make_shared<StrictMock<ControlServerInternalFactoryMock>>()},
      m_heartbeatProcedureFactory{std::make_unique<StrictMock<HeartbeatProcedureFactoryMock>>()},
      m_heartbeatProcedureFactoryMock{*m_heartbeatProcedureFactory},
      m_heartbeatProcedureMock{std::make_shared<StrictMock<HeartbeatProcedureMock>>()},
      m_controlServerInternalMock{std::make_shared<StrictMock<ControlServerInternalMock>>()},
      m_controlClientMock{std::make_shared<StrictMock<ControlClientServerInternalMock>>()},
      m_ackSenderMock{std::make_shared<StrictMock<AckSenderMock>>()}, m_sut{m_controlServerInternalFactoryMock,
                                                                            std::move(m_heartbeatProcedureFactory)}
{
}

void ControlServiceTests::controlServerInternalFactoryWillCreateControlServerInternal(int id)
{
    EXPECT_CALL(*m_controlServerInternalFactoryMock, createControlServerInternal(id, _))
        .WillOnce(Return(m_controlServerInternalMock));
    EXPECT_CALL(*m_controlServerInternalMock, registerClient(_, _));
}

void ControlServiceTests::controlServerInternalFactoryWillCreateControlServerInternalWithSetState(int id)
{
    EXPECT_CALL(*m_controlServerInternalFactoryMock, createControlServerInternal(id, _))
        .WillOnce(Return(m_controlServerInternalMock));
    EXPECT_CALL(*m_controlServerInternalMock, registerClient(_, _));
}

void ControlServiceTests::controlServerInternalWillAck()
{
    EXPECT_CALL(*m_controlServerInternalMock, ack(kPingId));
}

void ControlServiceTests::controlServerInternalWillSetApplicationState()
{
    EXPECT_CALL(*m_controlServerInternalMock, setApplicationState(kAppState));
}

void ControlServiceTests::controlServerInternalWillPing()
{
    EXPECT_CALL(*m_heartbeatProcedureMock, createHandler(_));
    EXPECT_CALL(*m_controlServerInternalMock, ping(_));
}

void ControlServiceTests::heartbeatProcedureWillBeCreated()
{
    EXPECT_CALL(m_heartbeatProcedureFactoryMock, createHeartbeatProcedure(_, kPingId))
        .WillOnce(Return(m_heartbeatProcedureMock));
}

void ControlServiceTests::triggerAddControl(int id)
{
    m_sut.addControl(id, m_controlClientMock);
}

void ControlServiceTests::triggerRemoveControl(int id)
{
    m_sut.removeControl(id);
}

bool ControlServiceTests::triggerAck(int id)
{
    return m_sut.ack(id, kPingId);
}

void ControlServiceTests::triggerSetApplicationState()
{
    m_sut.setApplicationState(kAppState);
}

bool ControlServiceTests::triggerPing()
{
    return m_sut.ping(kPingId, m_ackSenderMock);
}
