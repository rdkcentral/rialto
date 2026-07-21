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

#include "ClientComponentTest.h"
#include <gtest/gtest.h>

namespace firebolt::rialto::client::ct
{
class MuteTest : public ClientComponentTest
{
public:
    double m_isVideoMaster = false;

    MuteTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~MuteTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: IsVideoMaster
 * Test Objective:
 *  Test the IsVideoMaster API.
 *
 * Sequence Diagrams:
 *  IsVideoMaster - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams
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
 *  Step 1: Call IsVideoMaster
 *   Call IsVideoMaster
 *   Expect that IsVideoMaster propagated to the server and returns the IsVideoMaster status.
 *
 *  Step 2: Call IsVideoMaster failure
 *   Call IsVideoMaster
 *   Expect that when IsVideoMaster fails, it returns the error status.
 *
 *  Step 3: Call IsVideoMaster capability
 *   Call IsVideoMaster capability
 *   Expect that IsVideoMaster propagated to the server and returns the IsVideoMaster status.
 *
 *  Step 4: Call IsVideoMaster capability failure
 *   Call IsVideoMaster capability
 *   Expect that when IsVideoMaster fails, it returns the error status.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  IsVideoMaster can be set and got multiple times without issue.
 *
 * Code:
 */
TEST_F(MuteTest, mute)
{
    // Step 1: Call IsVideoMaster
    MediaPipelineTestMethods::shouldCheckIsVideoMaster();
    MediaPipelineTestMethods::isVideoMaster();

    // Step 2: Call IsVideoMaster failure
    MediaPipelineTestMethods::shouldFailToCheckIsVideoMaster();
    MediaPipelineTestMethods::isVideoMasterFailure();
}
} // namespace firebolt::rialto::client::ct
