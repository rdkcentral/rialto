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
#include "ExpectMessage.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"

using namespace firebolt::rialto::server::ct;

class SessionServerStateChangeTest : public RialtoServerComponentTest
{
public:
    SessionServerStateChangeTest() = default;
    ~SessionServerStateChangeTest() override = default;

    void configureSutInInactiveState()
    {
        ::rialto::SetConfigurationRequest request{createGenericSetConfigurationReq()};
        request.set_initialsessionserverstate(::rialto::SessionServerState::INACTIVE);

        ExpectMessage<::rialto::StateChangedEvent> expectedMessage(m_serverManagerStub);

        ConfigureAction<SetConfiguration>(m_serverManagerStub).send(request).expectSuccess();

        auto receivedMessage = expectedMessage.getMessage();
        ASSERT_TRUE(receivedMessage);
        EXPECT_EQ(receivedMessage->sessionserverstate(), ::rialto::SessionServerState::INACTIVE);
    }
};

/*
 * Component Test: RialtoApplicationSessionServer goes from Not Running -> Unitialized (server preloading) -> active sequence
 * Test Objective:
 *  Test that the server can be successfully started in an uninitialised state and then activated
 *
 * Sequence Diagrams:
 *  https://wiki.rdkcentral.com/pages/viewpage.action?spaceKey=ASP&title=Rialto+Application+Session+Management
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
 *
 * Test Steps:
 *  Step A1: send a SetConfiguration message to make server inactive; and then expect a StateChangedEvent message
 *      - send a SetConfiguration message
 *        within sequence diagram "Unitialized to Inactive" this implements steps 2 and 3
 *      - expect a StateChangedEvent message
 *        within sequence diagram "Unitialized to Inactive" this implements step 7
 *
 *  Step A2: send a SetStateRequest message to make server active; and then expect StateChangedEvent message
 *      - send a SetStateRequest message
 *        within sequence diagram "Inactive to Active" this implements steps 2 and 3
 *      - expect StateChangedEvent message
 *        within sequence diagram "Inactive to Active" this implements step 7
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are handled by the server.
 *
 * Code:
 */
TEST_F(SessionServerStateChangeTest, ShouldChangeFromInactiveToActive)
{
    // Step A1: send a SetConfiguration message to make server inactive; and then expect a StateChangedEvent message
    configureSutInInactiveState();

    // Step A2: send a SetStateRequest message to make server active; and then expect StateChangedEvent message
    setStateActive();
}

/*
 * Component Test: RialtoApplicationSessionServer goes from Not Running -> Initialized (active state) -> inactive
 * sequence Test Objective: Test that the server can be successfully started in an active state and then de-activated
 *
 * Sequence Diagrams:
 *  https://wiki.rdkcentral.com/pages/viewpage.action?spaceKey=ASP&title=Rialto+Application+Session+Management
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
 *  Step B1: send a SetConfiguration message to make server active; and then expect StateChangedEvent message
 *      There doesn't seem to be a sequence diagram for this
 *
 *  Step B2: send a SetStateRequest message to make server inactive; and then expect StateChangedEvent message
 *      - send a SetStateRequest message
 *        within sequence diagram "Active to Inactive" this implements steps 2 and 3
 *      - expect StateChangedEvent message
 *        within sequence diagram "Active to Inactive" this implements step 9
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are handled by the server.
 *
 * Code:
 */
TEST_F(SessionServerStateChangeTest, ShouldChangeFromActiveToInactive)
{
    // Step B1: send a SetConfiguration message to make server active; and then expect StateChangedEvent message
    configureSutInActiveState();

    // Step B2: send a SetStateRequest message to make server inactive; and then expect StateChangedEvent message
    setStateInactive();
}
