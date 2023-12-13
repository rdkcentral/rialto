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
class DualVideoPlaybackTest : public ClientComponentTest
{
public:
    DualVideoPlaybackTest() : ClientComponentTest() { ClientComponentTest::startApplicationRunning(); }

    ~DualVideoPlaybackTest() { ClientComponentTest::stopApplication(); }
};

/*
 * Component Test: Dual Video Playback
 * Test Objective:
 *  Test the playback of dual video content. Check that all states are transitioned successfully
 *  and data is written to the buffer.
 *
 * Sequence Diagrams:
 *  Create Secondary Player (westeros) - https://wiki.rdkcentral.com/pages/viewpage.action?pageId=226375556
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
 *  Step 1: Create a new media session for primary video
 *   Create an instance of MediaPipeline.
 *   Expect that a session is created on the server.
 *   Check that the object returned is valid.
 *
 *  Step 2: Load content on primary
 *   Load MSE.
 *   Expect that load is propagated to the server.
 *   Server notifys the client that the NetworkState has changed to BUFFERING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 3: Attach all sources to primary
 *   Attach the video source.
 *   Expect that attach source for video propagated to the server.
 *   Attach the audio source.
 *   Expect that attach source for audio propagated to the server.
 *   Set all sources attached.
 *   Expect that all source attached is propagated to the server.
 *   Server notifys the client that the Playback state has changed to IDLE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 4: Create a new media session for secondary video
 *   Create an instance of MediaPipeline with < max video capabilities.
 *   Expect that a session is created on the server.
 *   Check that the object returned is valid.
 *
 *  Step 5: Load content on secondary
 *   Load MSE.
 *   Expect that load is propagated to the server.
 *   Server notifys the client that the NetworkState has changed to BUFFERING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 6: Attach video only source to secondary
 *   Attach the video source.
 *   Expect that attach source for video propagated to the server.
 *   Set all sources attached.
 *   Expect that all source attached is propagated to the server.
 *   Server notifys the client that the Playback state has changed to IDLE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 7: Buffer to paused state on primary
 *   Write video frames.
 *   Write audio frames.
 *   Server notifys the client that the Network state has changed to BUFFERED.
 *   Expect that the state change notification is propagated to the client.
 *   Server notifys the client that the Playback state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 8: Buffer to paused state on secondary
 *   Write video frames.
 *   Server notifys the client that the Network state has changed to BUFFERED.
 *   Expect that the state change notification is propagated to the client.
 *   Server notifys the client that the Playback state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 9: Play primary
 *   Play the primary AV content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 9: Play secondary
 *   Play the secondary video only content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 10: Buffer more primary content
 *   Write video frames.
 *   Write audio frames.
 *   x3
 *
 *  Step 11: Buffer more secondary content
 *   Write video frames.
 *   x3
 *
 *  Step 12: Terminate the secondary media session
 *   Stop the playback on secondary session.
 *   Expect that stop propagated to the server.
 *   Server notifys the client that the Playback state has changed to STOPPED.
 *   Expect that the state change notification is propagated to the client.
 *   Destroy instance of MediaPipeline.
 *   Expect that the secondary session is destroyed on the server.
 *
 *  Step 13: Terminate the primary media session
 *   Stop the playback on primary session.
 *   Expect that stop propagated to the server.
 *   Server notifys the client that the Playback state has changed to STOPPED.
 *   Expect that the state change notification is propagated to the client.
 *   Destroy instance of MediaPipeline.
 *   Expect that the primary session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are propagated to the server.
 *  Both sessions are handled and played successfully in parallel without conflict.
 *  Data is successfully written to the shared memory.
 *
 * Code:
 */
