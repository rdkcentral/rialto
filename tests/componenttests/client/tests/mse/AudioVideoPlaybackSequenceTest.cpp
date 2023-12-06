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

class AudioVideoPlaybackSequenceTest : public ClientComponentTest
{
public:
    int32_t m_segmentId{-1};

    AudioVideoPlaybackSequenceTest() : ClientComponentTest() { ClientComponentTest::startApplicationRunning(); }

    ~AudioVideoPlaybackSequenceTest() { ClientComponentTest::stopApplication(); }
};

/*
 * Component Test: Audio Video Playback Sequence
 * Test Objective:
 *  Test the playback of video and audio MSE content. The test transitions through the playback states
 *  buffering 1 frame of both audio and video content before preroll and 1 frame of both audio and video
 *  content after preroll. The session is then terminated. All the metadata and media data written to the
 *  shared buffer is checked for accuracy.
 *
 * Sequence Diagrams:
 *  Create, Destroy - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams
 *  Start/Resume Playback, Pause Playback, Stop, End of stream, Shared memory buffer refill
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
 *  Step 3: Attach all sources
 *   Attach the video source.
 *   Expect that attach source for video propagated to the server.
 *   Attach the audio source.
 *   Expect thatattach source for audio propagated to the server.
 *   Set all sources attached.
 *   Expect that all source attached is propagated to the server.
 *   Server notifys the client that the Playback state has changed to IDLE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 4: Pause
 *   Pause the content.
 *   Expect that pause is propagated to the server.
 *
 *  Step 5: Write 1 audio frame
 *   Server notifys the client that it needs 3 frames of audio data.
 *   Writes 1 frame of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 6: Write 1 video frame
 *   Server notifys the client that it needs 3 frames of video data.
 *   Writes 1 frame of video data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 7: Notify buffered
 *   Server notifys the client that the Network state has changed to BUFFERED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 8: Notify paused
 *   Server notifys the client that the Network state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 9: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 10: Write 1 audio frame
 *   Server notifys the client that it needs 20 frames of audio data.
 *   Writes 1 frames of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 11: Write 1 video frame
 *   Server notifys the client that it needs 20 frames of video data.
 *   Writes 1 frame of video data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 12: End of audio stream
 *   Server notifys the client that it needs 20 frames of audio data.
 *   Send EOS status with haveData.
 *
 *  Step 13: End of video stream
 *   Server notifys the client that it needs 20 frames of video data.
 *   Send EOS status with haveData.
 *
 *  Step 14: Notify end of stream
 *   Server notifys the client that the Network state has changed to END_OF_STREAM.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 15: Remove sources
 *   Remove the video source.
 *   Expect that remove source for video propagated to the server.
 *   Remove the audio source.
 *   Expect that remove source for audio propagated to the server.
 *
 *  Step 16: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the server.
 *   Server notifys the client that the Playback state has changed to STOPPED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 17: Destroy media session
 *   Destroy instance of MediaPipeline.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are propagated to the server.
 *  The state of the MediaPipeline is successfully negotiationed in the normal playback scenario.
 *  Data is successfully written to the shared memory for both audio and video.
 *
 * Code:
 */
TEST_F(AudioVideoPlaybackSequenceTest, playback)
{
    // Step 1: Create a new media session
    MediaPipelineTestMethods::shouldCreateMediaSession();
    MediaPipelineTestMethods::createMediaPipeline();

    // Step 2: Load content
    MediaPipelineTestMethods::shouldLoad();
    MediaPipelineTestMethods::load();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffering();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffering();

    // Step 3: Attach all sources
    MediaPipelineTestMethods::shouldAttachVideoSource();
    MediaPipelineTestMethods::attachSourceVideo();
    MediaPipelineTestMethods::shouldAttachAudioSource();
    MediaPipelineTestMethods::attachSourceAudio();
    MediaPipelineTestMethods::shouldAllSourcesAttached();
    MediaPipelineTestMethods::allSourcesAttached();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateIdle();
    MediaPipelineTestMethods::sendNotifyPlaybackStateIdle();

    // Step 4: Pause
    MediaPipelineTestMethods::shouldPause();
    MediaPipelineTestMethods::pause();

    // Step 5: Write 1 audio frame
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioBeforePreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll();
    m_segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
    MediaPipelineTestMethods::checkMseAudioSegmentWritten(m_segmentId);
    MediaPipelineTestMethods::shouldHaveDataOk(1);
    MediaPipelineTestMethods::haveDataOk();

    // Step 6: Write 1 video frame
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoBeforePreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataVideoBeforePreroll();
    m_segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(m_segmentId);
    MediaPipelineTestMethods::shouldHaveDataOk(1);
    MediaPipelineTestMethods::haveDataOk();

    // Step 7: Notify buffered
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffered();

    // Step 8: Notify paused
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 9: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlay();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlay();

    // Step 10: Write 1 audio frame
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioAfterPreroll();
    m_segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
    MediaPipelineTestMethods::checkMseAudioSegmentWritten(m_segmentId);
    MediaPipelineTestMethods::shouldHaveDataOk(1);
    MediaPipelineTestMethods::haveDataOk();

    // Step 11: Write 1 video frame
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataVideoAfterPreroll();
    m_segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(m_segmentId);
    MediaPipelineTestMethods::shouldHaveDataOk(1);
    MediaPipelineTestMethods::haveDataOk();

    // Step 12: End of audio stream
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioAfterPreroll();
    MediaPipelineTestMethods::shouldHaveDataEos(0);
    MediaPipelineTestMethods::haveDataEos();

    // Step 13: End of video stream
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataVideoAfterPreroll();
    MediaPipelineTestMethods::shouldHaveDataEos(0);
    MediaPipelineTestMethods::haveDataEos();

    // Step 14: Notify end of stream
    MediaPipelineTestMethods::shouldNotifyPlaybackStateEndOfStream();
    MediaPipelineTestMethods::sendNotifyPlaybackStateEndOfStream();

    // Step 15: Remove sources
    MediaPipelineTestMethods::shouldRemoveVideoSource();
    MediaPipelineTestMethods::removeSourceVideo();
    MediaPipelineTestMethods::shouldRemoveAudioSource();
    MediaPipelineTestMethods::removeSourceAudio();

    // Step 16: Stop
    MediaPipelineTestMethods::shouldStop();
    MediaPipelineTestMethods::stop();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateStopped();
    MediaPipelineTestMethods::sendNotifyPlaybackStateStopped();

    // Step 17: Destroy media session
    MediaPipelineTestMethods::shouldDestroyMediaSession();
    MediaPipelineTestMethods::destroyMediaPipeline();
}
