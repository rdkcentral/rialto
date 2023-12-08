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
class PlayPauseStopFailuresTest : public ClientComponentTest
{
public:
    PlayPauseStopFailuresTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~PlayPauseStopFailuresTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Play Pause and Stop Api Failures
 * Test Objective:
 *  Check that failures returned directly from the Play, Pause and Stop apis and failures returned asyncronously
 *  during server state changes are handled correctly. Subsequent Api requests after failures are successful.
 *
 * Sequence Diagrams:
 *  Start/Resume Playback, Pause Playback, Stop - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Step 1: Play api failure
 *   Play the content.
 *   Expect that play propagated to the server and return failure.
 *
 *  Step 2: Playing state failure
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to FAILURE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 3: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 4: Pause api failure
 *   Pause the content.
 *   Expect that pause propagated to the server and return failure.
 *
 *  Step 5: Paused state failure
 *   Pause the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to FAILURE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 6: Pause
 *   Pause the content.
 *   Expect that pause propagated to the server.
 *   Server notifys the client that the Playback state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 7: Stop api failure
 *   Stop the content.
 *   Expect that stop propagated to the server and return failure.
 *
 *  Step 8: Stopped state failure
 *   Stop the content.
 *   Expect that stop propagated to the server.
 *   Server notifys the client that the Playback state has changed to FAILURE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 9: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the server.
 *   Server notifys the client that the Playback state has changed to STOPPED.
 *   Expect that the state change notification is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  All failures are notified to the calling application.
 *  Failures are recoverable.
 *
 * Code:
 */
TEST_F(PlayPauseStopFailuresTest, playbackFailures)
{
    // Step 1: Play api failure
    MediaPipelineTestMethods::shouldPlayWithFailure();
    MediaPipelineTestMethods::playFailure();

    // Step 2: Playing state failure
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateFailure();
    MediaPipelineTestMethods::sendNotifyPlaybackStateFailure();

    // Step 3: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 4: Pause api failure
    MediaPipelineTestMethods::shouldPauseWithFailure();
    MediaPipelineTestMethods::pauseFailure();

    // Step 5: Paused state failure
    MediaPipelineTestMethods::shouldPause();
    MediaPipelineTestMethods::pause();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateFailure();
    MediaPipelineTestMethods::sendNotifyPlaybackStateFailure();

    // Step 6: Pause
    MediaPipelineTestMethods::shouldPause();
    MediaPipelineTestMethods::pause();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 7: Stop api failure
    MediaPipelineTestMethods::shouldStopWithFailure();
    MediaPipelineTestMethods::stopFailure();

    // Step 8: Stopped state failure
    MediaPipelineTestMethods::shouldStop();
    MediaPipelineTestMethods::stop();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateFailure();
    MediaPipelineTestMethods::sendNotifyPlaybackStateFailure();

    // Step 9: Stop
    MediaPipelineTestMethods::shouldStop();
    MediaPipelineTestMethods::stop();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateStopped();
    MediaPipelineTestMethods::sendNotifyPlaybackStateStopped();
}
} // namespace firebolt::rialto::client::ct
