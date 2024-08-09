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
class ProcessAudioGapTest : public ClientComponentTest
{
public:
    ProcessAudioGapTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~ProcessAudioGapTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Process Audio Gap
 * Test Objective:
 *  Test the Process Audio Gap APIs.
 *
 * Sequence Diagrams:
 *  Process Audio Gap - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams
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
 *  Step 1: Process audio gap
 *   Process audio gap
 *   Expect that Process Audio Gap propagated to the server.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Process Audio Gap request is propagated to the server
 *
 * Code:
 */
TEST_F(ProcessAudioGapTest, success)
{
    // Step 1: Process audio gap
    MediaPipelineTestMethods::shouldProcessAudioGap();
    MediaPipelineTestMethods::processAudioGap();
}

/*
 * Component Test: Process Audio Gap failure
 * Test Objective:
 *  Test the Process Audio Gap APIs.
 *
 * Sequence Diagrams:
 *  Process Audio Gap - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams
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
 *  Step 1: Process audio gap failure
 *   Process audio gap
 *   Expect that Process Audio Gap fails, when server returns an error
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Process Audio Gap request is propagated to the server
 *
 * Code:
 */
TEST_F(ProcessAudioGapTest, failure)
{
    // Step 1: Process audio gap failure
    MediaPipelineTestMethods::shouldFailToProcessAudioGap();
    MediaPipelineTestMethods::processAudioGapFailure();
}
} // namespace firebolt::rialto::client::ct
