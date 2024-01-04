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
class AudioVideoPlaybackEncryptedSegmentsTest : public ClientComponentTest
{
public:
    int32_t m_segmentId{-1};
    uint32_t m_numberOfNeedDatas{0};
    uint32_t m_numberOfFramesPerNeedData{0};

    AudioVideoPlaybackEncryptedSegmentsTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaKeysTestMethods::initaliseWidevineMediaKeySession();
        MediaPipelineTestMethods::startAudioVideoMediaSessionWaitForPreroll();
    }

    ~AudioVideoPlaybackEncryptedSegmentsTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        MediaKeysTestMethods::terminateMediaKeySession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Audio Video Playback Encrypted Segments
 * Test Objective:
 *  Test the writing of encrypted audio and video segments during playback.
 *
 * Sequence Diagrams:
 *  Shared memory buffer refill - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Initalise a media key session.
 *  Initalise a audio video media session to waiting for preroll.
 *
 * Test Steps:
 *  Step 1: Write 6 encrypted audio frames with key 1
 *   Server notifys the client that it needs 3 frames of audio data.
 *   Writes 3 encrypted frames of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *   x2
 *
 *  Step 2: Write 6 encrypted video frames with key 1
 *   Server notifys the client that it needs 3 frames of video data.
 *   Writes 3 encrypted frames of video data to the shared buffer.
 *   Notify the server that the data has been written.
 *   x2
 *
 *  Step 3: Write 2 encrypted audio frames with key 2
 *   Server notifys the client that it needs 3 frames of audio data.
 *   Writes 2 encrypted frames of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 4: Write 2 encrypted video frames with key 2
 *   Server notifys the client that it needs 3 frames of video data.
 *   Writes 2 encrypted frames of video data to the shared buffer.
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
 *  Step 7: Write 20 encrypted audio frames with key 3
 *   Server notifys the client that it needs 20 frames of audio data.
 *   Writes 20 encrypted frames of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 8: Write 20 encrypted video frames with key 3
 *   Server notifys the client that it needs 20 frames of video data.
 *   Writes 20 encrypted frames of video data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 9: Write 5 encrypted audio frames with key 1
 *   Server notifys the client that it needs 20 frames of audio data.
 *   Writes 5 encrypted frames of audio data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 *  Step 10: Write 5 encrypted video frames with key 1
 *   Server notifys the client that it needs 20 frames of video data.
 *   Writes 5 encrypted frames of video data to the shared buffer.
 *   Notify the server that the data has been written.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Terminate the media key session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Encrypted data is successfully written to the shared memory for both audio and video.
 *
 * Code:
 */
