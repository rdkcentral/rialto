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
class PlaybackErrorNotificationTest : public ClientComponentTest
{
public:
    PlaybackErrorNotificationTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();

        // Play
        MediaPipelineTestMethods::shouldPlay();
        MediaPipelineTestMethods::play();
        MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
        MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();
    }

    ~PlaybackErrorNotificationTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: PlaybackError Notification
 * Test Objective:
 *  Test the PlaybackError notification for all sources.
 *
 * Sequence Diagrams:
 *  TODO
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
 *  Step 1: Notify playback error audio
 *   Server notifies the client playback error for audio.
 *   Expect that the playback error notification is propagated to the client.
 *   Check source & playback error.
 *
 *  Step 2: Notify playback error video
 *   Server notify the client playback error for video.
 *   Expect that the playback error notification is propagated to the client.
 *   Check source & playback error.
 *
 * Test Tear-down:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  PlaybackError notifications for audio or video are handled by rialto client and the information is sent to the application.
 *
 * Code:
 */
TEST_F(PlaybackErrorNotificationTest, notifications)
{
    // Step 1: Notify playback error audio
    MediaPipelineTestMethods::shouldNotifyPlaybackErrorAudio();
    MediaPipelineTestMethods::sendNotifyPlaybackErrorAudio();

    // Step 2: Notify playback error video
    MediaPipelineTestMethods::shouldNotifyPlaybackErrorVideo();
    MediaPipelineTestMethods::sendNotifyPlaybackErrorVideo();
}
} // namespace firebolt::rialto::client::ct
