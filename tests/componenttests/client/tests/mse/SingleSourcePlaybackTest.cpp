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
class SingleSourcePlaybackTest : public ClientComponentTest
{
public:
    int32_t m_segmentId{-1};

    SingleSourcePlaybackTest() : ClientComponentTest() { ClientComponentTest::startApplicationRunning(); }

    ~SingleSourcePlaybackTest() { ClientComponentTest::stopApplication(); }
};

/*
 * Component Test: Audio Only Playback
 * Test Objective:
 *  Test tha playback of single source audio MSE content. Check that all states are transitioned successfully
 *  and data is written to the buffer.
 *
 * Sequence Diagrams:
 *  
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
 *  Step 1: Create a new media session
 *   Create an instance of MediaPipeline.
 *   Expect that a session is created on the server.
 *   Check that the object returned is valid.
 *
 *  Step 2: Load content
 *   Load MSE.
 *   Expect that load is propagated to the server.
 *   Server notifys the client that the NetworkState has changed to BUFFERING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 3: Attach audio source
 *   Attach the audio source.
 *   Expect that attach source for audio propagated to the server.
 *   Set all sources attached.
 *   Expect that all source attached is propagated to the server.
 *   Server notifys the client that the Playback state has changed to IDLE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 4: Buffer to paused state
 *   Write audio frames.
 *   Server notifys the client that the Network state has changed to BUFFERED.
 *   Expect that the state change notification is propagated to the client.
 *   Server notifys the client that the Playback state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 5: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 6: Buffer more frames while playing
 *   Write audio frames.
 *   x3
 *
 *  Step 7: Remove audio source
 *   Remove the audio source.
 *   Expect that remove source for audio propagated to the server.
 *
 *  Step 8: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the server.
 *   Server notifys the client that the Playback state has changed to STOPPED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 9: Destroy media session
 *   Destroy instance of MediaPipeline.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are propagated to the server.
 *  The state of the MediaPipeline is successfully negotiationed in the audio only playback scenario.
 *  Data is successfully written to the shared memory for both audio.
 *
 * Code:
 */
TEST_F(SingleSourcePlaybackTest, audioOnly)
{
    // Step 1: Create a new media session
    MediaPipelineTestMethods::shouldCreateMediaSession();
    MediaPipelineTestMethods::createMediaPipeline();

    // Step 2: Load content
    MediaPipelineTestMethods::shouldLoad();
    MediaPipelineTestMethods::load();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffering();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffering();

    // Step 3: Attach audio source
    MediaPipelineTestMethods::shouldAttachAudioSource();
    MediaPipelineTestMethods::attachSourceAudio();
    MediaPipelineTestMethods::shouldAllSourcesAttached();
    MediaPipelineTestMethods::allSourcesAttached();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateIdle();
    MediaPipelineTestMethods::sendNotifyPlaybackStateIdle();

    // Step 4: Buffer to paused state
    MediaPipelineTestMethods::writeAudioFrames();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 5: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 6: Buffer more frames
    MediaPipelineTestMethods::writeAudioFrames();
    MediaPipelineTestMethods::writeAudioFrames();
    MediaPipelineTestMethods::writeAudioFrames();

    // Step 7: Remove sources
    MediaPipelineTestMethods::shouldRemoveAudioSource();
    MediaPipelineTestMethods::removeSourceAudio();

    // Step 8: Stop
    MediaPipelineTestMethods::shouldStop();
    MediaPipelineTestMethods::stop();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateStopped();
    MediaPipelineTestMethods::sendNotifyPlaybackStateStopped();

    // Step 9: Destroy media session
    MediaPipelineTestMethods::shouldDestroyMediaSession();
    MediaPipelineTestMethods::destroyMediaPipeline();
}

/*
 * Component Test: Video Only Playback
 * Test Objective:
 *  Test tha playback of single source video MSE content. Check that all states are transitioned successfully
 *  and data is written to the buffer.
 *
 * Sequence Diagrams:
 *  
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
 *  Step 1: Create a new media session
 *   Create an instance of MediaPipeline.
 *   Expect that a session is created on the server.
 *   Check that the object returned is valid.
 *
 *  Step 2: Load content
 *   Load MSE.
 *   Expect that load is propagated to the server.
 *   Server notifys the client that the NetworkState has changed to BUFFERING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 3: Attach video source
 *   Attach the video source.
 *   Expect that attach source for video propagated to the server.
 *   Set all sources attached.
 *   Expect that all source attached is propagated to the server.
 *   Server notifys the client that the Playback state has changed to IDLE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 4: Buffer to paused state
 *   Write video frames.
 *   Server notifys the client that the Network state has changed to BUFFERED.
 *   Expect that the state change notification is propagated to the client.
 *   Server notifys the client that the Playback state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 5: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 6: Buffer more frames while playing
 *   Write video frames.
 *   x3
 *
 *  Step 7: Remove video source
 *   Remove the video source.
 *   Expect that remove source for video propagated to the server.
 *
 *  Step 8: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the server.
 *   Server notifys the client that the Playback state has changed to STOPPED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 9: Destroy media session
 *   Destroy instance of MediaPipeline.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are propagated to the server.
 *  The state of the MediaPipeline is successfully negotiationed in the audio only playback scenario.
 *  Data is successfully written to the shared memory.
 *
 * Code:
 */
TEST_F(SingleSourcePlaybackTest, videoOnly)
{
    // Step 1: Create a new media session
    MediaPipelineTestMethods::shouldCreateMediaSession();
    MediaPipelineTestMethods::createMediaPipeline();

    // Step 2: Load content
    MediaPipelineTestMethods::shouldLoad();
    MediaPipelineTestMethods::load();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffering();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffering();

    // Step 3: Attach video source
    MediaPipelineTestMethods::shouldAttachVideoSource();
    MediaPipelineTestMethods::attachSourceVideo();
    MediaPipelineTestMethods::shouldAllSourcesAttached();
    MediaPipelineTestMethods::allSourcesAttached();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateIdle();
    MediaPipelineTestMethods::sendNotifyPlaybackStateIdle();

    // Step 4: Buffer to paused state
    MediaPipelineTestMethods::writeVideoFrames();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 5: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 6: Buffer more frames while playing
    MediaPipelineTestMethods::writeVideoFrames();
    MediaPipelineTestMethods::writeVideoFrames();
    MediaPipelineTestMethods::writeVideoFrames();

    // Step 7: Remove sources
    MediaPipelineTestMethods::shouldRemoveVideoSource();
    MediaPipelineTestMethods::removeSourceVideo();

    // Step 8: Stop
    MediaPipelineTestMethods::shouldStop();
    MediaPipelineTestMethods::stop();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateStopped();
    MediaPipelineTestMethods::sendNotifyPlaybackStateStopped();

    // Step 9: Destroy media session
    MediaPipelineTestMethods::shouldDestroyMediaSession();
    MediaPipelineTestMethods::destroyMediaPipeline();
}
} // namespace firebolt::rialto::client::ct
