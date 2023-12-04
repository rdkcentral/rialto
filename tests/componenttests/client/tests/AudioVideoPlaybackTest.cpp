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

class AudioVideoPlaybackTest : public ClientComponentTest
{
public:
    const uint32_t kNumberOfNeedDatasBeforePreroll = 2;
    const uint32_t kNumberOfFramesPerNeedDataBeforePreroll = 3;
    const uint32_t kNumberOfNeedDatasAfterPreroll = 1;
    const uint32_t kNumberOfFramesPerNeedDataAfterPreroll = 20;

    AudioVideoPlaybackTest()
        : ClientComponentTest()
    {
        // Set the metadata version shared.
        setenv("RIALTO_METADATA_VERSION", "2", 1);
        ClientComponentTest::startApplicationRunning();
    }

    ~AudioVideoPlaybackTest()
    {
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test:
 * Test Objective:
 *  Test the playback of video and audio MSE content. The test transitions through the playback states
 *  buffering 27 frames of both audio and video content before termination of the session. All the metadata and 
 *  media data written to the shared buffer is checked for accuracy.
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
 *  Step 5: Write 6 audio frames
 *   Server notifys the client that it needs 3 frames of audio data.
 *   Writes 3 frames of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *   x2
 *
 *  Step 6: Write 6 video frames
 *   Server notifys the client that it needs 3 frames of video data.
 *   Writes 3 frames of video data to the shared buffer.
 *   Notify the server that the data has been written.
 *   x2
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
 *  Step 10: Write 20 audio frames
 *   Server notifys the client that it needs 20 frames of audio data.
 *   Writes 5 frames of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 11: Write 20 video frames
 *   Server notifys the client that it needs 20 frames of video data.
 *   Writes 5 frames of video data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 12: End of audio stream
 *   Server notifys the client that it needs 20 frames of audio data.
 *   Writes 1 frame to the shared buffer.
 *   Send EOS status with samples.
 *
 *  Step 13: End of video stream
 *   Server notifys the client that it needs 20 frames of video data.
 *   Writes 1 frame to the shared buffer.
 *   Send EOS status with samples.
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
TEST_F(AudioVideoPlaybackTest, playback)
{
    int32_t segmentId = -1;

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

    // Step 5: Write 6 audio frames
    for (uint32_t i = 0; i < kNumberOfNeedDatasBeforePreroll; i++)
    {
        MediaPipelineTestMethods::shouldNotifyNeedDataAudioBeforePreroll();
        MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll();
        
        for (uint32_t j = 0; j < kNumberOfFramesPerNeedDataBeforePreroll; j++)
        {
            segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
            MediaPipelineTestMethods::checkMseAudioSegmentWritten(segmentId);
        }

        MediaPipelineTestMethods::shouldHaveDataBeforePreroll();
        MediaPipelineTestMethods::haveDataOk();
    }

    // Step 6: Write 6 video frames
    for (uint32_t i = 0; i < kNumberOfNeedDatasBeforePreroll; i++)
    {
        MediaPipelineTestMethods::shouldNotifyNeedDataVideoBeforePreroll();
        MediaPipelineTestMethods::sendNotifyNeedDataVideoBeforePreroll();
        
        for (uint32_t j = 0; j < kNumberOfFramesPerNeedDataBeforePreroll; j++)
        {
            segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
            MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
        }

        MediaPipelineTestMethods::shouldHaveDataBeforePreroll();
        MediaPipelineTestMethods::haveDataOk();
    }

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

    // Step 10: Write 20 audio frames
    for (uint32_t i = 0; i < kNumberOfNeedDatasAfterPreroll; i++)
    {
        MediaPipelineTestMethods::shouldNotifyNeedDataAudioAfterPreroll();
        MediaPipelineTestMethods::sendNotifyNeedDataAudioAfterPreroll();
        
        for (uint32_t j = 0; j < kNumberOfFramesPerNeedDataAfterPreroll; j++)
        {
            segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
            MediaPipelineTestMethods::checkMseAudioSegmentWritten(segmentId);
        }

        MediaPipelineTestMethods::shouldHaveDataAfterPreroll();
        MediaPipelineTestMethods::haveDataOk();
    }

    // Step 11: Write 20 video frames
    for (uint32_t i = 0; i < kNumberOfNeedDatasAfterPreroll; i++)
    {
        MediaPipelineTestMethods::shouldNotifyNeedDataVideoAfterPreroll();
        MediaPipelineTestMethods::sendNotifyNeedDataVideoAfterPreroll();
        
        for (uint32_t j = 0; j < kNumberOfFramesPerNeedDataAfterPreroll; j++)
        {
            segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
            MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
        }

        MediaPipelineTestMethods::shouldHaveDataAfterPreroll();
        MediaPipelineTestMethods::haveDataOk();
    }

    // Step 12: End of audio stream
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioAfterPreroll();
    segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
    MediaPipelineTestMethods::checkMseAudioSegmentWritten(segmentId);
    MediaPipelineTestMethods::shouldHaveDataEos(1);
    MediaPipelineTestMethods::haveDataEos();

    // Step 13: End of video stream
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataVideoAfterPreroll();
    segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
    MediaPipelineTestMethods::shouldHaveDataEos(1);
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
