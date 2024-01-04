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
class SelectKeyIdTest : public ClientComponentTest
{
public:
    int32_t m_segmentId{-1};

    SelectKeyIdTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaKeysTestMethods::initalisePlayreadyMediaKeySession();
        MediaPipelineTestMethods::startAudioVideoMediaSessionWaitForPreroll();
    }

    ~SelectKeyIdTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        MediaKeysTestMethods::terminateMediaKeySession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Select Key ID for playready.
 * Test Objective:
 *  Test that a key id can be selected and added to the segments for playready session.
 *
 * Sequence Diagrams:
 *  Select Key ID
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Media+Key+Session+Management+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaKeys, MediaPipeline
 *
 * Test Initialize:
 *  Create memory region for the shared buffer.
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *  Initalise a playready media key session.
 *  Initalise a audio video media session to waiting for preroll.
 *
 * Test Steps:
 *  Step 1: Select key id for video
 *   SelectKeyId with the video key.
 *   Api call returns with success.
 *
 *  Step 2: Add encrypted video segment
 *   Server notifys the client that it needs 20 frames of video data.
 *   Writes 1 frame of video data to the shared buffer with key session id set.
 *   Check that the key id has been added to the metadata.
 *   Notify the server that the data has been written.
 *
 *  Step 3: Select key id for audio
 *   SelectKeyId with the audio key.
 *   Api call returns with success.
 *
 *  Step 4: Add encrypted audio segment
 *   Server notifys the client that it needs 20 frames of audio data.
 *   Writes 1 frame of audio data to the shared buffer with key session id set.
 *   Check that the key id has been added to the metadata.
 *   Notify the server that the data has been written.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Terminate the media key session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Keys can be selected using the selectKeyId API for playready segments.
 *
 * Code:
 */
TEST_F(SelectKeyIdTest, keyManagement)
{
    // Step 1: Select key id for video
    uint32_t keyIndex = 2;
    MediaKeysTestMethods::selectKeyId(keyIndex);

    // Step 2: Add encrypted video segment
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataVideoAfterPreroll();
    m_segmentId = MediaPipelineTestMethods::addSegmentEncryptedVideo();
    MediaPipelineTestMethods::checkVideoKeyId(m_segmentId, keyIndex);
    MediaPipelineTestMethods::shouldHaveDataOk(1);
    MediaPipelineTestMethods::haveDataOk();

    // Step 3: Select key id for audio
    keyIndex = 1;
    MediaKeysTestMethods::selectKeyId(keyIndex);

    // Step 4: Add encrypted audio segment
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioAfterPreroll();
    m_segmentId = MediaPipelineTestMethods::addSegmentEncryptedAudio();
    MediaPipelineTestMethods::checkAudioKeyId(m_segmentId, keyIndex);
    MediaPipelineTestMethods::shouldHaveDataOk(1);
    MediaPipelineTestMethods::haveDataOk();
}
} // namespace firebolt::rialto::client::ct
