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
 * Component Test: <Test Description>
 * Test Objective:
 *  <Detailed Test Description>
 *
 * Sequence Diagrams:
 *  <Links To Relevant Sequence Diagrams>
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: <Component Tested>
 *
 * Test Initialize:
 *  <Test Initialization Steps>
 *
 * Test Steps:
 *  Step 1: <Test Step Name>
 *   <Test Step Description>
 *
 *  <Further Test Steps>
 *
 * Test Teardown:
 *  <Test Termination Steps>
 *
 * Expected Results:
 *  <Description Of Results To Expect>
 *
 * Code:
 */
TEST_F(SessionServerLogLevelTest, ShouldSetLogLevels)
{
    willConfigureSocket();
    configureSutInActiveState();
    setLogLevels();
}
