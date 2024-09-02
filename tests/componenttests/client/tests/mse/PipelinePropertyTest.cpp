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
 *  Immediate-output property
 *  - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-ImmediateOutput
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
    bool kTestValueOfImmediateOutput{true};
    MediaPipelineTestMethods::shouldSetImmediateOutput(kTestValueOfImmediateOutput);
    MediaPipelineTestMethods::setImmediateOutput(kTestValueOfImmediateOutput);

    // Step 2: Get Immediate Output
    MediaPipelineTestMethods::shouldGetImmediateOutput(kTestValueOfImmediateOutput);
    MediaPipelineTestMethods::getImmediateOutput(kTestValueOfImmediateOutput);
}
} // namespace firebolt::rialto::client::ct
