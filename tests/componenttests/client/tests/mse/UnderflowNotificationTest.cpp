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
class UnderflowNotificationTest : public ClientComponentTest
{
public:
    int32_t m_segmentId{-1};
    size_t m_framesToWrite{1};

    UnderflowNotificationTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();

        // Play
        MediaPipelineTestMethods::shouldPlay();
        MediaPipelineTestMethods::play();
        MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
        MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();
    }

    ~UnderflowNotificationTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Underflow notification
 * Test Objective:
 *  Test the underflow notification for all sources.
 *
 * Sequence Diagrams:
 *  Underflow - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Initalise a audio video media session playing.
 *
 * Test Steps:
 *  Step 1: Notify need data for audio
 *   Server notifys the client that it needs 20 frames of audio data.
 *
 *  Step 2: Notify audio underflow
 *   Server notifys the client underflow with source id audio.
 *   Expect that the underflow notification is propagated to the client.
 *
 *  Step 3: Recover audio underflow
 *   addSegment with one audio frame.
 *   Api call returns with success.
 *   haveData with status OK.
 *   Expect that haveData propagated to the server.
 *   Api call returns with success.
 *
 *  Step 4: Notify need data for video
 *   Server notifys the client that it needs 20 frames of video data.
 *
 *  Step 5: Notify video underflow
 *   Server notifys the client underflow with source id video.
 *   Expect that the underflow notification is propagated to the client.
 *
 *  Step 6: Recover video underflow
 *   addSegment with one video frame.
 *   Api call returns with success.
 *   haveData with status OK.
 *   Expect that haveData propagated to the server.
 *   Api call returns with success.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Underflows notified for audio or video are propagated to the application and can be recovered.
 *
 * Code:
 */
TEST_F(UnderflowNotificationTest, notifications)
{
    // Step 1: Notify need data for audio
    MediaPipelineTestMethods::shouldNotifyNeedDataAudio(m_framesToWrite);
    MediaPipelineTestMethods::sendNotifyNeedDataAudio(m_framesToWrite);

    // Step 2: Notify audio underflow
    MediaPipelineTestMethods::shouldNotifyBufferUnderflowAudio();
    MediaPipelineTestMethods::sendNotifyBufferUnderflowAudio();

    // Step 3: Recover audio underflow
    m_segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
    MediaPipelineTestMethods::checkMseAudioSegmentWritten(m_segmentId);
    MediaPipelineTestMethods::shouldHaveDataOk(m_framesToWrite);
    MediaPipelineTestMethods::haveDataOk();

    // Step 4: Notify need data for video
    MediaPipelineTestMethods::shouldNotifyNeedDataVideo(m_framesToWrite);
    MediaPipelineTestMethods::sendNotifyNeedDataVideo(m_framesToWrite);

    // Step 5: Notify video underflow
    MediaPipelineTestMethods::shouldNotifyBufferUnderflowVideo();
    MediaPipelineTestMethods::sendNotifyBufferUnderflowVideo();

    // Step 6: Recover video underflow
    m_segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(m_segmentId);
    MediaPipelineTestMethods::shouldHaveDataOk(m_framesToWrite);
    MediaPipelineTestMethods::haveDataOk();
}
} // namespace firebolt::rialto::client::ct
