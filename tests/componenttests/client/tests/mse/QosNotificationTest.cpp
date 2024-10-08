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
class QosNotificationTest : public ClientComponentTest
{
public:
    QosNotificationTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();

        // Play
        MediaPipelineTestMethods::shouldPlay();
        MediaPipelineTestMethods::play();
        MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
        MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();
    }

    ~QosNotificationTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: QOS Notification
 * Test Objective:
 *  Test the QOS notification for all sources
 *  Also test the get-stats functionality (which returns the number of rendered and dropped frames)
 *
 * Sequence Diagrams:
 *  Quality of Service
 *  - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-QualityofService
 *  Stats
 *  - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-Stats
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
 *  Step 1: Notify qos audio
 *   Server notifies the client qos for audio.
 *   Expect that the qos notification is propagated to the client.
 *   Check qos info.
 *
 *  Step 2: Notify qos video
 *   Server notify the client qos for video.
 *   Expect that the qos notification is propagated to the client.
 *   Check qos info.
 *
 *  Step 3: Get Stats
 *   GetStats
 *   Expect that GetStats propagated to the server and returns the correct
 *   number of frames rendered and dropped.
 *
 * Test Tear-down:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Qos notifications for audio or video are handled by rialto client and the information is sent to the application.
 *
 * Code:
 */
TEST_F(QosNotificationTest, notifications)
{
    // Step 1: Notify qos audio
    MediaPipelineTestMethods::shouldNotifyQosAudio();
    MediaPipelineTestMethods::sendNotifyQosAudio();

    // Step 2: Notify qos video
    MediaPipelineTestMethods::shouldNotifyQosVideo();
    MediaPipelineTestMethods::sendNotifyQosVideo();

    // Step 3: Get stats
    {
        const uint64_t kRenderedFrames = 2345;
        const uint64_t kDroppedFrames = 6;
        MediaPipelineTestMethods::shouldGetStats(kRenderedFrames, kDroppedFrames);
        MediaPipelineTestMethods::getStats(kRenderedFrames, kDroppedFrames);
    }
}
} // namespace firebolt::rialto::client::ct
