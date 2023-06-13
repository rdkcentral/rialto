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

#ifndef CONTROL_SERVICE_TESTS_FIXTURE_H_
#define CONTROL_SERVICE_TESTS_FIXTURE_H_

#include "AckSenderMock.h"
#include "ControlClientServerInternalMock.h"
#include "ControlServerInternalFactoryMock.h"
#include "ControlServerInternalMock.h"
#include "ControlService.h"
#include "HeartbeatProcedureFactoryMock.h"
#include "HeartbeatProcedureMock.h"
#include <gtest/gtest.h>
#include <memory>

using firebolt::rialto::ApplicationState;
using firebolt::rialto::server::AckSenderMock;
using firebolt::rialto::server::ControlClientServerInternalMock;
using firebolt::rialto::server::ControlServerInternalFactoryMock;
using firebolt::rialto::server::ControlServerInternalMock;
using firebolt::rialto::server::HeartbeatProcedureFactoryMock;
using firebolt::rialto::server::HeartbeatProcedureMock;
using firebolt::rialto::server::service::ControlService;
using testing::StrictMock;

class ControlServiceTests : public testing::Test
{
public:
    ControlServiceTests();
    ~ControlServiceTests() override = default;

    void controlServerInternalFactoryWillCreateControlServerInternal(int id);
    void controlServerInternalFactoryWillCreateControlServerInternalWithSetState(int id);
    void controlServerInternalWillAck();
    void controlServerInternalWillSetApplicationState();
    void controlServerInternalWillPing();
    void heartbeatProcedureWillBeCreated();

    void triggerAddControl(int id);
    void triggerRemoveControl(int id);
    bool triggerAck(int id);
    bool triggerPing();
    void triggerSetApplicationState();

private:
    std::shared_ptr<StrictMock<ControlServerInternalFactoryMock>> m_controlServerInternalFactoryMock;
    std::unique_ptr<StrictMock<HeartbeatProcedureFactoryMock>> m_heartbeatProcedureFactory;
    StrictMock<HeartbeatProcedureFactoryMock> &m_heartbeatProcedureFactoryMock;
    std::shared_ptr<StrictMock<HeartbeatProcedureMock>> m_heartbeatProcedureMock;
    std::shared_ptr<StrictMock<ControlServerInternalMock>> m_controlServerInternalMock;
    std::shared_ptr<StrictMock<ControlClientServerInternalMock>> m_controlClientMock;
    std::shared_ptr<StrictMock<AckSenderMock>> m_ackSenderMock;
    ControlService m_sut;
};

#endif // CONTROL_SERVICE_TESTS_FIXTURE_H_
