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
class RemoveAudioPlaybackTest : public ClientComponentTest
{
public:
    RemoveAudioPlaybackTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~RemoveAudioPlaybackTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Playback content when audio source has been removed and reattached.
 * Test Objective:
 *  Test that video only playback can continue if the audio source is removed, and that audio can be restarted
 *  when it is reattached.
 *
 * Sequence Diagrams:
 *  Rialto Dynamic Audio Stream Switching
 *   - https://wiki.rdkcentral.com/pages/viewpage.action?spaceKey=ASP&title=Rialto+Dynamic+Audio+Stream+Switching
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
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 2: Pause
 *   Pause the content.
 *   Expect that pause propagated to the server.
 *   Server notifys the client that the Playback state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 3: Remove Audio Source
 *   Remove the audio source.
 *   Expect that remove source for audio propagated to the server.
 *
 *  Step 4: Write video frames
 *   Write video frames.
 *
 *  Step 5: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 6: Pause
 *   Pause the content.
 *   Expect that pause propagated to the server.
 *   Server notifys the client that the Playback state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 7: Reattach audio source
 *   Attach the audio source again.
 *   Expect that attach source for audio propagated to the server.
 *
 *  Step 8: Write video and audio frames
 *   Write video frames.
 *   Write audio frames.
 *
 *  Step 9: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Video playback is unaffected when audio source is removed mid playback.
 *  Audio can be reattached and playback of both video and audio resumes.
 *
 * Code:
 */
TEST_F(RemoveAudioPlaybackTest, removeAudio)
{
    // Step 1: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 2: Pause
    MediaPipelineTestMethods::shouldPause();
    MediaPipelineTestMethods::pause();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 3: Remove Audio Source
    MediaPipelineTestMethods::shouldRemoveAudioSource();
    MediaPipelineTestMethods::removeSourceAudio();

    // Step 4: Write video frames
    writeVideoFrames();

    // Step 5: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 6: Pause
    MediaPipelineTestMethods::shouldPause();
    MediaPipelineTestMethods::pause();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 7: Reattach audio source
    MediaPipelineTestMethods::shouldAttachAudioSource();
    MediaPipelineTestMethods::attachSourceAudio();

    // Step 8: Write video and audio frames
    writeVideoFrames();
    writeAudioFrames();

    // Step 9: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();
}
} // namespace firebolt::rialto::client::ct