TEST_F(DualVideoPlaybackTest, playback)
{
    // Step 1: Create a new media session for primary video
    MediaPipelineTestMethods::shouldCreateMediaSession();
    MediaPipelineTestMethods::createMediaPipeline();

    // Step 2: Load content on primary
    MediaPipelineTestMethods::shouldLoad();
    MediaPipelineTestMethods::load();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffering();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffering();

    // Step 3: Attach all sources to primary
    MediaPipelineTestMethods::shouldAttachVideoSource();
    MediaPipelineTestMethods::attachSourceVideo();
    MediaPipelineTestMethods::shouldAttachAudioSource();
    MediaPipelineTestMethods::attachSourceAudio();
    MediaPipelineTestMethods::shouldAllSourcesAttached();
    MediaPipelineTestMethods::allSourcesAttached();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateIdle();
    MediaPipelineTestMethods::sendNotifyPlaybackStateIdle();

    // Step 4: Create a new media session for secondary video
    MediaPipelineTestMethods::shouldCreateMediaSessionSecondary();
    MediaPipelineTestMethods::createMediaPipelineSecondary();

    // Step 5: Load content on secondary
    MediaPipelineTestMethods::shouldLoadSecondary();
    MediaPipelineTestMethods::loadSecondary();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBufferingSecondary();
    MediaPipelineTestMethods::sendNotifyNetworkStateBufferingSecondary();

    // Step 6: Attach video only source to secondary
    MediaPipelineTestMethods::shouldAttachVideoSourceSecondary();
    MediaPipelineTestMethods::attachSourceVideoSecondary();
    MediaPipelineTestMethods::shouldAllSourcesAttachedSecondary();
    MediaPipelineTestMethods::allSourcesAttachedSecondary();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateIdleSecondary();
    MediaPipelineTestMethods::sendNotifyPlaybackStateIdleSecondary();

    // Step 7: Buffer to paused state on primary
    MediaPipelineTestMethods::writeVideoFrames();
    MediaPipelineTestMethods::writeAudioFrames();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 8: Buffer to paused state on secondary
    MediaPipelineTestMethods::writeVideoFramesSecondary();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBufferedSecondary();
    MediaPipelineTestMethods::sendNotifyNetworkStateBufferedSecondary();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePausedSecondary();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePausedSecondary();

    // Step 9: Play primary
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 9: Play secondary
    MediaPipelineTestMethods::shouldPlaySecondary();
    MediaPipelineTestMethods::playSecondary();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlayingSecondary();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlayingSecondary();

    // Step 10: Buffer more primary content
    MediaPipelineTestMethods::writeVideoFrames();
    MediaPipelineTestMethods::writeAudioFrames();
    MediaPipelineTestMethods::writeVideoFrames();
    MediaPipelineTestMethods::writeAudioFrames();
    MediaPipelineTestMethods::writeVideoFrames();
    MediaPipelineTestMethods::writeAudioFrames();

    // Step 11: Buffer more secondary content
    MediaPipelineTestMethods::writeVideoFramesSecondary();
    MediaPipelineTestMethods::writeVideoFramesSecondary();
    MediaPipelineTestMethods::writeVideoFramesSecondary();

    // Step 12: Terminate the secondary media session
    MediaPipelineTestMethods::shouldRemoveVideoSourceSecondary();
    MediaPipelineTestMethods::removeSourceVideoSecondary();
    MediaPipelineTestMethods::shouldStopSecondary();
    MediaPipelineTestMethods::stopSecondary();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateStoppedSecondary();
    MediaPipelineTestMethods::sendNotifyPlaybackStateStoppedSecondary();
    MediaPipelineTestMethods::shouldDestroyMediaSessionSecondary();
    MediaPipelineTestMethods::destroyMediaPipelineSecondary();

    // Step 13: Terminate the primary media session
    MediaPipelineTestMethods::shouldRemoveVideoSource();
    MediaPipelineTestMethods::removeSourceVideo();
    MediaPipelineTestMethods::shouldRemoveAudioSource();
    MediaPipelineTestMethods::removeSourceAudio();
    MediaPipelineTestMethods::shouldStop();
    MediaPipelineTestMethods::stop();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateStopped();
    MediaPipelineTestMethods::sendNotifyPlaybackStateStopped();
    MediaPipelineTestMethods::shouldDestroyMediaSession();
    MediaPipelineTestMethods::destroyMediaPipeline();
}

} // namespace firebolt::rialto::client::ct
