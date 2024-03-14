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
 */

#include "ClientComponentTest.h"
#include <gtest/gtest.h>

namespace firebolt::rialto::client::ct
{
class SetSourcePositionTest : public ClientComponentTest
{
public:
    SetSourcePositionTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~SetSourcePositionTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Set Source Position success
 * Test Objective:
 *  Test that Set Source Position is successfully handled.
 *
 * Sequence Diagrams:
 *  Set Source Position - https://wiki.rdkcentral.com/display/ASP/Rialto+Flush+and+Seek+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline
 *
 * Test Initialize:
 *  Create memory region for the shared buffer.
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *  Initalise a audio video media session paused and prerolled.
 *
 * Test Steps:
 *  Step 1: Set Source Position
 *   Server notifies the client that set source position procedure has been queued in server
 *   Expect that the procedure status is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Set Source Position is handled and forwarded to the server.
 *
 * Code:
 */
TEST_F(SetSourcePositionTest, setSourcePositionSuccess)
{
    // Step 1: Set Source Position
    MediaPipelineTestMethods::shouldSetSourcePosition();
    MediaPipelineTestMethods::setSourcePosition();
}

/*
 * Component Test: Set Source Position failures
 * Test Objective:
 *  Check that failures returned directly from the Set Source Position api are handled correctly.
 *
 * Sequence Diagrams:
 *  Set Source Position - https://wiki.rdkcentral.com/display/ASP/Rialto+Flush+and+Seek+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline
 *
 * Test Initialize:
 *  Create memory region for the shared buffer.
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *  Initalise a audio video media session paused and prerolled.
 *
 * Test Steps:
 *  Step 1: Set Source Position failure
 *   Server notifies the client that Set Source Position failed
 *   Expect that the procedure status is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  FAILURE is notifed if set source position fails.
 *
 * Code:
 */
TEST_F(SetSourcePositionTest, failures)
{
    // Step 1: Set Source Position failure
    MediaPipelineTestMethods::shouldFailToSetSourcePosition();
    MediaPipelineTestMethods::setSourcePositionFailure();
}
} // namespace firebolt::rialto::client::ct
