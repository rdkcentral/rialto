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

#include "ControlTest.h"
#include "ExpectMessage.h"
#include "controlmodule.pb.h"

namespace firebolt::rialto::server::ct
{
/*
 * Component Test: Application state change from RUNNING->INACTIVE->RUNNING
 * Test Objective:
 *  Test the full lifecycle of an application and verify that the client is always notified.
 *
 * Sequence Diagrams:
 *  Start Application in Running State, Application state change: Running to Inactive,
 *  Switch Application from Inactive to Running State, Stop Application from Running State
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Application+Session+Management
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
 *  Step 1: Change state to INACTIVE
 *   ServerManager requests the server to change the state to INACTIVE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 2: Change state to RUNNING
 *   ServerManager requests the server to change the state to ACTIVE.
 *   Expect that the state change notification to RUNNING is propagated to the client.
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  The lifecycle of an application is successfully negotiated and notfied to
 *  listening clients.
 *
 * Code:
 */
TEST_F(ControlTest, ShouldNotifyState)
{
    // Step 1: Change state to INACTIVE
    ExpectMessage<ApplicationStateChangeEvent> m_expectedInactiveNotification{m_clientStub};

    setStateInactive();

    auto inactiveNotification{m_expectedInactiveNotification.getMessage()};
    ASSERT_TRUE(inactiveNotification);
    EXPECT_EQ(inactiveNotification->application_state(), ApplicationStateChangeEvent_ApplicationState_INACTIVE);

    // Step 2: Change state to RUNNING
    ExpectMessage<ApplicationStateChangeEvent> m_expectedRunningNotification{m_clientStub};

    setStateActive();

    auto runningNotification{m_expectedRunningNotification.getMessage()};
    ASSERT_TRUE(runningNotification);
    EXPECT_EQ(runningNotification->application_state(), ApplicationStateChangeEvent_ApplicationState_RUNNING);
}
} // namespace firebolt::rialto::server::ct
