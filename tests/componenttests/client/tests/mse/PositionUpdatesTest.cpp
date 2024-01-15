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
class PositionUpdatesTest : public ClientComponentTest
{
public:
    int64_t m_position = 10;

    PositionUpdatesTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~PositionUpdatesTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Position reporting
 * Test Objective:
 *  Test position can be notified by the server or fetched by the client.
 *
 * Sequence Diagrams:
 *  Position Reporting - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Step 1: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifies the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 2: Notify position
 *   Server notifies the client of position 10.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 3: Get position
 *   GetPosition.
 *   Expect that GetPosition propagated to the server and returns the position 10.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Position is notified correctly from the server.
 *  Position is retrieved correctly from the server.
 *
 * Code:
 */
TEST_F(PositionUpdatesTest, positionUpdates)
{
    // Step 1: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 2: Notify position
    m_position = 10;
    MediaPipelineTestMethods::shouldNotifyPosition(m_position);
    MediaPipelineTestMethods::sendNotifyPositionChanged(m_position);

    // Step 3: Get position
    MediaPipelineTestMethods::shouldGetPosition(m_position);
    MediaPipelineTestMethods::getPosition(m_position);
}
} // namespace firebolt::rialto::client::ct
