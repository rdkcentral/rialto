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
class PipelinePropertyTest : public ClientComponentTest
{
public:
    PipelinePropertyTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~PipelinePropertyTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Get and set various pipeline properties
 * Test Objective:
 *   To test the reading and writing of different pipeline properties
 *   on the RialtoServer.
 *
 * Sequence Diagrams:
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
 *  Step 1: Set Immediate Output
 *   SetImmediateOutput
 *   Expect that SetImmediateOutput propagated to the server and sets the property
 *
 *  Step 2: Get Immediate Output
 *   GetImmediateOutput
 *   Expect that GetImmediateOutput propagated to the server and gets the property
 *
 *  Step 3: Set Low Latency
 *   SetLowLatency
 *   Expect that SetLowLatency propagated to the server and sets the property
 *
 *  Step 4: Set Sync
 *   SetSync
 *   Expect that SetSync propagated to the server and sets the property
 *
 *  Step 5: Get Sync
 *   GetSync
 *   Expect that GetSync propagated to the server and gets the property
 *
 *  Step 6: Set SyncOff
 *   SetSyncOff
 *   Expect that SetSyncOff propagated to the server and sets the property
 *
 *  Step 7: Set StreamSyncMode
 *   SetStreamSyncMode
 *   Expect that SetStreamSyncMode propagated to the server and sets the property
 *
 *  Step 8: Get StreamSyncMode
 *   GetStreamSyncMode
 *   Expect that GetStreamSyncMode propagated to the server and gets the property
 *
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *   All properties are correctly obtained/ written as expected
 *
 * Code:
 */
TEST_F(PipelinePropertyTest, setAndGetPipelineProperties)
{
    // Step 1: Set Immediate Output
    bool immediateOutput{true};
    MediaPipelineTestMethods::shouldSetImmediateOutput(immediateOutput);
    MediaPipelineTestMethods::setImmediateOutput(immediateOutput);

    // Step 2: Get Immediate Output
    MediaPipelineTestMethods::shouldGetImmediateOutput(immediateOutput);
    MediaPipelineTestMethods::getImmediateOutput(immediateOutput);

    // Step 3: Set Low Latency
    bool lowLatency{true};
    MediaPipelineTestMethods::shouldSetLowLatency(lowLatency);
    MediaPipelineTestMethods::setLowLatency(lowLatency);

    // Step 4: Set Sync
    bool sync{true};
    MediaPipelineTestMethods::shouldSetSync(sync);
    MediaPipelineTestMethods::setSync(sync);

    // Step 5: Get Sync
    MediaPipelineTestMethods::shouldGetSync(sync);
    MediaPipelineTestMethods::getSync(sync);

    // Step 6: Set SyncOff
    bool syncOff{true};
    MediaPipelineTestMethods::shouldSetSyncOff(syncOff);
    MediaPipelineTestMethods::setSyncOff(syncOff);

    // Step 7: Set StreamSyncMode
    int32_t streamSyncMode{1};
    MediaPipelineTestMethods::shouldSetStreamSyncMode(streamSyncMode);
    MediaPipelineTestMethods::setStreamSyncMode(streamSyncMode);

    // Step 8: Get StreamSyncMode
    MediaPipelineTestMethods::shouldGetStreamSyncMode(streamSyncMode);
    MediaPipelineTestMethods::getStreamSyncMode(streamSyncMode);
}
} // namespace firebolt::rialto::client::ct
