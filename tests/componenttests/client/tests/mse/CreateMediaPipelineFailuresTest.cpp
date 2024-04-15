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
class CreateMediaPipelineFailures : public ClientComponentTest
{
public:
    CreateMediaPipelineFailures() : ClientComponentTest() { ClientComponentTest::startApplicationRunning(); }

    ~CreateMediaPipelineFailures() { ClientComponentTest::stopApplication(); }
};

/*
 * Component Test: Create Media Pipeline Failure
 * Test Objective:
 *  Test the failure to create a media pipeline.
 *
 * Sequence Diagrams:
 *  Create - https://wiki.rdkcentral.com/pages/viewpage.action?pageId=226375556
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
 *
 * Test Steps:
 *  Step 1: Create a new media session failure
 *   Create an instance of MediaPipeline.
 *   Expect that create session api is called on the server and return failure
 *   Check that no exeption is thrown.
 *   Check that no object has been created.
 *
 *  Step 2: Create a new media session after failure
 *   Create an instance of MediaPipeline.
 *   Expect that a session is created on the server.
 *   Check that the object returned is valid.
 *
 *  Step 3: Destroy media session
 *   Destroy instance of MediaPipeline.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Fail to create a media pipeline gracefully and subsequent media pipelines are unaffected.
 *
 * Code:
 */
TEST_F(CreateMediaPipelineFailures, playback)
{
    // Step 1: Create a new media session failure
    MediaPipelineTestMethods::shouldCreateMediaSessionFailure();
    MediaPipelineTestMethods::createMediaPipelineFailure();

    // Step 2: Create a new media session after failure
    MediaPipelineTestMethods::shouldCreateMediaSession();
    MediaPipelineTestMethods::createMediaPipeline();

    // Step 3: Destroy media session
    MediaPipelineTestMethods::shouldDestroyMediaSession();
    MediaPipelineTestMethods::destroyMediaPipeline();
}
} // namespace firebolt::rialto::client::ct
