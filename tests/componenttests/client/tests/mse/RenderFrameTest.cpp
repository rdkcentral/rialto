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
class RenderFrameTest : public ClientComponentTest
{
public:
    RenderFrameTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~RenderFrameTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Render Frame
 * Test Objective:
 *  Test the render frame API.
 *
 * Sequence Diagrams:
 *  Render Frame
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Step 1: Render frame start of playback
 *   RenderFrame.
 *   Expect that RenderFrame propagated to the server.
 *
 *  Step 2: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 3: Render frame failure
 *   RenderFrame.
 *   Expect that RenderFrame propagated to the server and return failure.
 *
 *  Step 4: Pause
 *   Pause the content.
 *   Expect that pause propagated to the server.
 *   Server notifys the client that the Playback state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 5: Seek
 *   SetPosition to position 0s.
 *   Server notifys the client that the Playback state has changed to SEEKING.
 *   Expect that the state change notification is propagated to the client.
 *   Server notifys the client that the Playback state has changed to FLUSHED.
 *   Expect that the state change notification is propagated to the client.
 *   Write audio frames.
 *   Write video frames.
 *   Server notifys the client that the Network state has changed to BUFFERED.
 *   Expect that the state change notification is propagated to the client.
 *   Server notifys the client that the Playback state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 6: Render frame after seek
 *   RenderFrame.
 *   Expect that RenderFrame propagated to the server.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Render frame is successful if playback is paused either at the start of playback or
 *  immediately after a seek operation, failure otherwise.
 *
 * Code:
 */
TEST_F(RenderFrameTest, successAndFailure)
{
    // Step 1: Render frame start of playback
    MediaPipelineTestMethods::shouldRenderFrame();
    MediaPipelineTestMethods::renderFrame();

    // Step 2: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 3: Render frame failure
    MediaPipelineTestMethods::shouldRenderFrameFailure();
    MediaPipelineTestMethods::renderFrameFailure();

    // Step 4: Pause
    MediaPipelineTestMethods::shouldPause();
    MediaPipelineTestMethods::pause();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 5: Seek
    MediaPipelineTestMethods::shouldSetPositionTo0();
    MediaPipelineTestMethods::setPosition0();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateSeeking();
    MediaPipelineTestMethods::sendNotifyPlaybackStateSeeking();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateFlushed();
    MediaPipelineTestMethods::sendNotifyPlaybackStateFlushed();
    MediaPipelineTestMethods::writeAudioFrames();
    MediaPipelineTestMethods::writeVideoFrames();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 6: Render frame after seek
    MediaPipelineTestMethods::shouldRenderFrame();
    MediaPipelineTestMethods::renderFrame();
}
} // namespace firebolt::rialto::client::ct
