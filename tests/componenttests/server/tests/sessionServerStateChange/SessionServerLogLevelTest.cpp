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
#include "RialtoLogging.h"
#include "RialtoServerComponentTest.h"

using namespace firebolt::rialto::server::ct;
using namespace firebolt::rialto::logging;

class SessionServerLogLevelTest : public RialtoServerComponentTest
{
public:
    SessionServerLogLevelTest() = default;
    ~SessionServerLogLevelTest() override = default;

    void setLogLevels()
    {
        ::rialto::SetLogLevelsRequest request{createSetLogLevelsRequest()};

        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_DEFAULT), RIALTO_DEBUG_LEVEL_DEFAULT);
        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_CLIENT), RIALTO_DEBUG_LEVEL_DEFAULT);
        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_SERVER), RIALTO_DEBUG_LEVEL_DEFAULT);
        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_IPC), RIALTO_DEBUG_LEVEL_DEFAULT);
        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_SERVER_MANAGER), RIALTO_DEBUG_LEVEL_DEFAULT);
        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_COMMON), RIALTO_DEBUG_LEVEL_DEFAULT);

        ConfigureAction<::firebolt::rialto::server::ct::SetLogLevelsRequest>(m_serverManagerStub)
            .send(request)
            .expectSuccess();

        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_DEFAULT), RIALTO_DEBUG_LEVEL_FATAL);
        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_CLIENT), RIALTO_DEBUG_LEVEL_ERROR);
        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_SERVER), RIALTO_DEBUG_LEVEL_WARNING);
        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_IPC), RIALTO_DEBUG_LEVEL_MILESTONE);
        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_SERVER_MANAGER), RIALTO_DEBUG_LEVEL_INFO);
        ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_COMMON), RIALTO_DEBUG_LEVEL_DEBUG);
    }
};

/*
 * Component Test: RialtoApplicationSessionServer will receive ping from RialtoServerManager and respond
 * Test Objective:
 *   RialtoApplicationSessionServer component is under test and will receive ping from stubbed RialtoServerManager
 *   and respond to RialtoServerManager with an acknowledgement
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
TEST_F(SessionServerLogLevelTest, ShouldSetLogLevels)
{
    // Step 1: monitor socket creation
    willConfigureSocket();

    // Step 2: send a SetConfiguration message to make server active; and then expect StateChangedEvent message
    configureSutInActiveState();

    // Step 3: Perform ping test
    setLogLevels();
}
