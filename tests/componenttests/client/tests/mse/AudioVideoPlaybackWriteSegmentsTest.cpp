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

namespace
{
constexpr uint32_t kNumberOfNeedDatasBeforePreroll{2};
constexpr uint32_t kNumberOfFramesPerNeedDataBeforePreroll{3};
constexpr uint32_t kNumberOfNeedDatasAfterPreroll{1};
constexpr uint32_t kNumberOfFramesPerNeedDataAfterPreroll{20};
}; // namespace

class AudioVideoPlaybackWriteSegmentsTest : public ClientComponentTest
{
public:
    int32_t m_segmentId{-1};

    AudioVideoPlaybackWriteSegmentsTest() : ClientComponentTest() 
    { 
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionWaitForPreroll();
    }

    ~AudioVideoPlaybackWriteSegmentsTest() 
    { 
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test:
 * Test Objective:
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
 *  Initalise a audio video media session to waiting for preroll.
 *
 * Test Steps:
 *  Step 1: Write 6 audio frames
 *   Server notifys the client that it needs 3 frames of audio data.
 *   Writes 3 frames of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *   x2
 *
 *  Step 2: Write 6 video frames
 *   Server notifys the client that it needs 3 frames of video data.
 *   Writes 3 frames of video data to the shared buffer.
 *   Notify the server that the data has been written.
 *   x2
 * 
 *  Step 3: Write 2 audio frames
 *   Server notifys the client that it needs 3 frames of audio data.
 *   Writes 2 frames of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 4: Write 2 video frames
 *   Server notifys the client that it needs 3 frames of video data.
 *   Writes 2 frames of video data to the shared buffer.
 *   Notify the server that the data has been written.
 *  
 *  Step 5: Preroll
 *   Server notifys the client that the Network state has changed to BUFFERED.
 *   Expect that the state change notification is propagated to the client.
 *   Server notifys the client that the Network state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 6: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 7: Write 20 audio frames
 *   Server notifys the client that it needs 20 frames of audio data.
 *   Writes 20 frames of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 8: Write 20 video frames
 *   Server notifys the client that it needs 20 frames of video data.
 *   Writes 20 frames of video data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 9: Write 5 audio frames
 *   Server notifys the client that it needs 20 frames of audio data.
 *   Writes 5 frames of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 10: Write 5 video frames
 *   Server notifys the client that it needs 20 frames of video data.
 *   Writes 5 frames of video data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *
 * Code:
 */
TEST_F(AudioVideoPlaybackWriteSegmentsTest, playback)
{
    // Step 1: Write 6 audio frames
    for (uint32_t i = 0; i < kNumberOfNeedDatasBeforePreroll; i++)
    {
        MediaPipelineTestMethods::shouldNotifyNeedDataAudioBeforePreroll();
        MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll();

        for (uint32_t j = 0; j < kNumberOfFramesPerNeedDataBeforePreroll; j++)
        {
            m_segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
            MediaPipelineTestMethods::checkMseAudioSegmentWritten(m_segmentId);
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
            m_segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
            MediaPipelineTestMethods::checkMseVideoSegmentWritten(m_segmentId);
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
