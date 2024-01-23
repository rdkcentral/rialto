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
class SetPositionTest : public ClientComponentTest
{
public:
    int64_t m_position = 0;
    SetPositionTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~SetPositionTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Set position success states
 * Test Objective:
 *  Test that seek is successfully handled is PAUSED, PLAYING and END_OF_STREAM state.
 *
 * Sequence Diagrams:
 *  Seek - https://wiki.rdkcentral.com/display/ASP/Rialto+Seek+Design
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
 *  Step 1: SetPosition in paused state
 *   SetPosition to position 10s.
 *   Server notifies the client that the Playback state has changed to SEEKING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 2: Seek complete
 *   Server notifies the client that the Playback state has changed to FLUSHED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 3: Buffer to paused state
 *   Write audio frames.
 *   Write video frames.
 *   Server notifies the client that the Network state has changed to BUFFERED.
 *   Expect that the state change notification is propagated to the client.
 *   Server notifies the client that the Playback state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 4: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifies the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 5: SetPosition in play state
 *   SetPosition to position 0s.
 *   Server notifies the client that the Playback state has changed to SEEKING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 6: Seek complete
 *   Server notifies the client that the Playback state has changed to FLUSHED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 7: Buffer to playing state
 *   Write audio frames.
 *   Write video frames.
 *   Server notifies the client that the Network state has changed to BUFFERED.
 *   Expect that the state change notification is propagated to the client.
 *   Server notifies the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 8: End of stream
 *   Write audio data end of stream.
 *   Write video data end of stream.
 *   Server notifies the client that the Network state has changed to END_OF_STREAM.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 9: SetPosition in end of stream state
 *   SetPosition to position 0s.
 *   Server notifies the client that the Playback state has changed to SEEKING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 10: Seek complete
 *   Server notifies the client that the Playback state has changed to FLUSHED.
 *   Expect that the state change notification is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Set position is handled and forwarded to the server for PLAYING, PAUSED and END_OF_STREAM state.
 *
 * Code:
 */
TEST_F(SetPositionTest, successStates)
{
    // Step 1: SetPosition in paused state
    m_position = 10;
    MediaPipelineTestMethods::shouldSetPosition(m_position);
    MediaPipelineTestMethods::setPosition(m_position);
    MediaPipelineTestMethods::shouldNotifyPlaybackStateSeeking();
    MediaPipelineTestMethods::sendNotifyPlaybackStateSeeking();

    // Step 2: Seek complete
    MediaPipelineTestMethods::shouldNotifyPlaybackStateFlushed();
    MediaPipelineTestMethods::sendNotifyPlaybackStateFlushed();

    // Step 3: Buffer to paused state
    MediaPipelineTestMethods::writeAudioFrames();
    MediaPipelineTestMethods::writeVideoFrames();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 4: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 5: SetPosition in play state
    m_position = 0;
    MediaPipelineTestMethods::shouldSetPosition(m_position);
    MediaPipelineTestMethods::setPosition(m_position);
    MediaPipelineTestMethods::shouldNotifyPlaybackStateSeeking();
    MediaPipelineTestMethods::sendNotifyPlaybackStateSeeking();

    // Step 6: Seek complete
    MediaPipelineTestMethods::shouldNotifyPlaybackStateFlushed();
    MediaPipelineTestMethods::sendNotifyPlaybackStateFlushed();

    // Step 7: Buffer to playing state
    MediaPipelineTestMethods::writeAudioFrames();
    MediaPipelineTestMethods::writeVideoFrames();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 8: End of stream
    MediaPipelineTestMethods::writeAudioEos();
    MediaPipelineTestMethods::writeVideoEos();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateEndOfStream();
    MediaPipelineTestMethods::sendNotifyPlaybackStateEndOfStream();

    // Step 9: SetPosition in end of stream state
    m_position = 10;
    MediaPipelineTestMethods::shouldSetPosition(m_position);
    MediaPipelineTestMethods::setPosition(m_position);
    MediaPipelineTestMethods::shouldNotifyPlaybackStateSeeking();
    MediaPipelineTestMethods::sendNotifyPlaybackStateSeeking();

    // Step 10: Seek complete
    MediaPipelineTestMethods::shouldNotifyPlaybackStateFlushed();
    MediaPipelineTestMethods::sendNotifyPlaybackStateFlushed();
}

/*
 * Component Test: Set position flush
 * Test Objective:
 *  Test that when seek is in progress data requests are flushed.
 *
 * Sequence Diagrams:
 *  Seek - https://wiki.rdkcentral.com/display/ASP/Rialto+Seek+Design
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
 *  Step 1: Need data
 *   Server notifies the client that it needs 20 frames of audio data.
 *
 *  Step 2: SetPosition in paused state
 *   SetPosition to position 10s.
 *   Server notifies the client that the Playback state has changed to SEEKING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 3: Add segment failure
 *   Add a segment.
 *   Expect that addSegment return failure.
 *
 *  Step 4: Have data ignored
 *   Notify the server of have data.
 *   Expect that have data is not propagted to the server while seeking.
 *
 *  Step 5: Seek complete
 *   Server notifies the client that the Playback state has changed to FLUSHED.
 *   Expect that the state change notification is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Segments cannot be added and have data requests are silently ignored when a seek is in progress.
 *
 * Code:
 */
TEST_F(SetPositionTest, flushed)
{
    // Step 1: Need data
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioBeforePreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll();

    // Step 2: SetPosition in paused state
    m_position = 10;
    MediaPipelineTestMethods::shouldSetPosition(m_position);
    MediaPipelineTestMethods::setPosition(m_position);
    MediaPipelineTestMethods::shouldNotifyPlaybackStateSeeking();
    MediaPipelineTestMethods::sendNotifyPlaybackStateSeeking();

    // Step 3: Add segment failure
    MediaPipelineTestMethods::addSegmentFailure();

    // Step 4: Have data ignored
    MediaPipelineTestMethods::haveDataOk();

    // Step 5: Seek complete
    MediaPipelineTestMethods::shouldNotifyPlaybackStateFlushed();
    MediaPipelineTestMethods::sendNotifyPlaybackStateFlushed();
}

/*
 * Component Test: Set position failures
 * Test Objective:
 *  Check that failures returned directly from the SetPosition api and failures returned asyncronously
 *  during server state changes are handled correctly.
 *
 * Sequence Diagrams:
 *  Seek - https://wiki.rdkcentral.com/display/ASP/Rialto+Seek+Design
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
 *  Step 1: SetPosition in paused state
 *   SetPosition to position 10s.
 *   Server notifies the client that the Playback state has changed to SEEKING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 2: SetPosition state failure
 *   Server notifies the client that the Playback state has changed to FAILURE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 3: SetPosition in failure state
 *   SetPosition to position 10s.
 *   Expect that setPosition return failure.
 *
 *  Step 4: Pause
 *   Pause the content.
 *   Expect that pause propagated to the server.
 *   Server notifies the client that the Playback state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 5: SetPosition in paused state
 *   SetPosition to position 0s.
 *   Server notifies the client that the Playback state has changed to SEEKING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 6: Seek complete
 *   Server notifies the client that the Playback state has changed to FLUSHED.
 *   Expect that the state change notification is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  FAILURE is notifed if seek fails ayncronously, subsequent set positions fail until the
 *  state has been moved to a valid state.
 *
 * Code:
 */
TEST_F(SetPositionTest, failures)
{
    // Step 1: SetPosition in paused state
    m_position = 10;
    MediaPipelineTestMethods::shouldSetPosition(m_position);
    MediaPipelineTestMethods::setPosition(m_position);
    MediaPipelineTestMethods::shouldNotifyPlaybackStateSeeking();
    MediaPipelineTestMethods::sendNotifyPlaybackStateSeeking();

    // Step 2: SetPosition state failure
    MediaPipelineTestMethods::shouldNotifyPlaybackStateFailure();
    MediaPipelineTestMethods::sendNotifyPlaybackStateFailure();

    // Step 3: SetPosition in failure state
    MediaPipelineTestMethods::setPositionFailure();

    // Step 4: Pause
    MediaPipelineTestMethods::shouldPause();
    MediaPipelineTestMethods::pause();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 5: SetPosition in paused state
    MediaPipelineTestMethods::shouldSetPosition(m_position);
    MediaPipelineTestMethods::setPosition(m_position);
    MediaPipelineTestMethods::shouldNotifyPlaybackStateSeeking();
    MediaPipelineTestMethods::sendNotifyPlaybackStateSeeking();

    // Step 6: Seek complete
    MediaPipelineTestMethods::shouldNotifyPlaybackStateFlushed();
    MediaPipelineTestMethods::sendNotifyPlaybackStateFlushed();
}
} // namespace firebolt::rialto::client::ct
