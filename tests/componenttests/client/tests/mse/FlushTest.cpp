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
class FlushTest : public ClientComponentTest
{
public:
    FlushTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~FlushTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Flush success
 * Test Objective:
 *  Test that flush is successfully handled.
 *
 * Sequence Diagrams:
 *  Flush - https://wiki.rdkcentral.com/display/ASP/Rialto+Flush+and+Seek+Design
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
 *  Step 1: Flush
 *   Server notifies the client that flush procedure has been queued in server
 *   Expect that the procedure status is propagated to the client.
 *
 *  Step 2: Flush complete
 *   Server notifies the client that the Flush procedure has been completed.
 *   Expect that the state change notification is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Flush is handled and forwarded to the server for PLAYING, PAUSED and END_OF_STREAM state.
 *
 * Code:
 */
TEST_F(FlushTest, flushSuccess)
{
    // Step 1: Flush
    MediaPipelineTestMethods::shouldFlush();
    MediaPipelineTestMethods::flush();

    // Step 2: Flush complete
    MediaPipelineTestMethods::shouldNotifySourceFlushed();
    MediaPipelineTestMethods::sendNotifySourceFlushed();
}

/*
 * Component Test: Discard Data When Flushing
 * Test Objective:
 *  Test that when flush is in progress data requests are discarded.
 *
 * Sequence Diagrams:
 *  Flush - https://wiki.rdkcentral.com/display/ASP/Rialto+Flush+and+Seek+Design
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
 *  Step 2: Flush in paused state
 *   Server notifies the client that flush procedure has been queued in server
 *   Expect that the procedure status is propagated to the client.
 *
 *  Step 3: Add segment failure
 *   Add a segment.
 *   Expect that addSegment return failure.
 *
 *  Step 4: Have data ignored
 *   Notify the server of have data.
 *   Expect that have data is not propagted to the server while flushing.
 *
 *  Step 5: Flush complete
 *   Server notifies the client that the Flush procedure has been completed.
 *   Expect that the state change notification is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Segments cannot be added and have data requests are silently ignored when a flush is in progress.
 *
 * Code:
 */
TEST_F(FlushTest, discardDataWhenFlushing)
{
    // Step 1: Need data
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioBeforePreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll();

    // Step 2: Flush in paused state
    MediaPipelineTestMethods::shouldFlush();
    MediaPipelineTestMethods::flush();

    // Step 3: Add segment failure
    MediaPipelineTestMethods::addSegmentFailure();

    // Step 4: Have data ignored
    MediaPipelineTestMethods::haveDataOk();

    // Step 5: Flush complete
    MediaPipelineTestMethods::shouldNotifySourceFlushed();
    MediaPipelineTestMethods::sendNotifySourceFlushed();
}

/*
 * Component Test: Flush failures
 * Test Objective:
 *  Check that failures returned directly from the Flush api are handled correctly.
 *
 * Sequence Diagrams:
 *  Flush - https://wiki.rdkcentral.com/display/ASP/Rialto+Flush+and+Seek+Design
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
 *  Step 1: Flush failure
 *   Server notifies the client that flush failed
 *   Expect that the procedure status is propagated to the client.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  FAILURE is notifed if flush fails.
 *
 * Code:
 */
TEST_F(FlushTest, failures)
{
    // Step 1: Flush failure
    MediaPipelineTestMethods::shouldFailToFlush();
    MediaPipelineTestMethods::flushFailure();
}
} // namespace firebolt::rialto::client::ct
