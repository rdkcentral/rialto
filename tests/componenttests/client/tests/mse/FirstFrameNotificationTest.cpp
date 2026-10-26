/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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
class FirstFrameNotificationTest : public ClientComponentTest
{
public:
    FirstFrameNotificationTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();

        MediaPipelineTestMethods::shouldPlay();
        MediaPipelineTestMethods::play();
        MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
        MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();
    }

    ~FirstFrameNotificationTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: First frame notification
 * Test Objective:
 *  Test the first frame notification for video and audio sources.
 *
 * Sequence Diagrams:
 *  First frame notification
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
 *  Initalise a audio video media session playing.
 *
 * Test Steps:
 *  Step 1: Notify first frame received for video
 *   Server notifies the client first frame received with source id video.
 *   Expect that the first frame notification is propagated to the client.
 *
 *  Step 2: Notify first frame received for audio
 *   Server notifies the client first frame received with source id audio.
 *   Expect that the first frame notification is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  First frame received notification is propagated to the application.
 *
 * Code:
 */
TEST_F(FirstFrameNotificationTest, notification)
{
    // Step 1: Notify first frame received for video
    MediaPipelineTestMethods::shouldNotifyFirstFrameReceivedVideo();
    MediaPipelineTestMethods::sendNotifyFirstFrameReceivedVideo();

    // Step 2: Notify first frame received for audio
    MediaPipelineTestMethods::shouldNotifyFirstFrameReceivedAudio();
    MediaPipelineTestMethods::sendNotifyFirstFrameReceivedAudio();
}
} // namespace firebolt::rialto::client::ct
