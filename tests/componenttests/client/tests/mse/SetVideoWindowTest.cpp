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
class SetVideoWindowTest : public ClientComponentTest
{
public:
    uint32_t m_x = 0;
    uint32_t m_y = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    SetVideoWindowTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~SetVideoWindowTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Set Video Window
 * Test Objective:
 *  Test the set video window API.
 *
 * Sequence Diagrams:
 *  Set Video Window Size & Position
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams
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
 *  Step 1: Set Video Window Small
 *   SetVideoWindow to x=528, y=210, width=427, height=240.
 *   Expect that SetVideoWindow propagated to the server.
 *
 *  Step 2: Set Video Window Full HD
 *   SetVideoWindow to x=0, y=0, width=1920, height=1080.
 *   Expect that SetVideoWindow propagated to the server and returns the volume.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Video window can be set to different values without issue.
 *
 * Code:
 */
TEST_F(SetVideoWindowTest, setVideoWindow)
{
    // Step 1: Set Video Window Small
    m_x = 528;
    m_y = 210;
    m_width = 427;
    m_height = 240;
    MediaPipelineTestMethods::shouldSetVideoWindow(m_x, m_y, m_width, m_height);
    MediaPipelineTestMethods::setSetVideoWindow(m_x, m_y, m_width, m_height);

    // Step 2: Set Video Window Full HD
    m_x = 0;
    m_y = 0;
    m_width = 1920;
    m_height = 1080;
    MediaPipelineTestMethods::shouldSetVideoWindow(m_x, m_y, m_width, m_height);
    MediaPipelineTestMethods::setSetVideoWindow(m_x, m_y, m_width, m_height);
}
} // namespace firebolt::rialto::client::ct