TEST_F(AudioVideoPlaybackEncryptedSegmentsTest, encryptedPlayback)
{
    // Step 1: Write 6 encrypted audio frames with key 1
    m_numberOfNeedDatas = 2;
    for (uint32_t i = 0; i < m_numberOfNeedDatas; i++)
    {
        MediaPipelineTestMethods::shouldNotifyNeedDataAudioBeforePreroll();
        MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll();

        m_numberOfFramesPerNeedData = 3;
        for (uint32_t j = 0; j < m_numberOfFramesPerNeedData; j++)
        {
            uint32_t keyIndex = 0;
            m_segmentId = MediaPipelineTestMethods::addSegmentEncryptedAudio(keyIndex);
            MediaPipelineTestMethods::checkEncryptedAudioSegmentWritten(m_segmentId, keyIndex);
        }

        MediaPipelineTestMethods::shouldHaveDataBeforePreroll();
        MediaPipelineTestMethods::haveDataOk();
    }

    // Step 2: Write 6 encrypted video frames with key 1
    m_numberOfNeedDatas = 2;
    for (uint32_t i = 0; i < m_numberOfNeedDatas; i++)
    {
        MediaPipelineTestMethods::shouldNotifyNeedDataVideoBeforePreroll();
        MediaPipelineTestMethods::sendNotifyNeedDataVideoBeforePreroll();

        m_numberOfFramesPerNeedData = 3;
        for (uint32_t j = 0; j < m_numberOfFramesPerNeedData; j++)
        {
            uint32_t keyIndex = 0;
            m_segmentId = MediaPipelineTestMethods::addSegmentEncryptedVideo(keyIndex);
            MediaPipelineTestMethods::checkEncryptedVideoSegmentWritten(m_segmentId, keyIndex);
        }

        MediaPipelineTestMethods::shouldHaveDataBeforePreroll();
        MediaPipelineTestMethods::haveDataOk();
    }

    // Step 3: Write 2 encrypted audio frames with key 2
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioBeforePreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll();

    m_numberOfFramesPerNeedData = 2;
    for (uint32_t j = 0; j < m_numberOfFramesPerNeedData; j++)
    {
        uint32_t keyIndex = 1;
        m_segmentId = MediaPipelineTestMethods::addSegmentEncryptedAudio(keyIndex);
        MediaPipelineTestMethods::checkEncryptedAudioSegmentWritten(m_segmentId, keyIndex);
    }

    MediaPipelineTestMethods::shouldHaveDataOk(2);
    MediaPipelineTestMethods::haveDataOk();

    // Step 4: Write 2 encrypted video frames with key 2
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoBeforePreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataVideoBeforePreroll();

    m_numberOfFramesPerNeedData = 2;
    for (uint32_t j = 0; j < m_numberOfFramesPerNeedData; j++)
    {
        uint32_t keyIndex = 1;
        m_segmentId = MediaPipelineTestMethods::addSegmentEncryptedVideo(keyIndex);
        MediaPipelineTestMethods::checkEncryptedVideoSegmentWritten(m_segmentId, keyIndex);
    }

    MediaPipelineTestMethods::shouldHaveDataOk(2);
    MediaPipelineTestMethods::haveDataOk();

    // Step 5: Preroll
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();

    // Step 6: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 7: Write 20 encrypted audio frames with key 3
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioAfterPreroll();

    m_numberOfFramesPerNeedData = 20;
    for (uint32_t j = 0; j < m_numberOfFramesPerNeedData; j++)
    {
        uint32_t keyIndex = 2;
        m_segmentId = MediaPipelineTestMethods::addSegmentEncryptedAudio(keyIndex);
        MediaPipelineTestMethods::checkEncryptedAudioSegmentWritten(m_segmentId, keyIndex);
    }

    MediaPipelineTestMethods::shouldHaveDataAfterPreroll();
    MediaPipelineTestMethods::haveDataOk();

    // Step 8: Write 20 encrypted video frames with key 3
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataVideoAfterPreroll();

    m_numberOfFramesPerNeedData = 20;
    for (uint32_t j = 0; j < m_numberOfFramesPerNeedData; j++)
    {
        uint32_t keyIndex = 2;
        m_segmentId = MediaPipelineTestMethods::addSegmentEncryptedVideo(keyIndex);
        MediaPipelineTestMethods::checkEncryptedVideoSegmentWritten(m_segmentId, keyIndex);
    }

    MediaPipelineTestMethods::shouldHaveDataAfterPreroll();
    MediaPipelineTestMethods::haveDataOk();

    // Step 9: Write 5 encrypted audio frames with key 1
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioAfterPreroll();

    m_numberOfFramesPerNeedData = 5;
    for (uint32_t j = 0; j < m_numberOfFramesPerNeedData; j++)
    {
        uint32_t keyIndex = 0;
        m_segmentId = MediaPipelineTestMethods::addSegmentEncryptedAudio(keyIndex);
        MediaPipelineTestMethods::checkEncryptedAudioSegmentWritten(m_segmentId, keyIndex);
    }

    MediaPipelineTestMethods::shouldHaveDataOk(5);
    MediaPipelineTestMethods::haveDataOk();

    // Step 10: Write 5 encrypted video frames with key 1
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataVideoAfterPreroll();

    m_numberOfFramesPerNeedData = 5;
    for (uint32_t j = 0; j < m_numberOfFramesPerNeedData; j++)
    {
        uint32_t keyIndex = 0;
        m_segmentId = MediaPipelineTestMethods::addSegmentEncryptedVideo(keyIndex);
        MediaPipelineTestMethods::checkEncryptedVideoSegmentWritten(m_segmentId, keyIndex);
    }

    MediaPipelineTestMethods::shouldHaveDataOk(5);
    MediaPipelineTestMethods::haveDataOk();
}
} // namespace firebolt::rialto::client::ct
