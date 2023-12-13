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
class AudioSwitchingTest : public ClientComponentTest
{
public:
    AudioSwitchingTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~AudioSwitchingTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Audio Switching
 * Test Objective:
 *  Test that media pipeline can switch between audio sources multiple times when paused or playing.
 *
 * Sequence Diagrams:
 *  Audio Stream Switching - https://wiki.rdkcentral.com/display/ASP/Rialto+Dynamic+Audio+Stream+Switching
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
 *  Step 1: Remove audio source
 *   Remove the audio source.
 *   Expect that remove source for audio propagated to the server.
 *
 *  Step 2: Attach mpeg audio source
 *   Attach the new audio source.
 *   Expect that attach source for audio propagated to the server.
 *
 *  Step 3: Write audio frame
 *   Write audio frames.
 *
 *  Step 4: Remove audio source
 *   Remove the audio source.
 *   Expect that remove source for audio propagated to the server.
 *
 *  Step 5: Attach eacs audio source
 *   Attach the new audio source.
 *   Expect that attach source for audio propagated to the server.
 *
 *  Step 6: Write audio frame
 *   Write audio frames.
 *
 *  Step 7: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 8: Remove audio source
 *   Remove the audio source.
 *   Expect that remove source for audio propagated to the server.
 *
 *  Step 9: Attach mp4 audio source
 *   Attach the new audio source.
 *   Expect that attach source for audio propagated to the server.
 *
 *  Step 10: Write audio frame
 *   Write audio frames.
 *
 *  Step 11: Remove audio source
 *   Remove the audio source.
 *   Expect that remove source for audio propagated to the server.
 *
 *  Step 12: Attach eacs audio source
 *   Attach the new audio source.
 *   Expect that attach source for audio propagated to the server.
 *
 *  Step 13: Write audio frame
 *   Write audio frames.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  The audio source can be removed and added during ongoing playback (paused/playing).
 *  Minimal disruption of the playback, no state changes.
 *
 * Code:
 */
TEST_F(AudioSwitchingTest, multiSwitching)
{
    // Step 1: Remove audio source
    MediaPipelineTestMethods::shouldRemoveAudioSource();
    MediaPipelineTestMethods::removeSourceAudio();

    // Step 2: Attach mpeg audio source
    MediaPipelineTestMethods::shouldAttachAudioSourceMpeg();
    MediaPipelineTestMethods::attachSourceAudioMpeg();

    // Step 3: Write audio frame
    writeAudioFrames();

    // Step 4: Remove audio source
    MediaPipelineTestMethods::shouldRemoveAudioSource();
    MediaPipelineTestMethods::removeSourceAudio();

    // Step 5: Attach eacs audio source
    MediaPipelineTestMethods::shouldAttachAudioSourceEacs();
    MediaPipelineTestMethods::attachSourceAudioEacs();

    // Step 6: Write audio frame
    writeAudioFrames();

    // Step 7: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 8: Remove audio source
    MediaPipelineTestMethods::shouldRemoveAudioSource();
    MediaPipelineTestMethods::removeSourceAudio();

    // Step 9: Attach mp4 audio source
    MediaPipelineTestMethods::shouldAttachAudioSourceMp4();
    MediaPipelineTestMethods::attachSourceAudioMp4();

    // Step 10: Write audio frame
    writeAudioFrames();

    // Step 11: Remove audio source
    MediaPipelineTestMethods::shouldRemoveAudioSource();
    MediaPipelineTestMethods::removeSourceAudio();

    // Step 12: Attach eacs audio source
    MediaPipelineTestMethods::shouldAttachAudioSourceEacs();
    MediaPipelineTestMethods::attachSourceAudioEacs();

    // Step 13: Write audio frame
    writeAudioFrames();
}
} // namespace firebolt::rialto::client::ct
