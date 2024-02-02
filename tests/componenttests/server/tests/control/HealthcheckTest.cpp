/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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
 *
 */

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "ControlTest.h"
#include "ExpectMessage.h"
#include "MessageBuilders.h"

namespace
{
constexpr int kPingId{1};
} // namespace

namespace firebolt::rialto::server::ct
{
/*
 * Component Test: Healthcheck procedure with connected client
 * Test Objective:
 *  Test the full healthcheck procedure with ping message sent to rialto client
 *
 * Sequence Diagrams:
 *  https://wiki.rdkcentral.com/pages/viewpage.action?spaceKey=ASP&title=Healthcheck+mechanism
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: Control
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *  Register Rialto Client Stub in RUNNING state
 *
 * Test Steps:
 *  Step 1: Start healthcheck procedure
 *   ServerManager stub requests the server to check state of all its components
 *   Expect that all server services are not deadlocked.
 *   Expect that PingEvent is received by connected Rialto Client stub.
 *
 *  Step 2: Finish healthcheck procedure
 *   Rialto Client stub sends Ack message to Rialto Server
 *   Expect that Rialto Server sends Ack to Server Manager Stub
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  The healthcheck procedure is performed successfully.
 *
 * Code:
 */
TEST_F(ControlTest, healthcheckProcedure)
{
    // Step 1: Start healthcheck procedure
    ExpectMessage<PingEvent> expectedPing{m_clientStub};

    ::rialto::PingRequest request{createPingRequest(kPingId)};
    ConfigureAction<::firebolt::rialto::server::ct::Ping>(m_serverManagerStub).send(request).expectSuccess();

    auto receivedPing{expectedPing.getMessage()};
    ASSERT_TRUE(receivedPing);
    EXPECT_EQ(receivedPing->control_handle(), m_controlHandle);
    EXPECT_EQ(receivedPing->id(), kPingId);

    // Step 2: Finish healthcheck procedure
    ExpectMessage<::rialto::AckEvent> expectedAck(m_serverManagerStub);

    auto ackReq{createAckRequest(m_controlHandle, kPingId)};
    ConfigureAction<Ack>(m_clientStub).send(ackReq).expectSuccess();

    auto receivedAck = expectedAck.getMessage();
    ASSERT_TRUE(receivedAck);
    EXPECT_EQ(receivedAck->id(), kPingId);
    EXPECT_EQ(receivedAck->success(), true);
}

/*
 * Component Test: Healthcheck procedure without ACK from connected client
 * Test Objective:
 *  Test the healthcheck procedure with ping message sent to rialto client and no ACK
 *
 * Sequence Diagrams:
 *  https://wiki.rdkcentral.com/pages/viewpage.action?spaceKey=ASP&title=Healthcheck+mechanism
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: Control
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *  Register Rialto Client Stub in RUNNING state
 *
 * Test Steps:
 *  Step 1: Start healthcheck procedure
 *   ServerManager stub requests the server to check state of all its components
 *   Expect that all server services are not deadlocked.
 *   Expect that PingEvent is received by connected Rialto Client stub.
 *
 *  Step 2: Fail healthcheck procedure
 *   Rialto Client stub doesnt send Ack message to Rialto Server
 *   Expect that Rialto Server doesnt send Ack to Server Manager Stub
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  The healthcheck procedure is failed.
 *
 * Code:
 */
TEST_F(ControlTest, noAckWithoutClientResponse)
{
    // Step 1: Start healthcheck procedure
    ExpectMessage<PingEvent> expectedPing{m_clientStub};

    ::rialto::PingRequest request{createPingRequest(kPingId)};
    ConfigureAction<::firebolt::rialto::server::ct::Ping>(m_serverManagerStub).send(request).expectSuccess();

    auto receivedPing{expectedPing.getMessage()};
    ASSERT_TRUE(receivedPing);
    EXPECT_EQ(receivedPing->control_handle(), m_controlHandle);
    EXPECT_EQ(receivedPing->id(), kPingId);

    // Step 2: Fail healthcheck procedure
    ExpectMessage<::rialto::AckEvent> expectedAck(m_serverManagerStub);

    auto receivedAck = expectedAck.getMessage();
    EXPECT_FALSE(receivedAck);
}

/*
 * Component Test: Healthcheck procedure with ACK with wrong ID from connected client
 * Test Objective:
 *  Test the healthcheck procedure with ping message sent to rialto client and ACK with wrong ID
 *
 * Sequence Diagrams:
 *  https://wiki.rdkcentral.com/pages/viewpage.action?spaceKey=ASP&title=Healthcheck+mechanism
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: Control
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *  Register Rialto Client Stub in RUNNING state
 *
 * Test Steps:
 *  Step 1: Start healthcheck procedure
 *   ServerManager stub requests the server to check state of all its components
 *   Expect that all server services are not deadlocked.
 *   Expect that PingEvent is received by connected Rialto Client stub.
 *
 *  Step 2: Fail healthcheck procedure
 *   Rialto Client stub sends Ack message with wrong id to Rialto Server
 *   Expect that Rialto Server doesnt send Ack to Server Manager Stub
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  The healthcheck procedure is failed.
 *
 * Code:
 */
TEST_F(ControlTest, noAckWhenClientRespondsWithWrongId)
{
    // Step 1: Start healthcheck procedure
    ExpectMessage<PingEvent> expectedPing{m_clientStub};

    ::rialto::PingRequest request{createPingRequest(kPingId)};
    ConfigureAction<::firebolt::rialto::server::ct::Ping>(m_serverManagerStub).send(request).expectSuccess();

    auto receivedPing{expectedPing.getMessage()};
    ASSERT_TRUE(receivedPing);
    EXPECT_EQ(receivedPing->control_handle(), m_controlHandle);
    EXPECT_EQ(receivedPing->id(), kPingId);

    // Step 2: Fail healthcheck procedure
    ExpectMessage<::rialto::AckEvent> expectedAck(m_serverManagerStub);

    auto ackReq{createAckRequest(m_controlHandle, kPingId + 1)};
    ConfigureAction<Ack>(m_clientStub).send(ackReq).expectSuccess();

    auto receivedAck = expectedAck.getMessage();
    EXPECT_FALSE(receivedAck);
}
} // namespace firebolt::rialto::server::ct
