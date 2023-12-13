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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "ControlModuleStub.h"
#include "ExpectMessage.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"

using namespace firebolt::rialto::server::ct;

class SessionServerHealthCheckTest : public RialtoServerComponentTest
{
public:
    SessionServerHealthCheckTest() = default;
    ~SessionServerHealthCheckTest() override = default;

    void sendAndRecievePing()
    {
        ::google::protobuf::int32 kMyId{3};
        ::rialto::PingRequest request{createPingRequest(kMyId)};

        ExpectMessage<::rialto::AckEvent> expectedMessage(m_serverManagerStub);

        ConfigureAction<::firebolt::rialto::server::ct::Ping>(m_serverManagerStub).send(request).expectSuccess();

        auto receivedMessage = expectedMessage.getMessage();
        ASSERT_TRUE(receivedMessage);
        EXPECT_EQ(receivedMessage->id(), kMyId);
        EXPECT_EQ(receivedMessage->success(), true);
    }
};

/*
 * Component Test: RialtoApplicationSessionServer will receive ping from RialtoServerManager and respond
 * Test Objective:
 *   RialtoApplicationSessionServer component is under test and will receive ping from stubbed RialtoServerManager
 *   and respond to RialtoServerManager with an acknowledgement
 *
 * Sequence Diagrams:
 *  https://wiki.rdkcentral.com/pages/viewpage.action?spaceKey=ASP&title=Healthcheck+mechanism
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: RialtoApplicationSessionServer with stubs for RialtoClient and RialtoServerManager
 *
 * Test Initialize:
 *   RialtoServerComponentTest::RialtoServerComponentTest() will set up wrappers and
 *      starts the application server running in its own thread
 *
 * Test Steps:
 *  Step 1: monitor socket creation
 *      sets up the Linux Wrapper so that socket creation is expected and monitored
 *
 *  Step 2: send a SetConfiguration message to make server active; and then expect StateChangedEvent message
 *      There doesn't seem to be a sequence diagram for this
 *
 *  Step 3: Perform ping test
 *      In the sequence diagram "Ping/Ack" this implements steps 1 and 6
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are handled by the server.
 *
 * Code:
 */
TEST_F(SessionServerHealthCheckTest, ShouldAcknowledgePing)
{
    // Step 1: monitor socket creation
    willConfigureSocket();

    // Step 2: send a SetConfiguration message to make server active; and then expect StateChangedEvent message
    configureSutInActiveState();

    // Step 3: Perform ping test
    sendAndRecievePing();
}
