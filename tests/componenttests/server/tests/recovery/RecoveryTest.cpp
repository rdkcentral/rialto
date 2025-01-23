/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "Constants.h"
#include "ExpectMessage.h"
#include "INamedSocket.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"
#include <memory>

namespace
{
constexpr int kPingId{1};
} // namespace

namespace firebolt::rialto::server::ct
{
class RecoveryTest : public RialtoServerComponentTest
{
public:
    RecoveryTest() : m_namedSocket{ipc::INamedSocketFactory::getFactory().createNamedSocket(kSocketName)}
    {
        EXPECT_TRUE(m_namedSocket);
    }

    ~RecoveryTest() override { m_namedSocket.reset(); }

    void configureSutWithFdInActiveState()
    {
        ASSERT_TRUE(m_namedSocket);
        ::rialto::SetConfigurationRequest request{createGenericSetConfigurationReq()};
        request.set_initialsessionserverstate(::rialto::SessionServerState::ACTIVE);
        request.set_sessionmanagementsocketfd(m_namedSocket->getFd());
        request.clear_sessionmanagementsocketname();

        ExpectMessage<::rialto::StateChangedEvent> expectedMessage(m_serverManagerStub);

        ConfigureAction<SetConfiguration>(m_serverManagerStub).send(request).expectSuccess();

        auto receivedMessage = expectedMessage.getMessage();
        ASSERT_TRUE(receivedMessage);
        EXPECT_EQ(receivedMessage->sessionserverstate(), ::rialto::SessionServerState::ACTIVE);
    }

    void registerClient()
    {
        ExpectMessage<ApplicationStateChangeEvent> expectedAppStateChange{m_clientStub};

        auto registerClientReq(createRegisterClientRequest());
        ConfigureAction<RegisterClient>(m_clientStub)
            .send(registerClientReq)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { m_controlHandle = resp.control_handle(); });

        auto receivedMessage = expectedAppStateChange.getMessage();
        ASSERT_TRUE(receivedMessage);
        EXPECT_EQ(receivedMessage->application_state(), ApplicationStateChangeEvent_ApplicationState_RUNNING);
    }

    void performHealthcheckProcedure()
    {
        ExpectMessage<PingEvent> expectedPing{m_clientStub};

        ::rialto::PingRequest request{createPingRequest(kPingId)};
        ConfigureAction<::firebolt::rialto::server::ct::Ping>(m_serverManagerStub).send(request).expectSuccess();

        auto receivedPing{expectedPing.getMessage()};
        ASSERT_TRUE(receivedPing);
        EXPECT_EQ(receivedPing->control_handle(), m_controlHandle);
        EXPECT_EQ(receivedPing->id(), kPingId);

        ExpectMessage<::rialto::AckEvent> expectedAck(m_serverManagerStub);

        auto ackReq{createAckRequest(m_controlHandle, kPingId)};
        ConfigureAction<Ack>(m_clientStub).send(ackReq).expectSuccess();

        auto receivedAck = expectedAck.getMessage();
        ASSERT_TRUE(receivedAck);
        EXPECT_EQ(receivedAck->id(), kPingId);
        EXPECT_EQ(receivedAck->success(), true);
    }

    void simulateCrashAndRecovery()
    {
        m_sut.reset();
        m_serverManagerStub.reset();
        m_clientStub.disconnect();

        configureWrappers();
        initialiseGstreamer();
        startSut();
        initialiseSut();
    }

private:
    std::unique_ptr<ipc::INamedSocket> m_namedSocket;
    int m_controlHandle{-1};
};

/*
 * Component Test: Set Configuration with Socket fd
 * Test Objective:
 *  Establish Rialto Server <-> Rialto client connection using file description
 *  provided by Rialto Server Manager stub.
 *
 * Sequence Diagrams:
 *  https://wiki.rdkcentral.com/display/ASP/Rialto+Application+Session+Management
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: Control
 *
 * Test Initialize:
 *  Initialize Rialto Server service
 *  Create named socket with file descriptor
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *  Register Rialto Client Stub in RUNNING state
 *
 * Test Steps:
 *  Step 1: Send SetConfiguration message with socket file descriptor
 *   ServerManager stub requests the server to configure.
 *   Expect that server is configured correctly
 *
 *  Step 2: Connect Rialto Client stub
 *   Rialto Client tries to connect to socket provided by Rialto Server
 *   Expect that rialto client can be connected successfully
 *
 *  Step 3: Register Rialto Client Stub in RUNNING state
 *   Rialto Client sends a request to register to Rialto Server
 *   Expect that rialto client can be registered successfully
 *
 *  Step 4: Perform healthcheck procedure to test if connection is stable
 *   ServerManager stub requests the server to check state of all its components
 *   Expect that all server services are not deadlocked.
 *   Expect that PingEvent is received by connected Rialto Client stub.
 *   Rialto Client stub sends Ack message to Rialto Server
 *   Expect that Rialto Server sends Ack to Server Manager Stub
 *
 *  Step 5: Simulate server crash and recovery
 *   Close server service without cleanup
 *   Reset all connections
 *   Restart service
 *
 *  Step 6: Configure recovered server with the same configuration data
 *   ServerManager stub requests the server to configure.
 *   Expect that server is configured correctly
 *
 *  Step 7: Connect Rialto Client stub
 *   Rialto Client tries to connect to socket provided by Rialto Server
 *   Expect that rialto client can be connected successfully
 *
 *  Step 8: Register Rialto Client Stub in RUNNING state
 *   Rialto Client sends a request to register to Rialto Server
 *   Expect that rialto client can be registered successfully
 *
 *  Step 9: Perform healthcheck procedure to test if connection is stable
 *   ServerManager stub requests the server to check state of all its components
 *   Expect that all server services are not deadlocked.
 *   Expect that PingEvent is received by connected Rialto Client stub.
 *   Rialto Client stub sends Ack message to Rialto Server
 *   Expect that Rialto Server sends Ack to Server Manager Stub
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  Rialto Server <-> Rialto Client connection should be established successfully
 *  using socket file descriptor provided by server manager stub
 *
 * Code:
 */
TEST_F(RecoveryTest, RecoverServerAfterCrash)
{
    // Step 1: Send SetConfiguration message with socket file descriptor
    configureSutWithFdInActiveState();

    // Step 2: Connect Rialto Client stub
    connectClient();

    // Step 3: Register Rialto Client Stub in RUNNING state
    registerClient();

    // Step 4: Perform healthcheck procedure to test if connection is stable
    performHealthcheckProcedure();

    // Step 5: Simulate server crash and recovery
    simulateCrashAndRecovery();

    // Step 6: Configure recovered server with the same configuration data
    configureSutWithFdInActiveState();

    // Step 7: Connect Rialto Client stub
    connectClient();

    // Step 8: Register Rialto Client Stub in RUNNING state
    registerClient();

    // Step 9: Perform healthcheck procedure to test if connection is stable
    performHealthcheckProcedure();
}
} // namespace firebolt::rialto::server::ct
