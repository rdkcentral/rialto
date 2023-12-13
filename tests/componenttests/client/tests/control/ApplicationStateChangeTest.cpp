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

#include "ClientComponentTest.h"
#include <gtest/gtest.h>

namespace firebolt::rialto::client::ct
{
class ApplicationStateChangeTest : public ClientComponentTest
{
};

/*
 * Component Test: Application state change from INACTIVE->RUNNING->INACTIVE
 * Test Objective:
 *  Test the full lifecycle of an application and verify that the client is always notified,
 *  and the shared memory initalised correctly.
 *
 * Sequence Diagrams:
 *  Start Application in Inactive State, Application state change: Inactive to Running,
 *  Switch Application from Running to Inactive State, Stop Application from Inactive State
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Application+Session+Management
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: Control
 *
 * Test Initialize:
 *  Create memory region for the shared buffer.
 *  Create a server that handles Control IPC requests.
 *
 * Test Steps:
 *  Step 1: Initialize control
 *   Create an instance of Control.
 *   Check that the object returned is valid.
 *
 *  Step 2: Register client
 *   Register a client listener.
 *   Check that the initial state is UNKNOWN.
 *
 *  Step 3: Change state to INACTIVE
 *   Server notifys the client that the state has changed to INACTIVE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 4: Change state to RUNNING
 *   Server notifys the client that the state has changed to RUNNING.
 *   Expect that the state change notification is propagated to the client.
 *   Expect that the shared memory region is fetched from the server.
 *
 *  Step 5: Change state to INACTIVE
 *   Server notifys the client that the state has changed to INACTIVE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 6: Disconnect the server
 *   Server notifys the client that it has disconnected.
 *   Expect that the state is changed to UNKNOWN in the client.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  The lifecycle of an application is successfully negotiated and notfied to
 *  listening clients.
 *
 * Code:
 */
TEST_F(ApplicationStateChangeTest, lifecycle)
{
    // Step 1: Initialize control
    ControlTestMethods::createControl();

    // Step 2: Register client
    ControlTestMethods::shouldRegisterClient();
    ControlTestMethods::registerClient();

    // Step 3: Change state to INACTIVE
    ControlTestMethods::shouldNotifyApplicationStateInactive();
    ControlTestMethods::sendNotifyApplicationStateInactive();

    // Step 4: Change state to RUNNING
    ControlTestMethods::shouldNotifyApplicationStateRunning();
    ControlTestMethods::sendNotifyApplicationStateRunning();

    // Step 5: Change state to INACTIVE
    ControlTestMethods::shouldNotifyApplicationStateInactive();
    ControlTestMethods::sendNotifyApplicationStateInactive();

    // Step 6: Disconnect the server
    ControlTestMethods::shouldNotifyApplicationStateUnknown();
    ClientComponentTest::disconnectServer();
    ClientComponentTest::waitEvent();
}
} // namespace firebolt::rialto::client::ct
