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
class WriteSegmentEdgeCasesTest : public ClientComponentTest
{
public:
    int32_t m_segmentId{-1};
    size_t m_framesToWrite{1};

    WriteSegmentEdgeCasesTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionWaitForPreroll();
    }

    ~WriteSegmentEdgeCasesTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Failures during writing for data.
 * Test Objective:
 *  Test add segments and have data failures.
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
 *  Initalise a audio video media session to waiting for preroll.
 *
 * Test Steps:
 *  Step 1: Have data with no need data (succeeds)
 *   haveData with status OK.
 *   Api call returns with success.
 *
 *  Step 2: Notify need data
 *   Server notifys the client that it needs 3 frames of video data.
 *
 *  Step 3: Send have data response with no available samples status
 *   haveData with status NO_AVAILABLE_SAMPLES.
 *   Expect that haveData is propagated to the server.
 *   Api call returns with success.
 *
 *  Step 4: Notify need data
 *   Server notifys the client that it needs 3 frames of video data.
 *
 *  Step 5: Send have data response with error status
 *   haveData with status ERROR.
 *   Expect that haveData is propagated to the server.
 *   Api call returns with success.
 *
 *  Step 6: Notify need data
 *   Server notifys the client that it needs 3 frames of video data.
 *
 *  Step 7: Add segments no space
 *   addSegment with large frame.
 *   Api call returns with NO_SPACE.
 *
 *  Step 8: Add 1 video segments
 *   addSegment.
 *   Api call returns with success.
 *
 *  Step 9: Send have data ok response but failure in server
 *   haveData with status OK.
 *   Expect that haveData is propagated to the server.
 *   Api call returns with error.
 *   Server notifys the client that the Playback state has changed to ERROR.
 *   Expect that the state change notification is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Rialto client can handle the addition of the maximum number of frames and partial number of frames
 *  before and after preroll. Data is successfully written to the shared memory for both audio and video.
 *
 * Code:
 */
TEST_F(WriteSegmentEdgeCasesTest, edgeCaseScenarios)
{
    // Step 1: Have data with no need data (succeeds)
    MediaPipelineTestMethods::haveDataOk();

    // Step 2: Notify need data
    MediaPipelineTestMethods::shouldNotifyNeedDataVideo(m_framesToWrite);
    MediaPipelineTestMethods::sendNotifyNeedDataVideo(m_framesToWrite);

    // Step 3: Send have data response with no available samples status
    MediaPipelineTestMethods::shouldHaveDataNoAvailableSamples();
    MediaPipelineTestMethods::haveDataNoAvailableSamples();

    // Step 4: Notify need data
    MediaPipelineTestMethods::shouldNotifyNeedDataVideo(m_framesToWrite);
    MediaPipelineTestMethods::sendNotifyNeedDataVideo(m_framesToWrite);

    // Step 5: Send have data response with error status
    MediaPipelineTestMethods::shouldHaveDataError();
    MediaPipelineTestMethods::haveDataError();

    // Step 6: Notify need data
    MediaPipelineTestMethods::shouldNotifyNeedDataVideo(m_framesToWrite);
    MediaPipelineTestMethods::sendNotifyNeedDataVideo(m_framesToWrite);

    // Step 7: Add segments no space
    MediaPipelineTestMethods::addSegmentMseVideoNoSpace();

    // Step 8: Add 1 video segments
    m_segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(m_segmentId);

    // Step 9: Send have data ok response but failure in server
    MediaPipelineTestMethods::shouldHaveDataFailure(m_framesToWrite);
    MediaPipelineTestMethods::haveDataFailure();
}
} // namespace firebolt::rialto::client::ct
