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

class SetPlaybackRateTest : public ClientComponentTest
{
public:
    SetPlaybackRateTest() : ClientComponentTest() 
    { 
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~SetPlaybackRateTest() 
    { 
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Set Playback Rate
 * Test Objective:
 *  Test the that set playback rate can be called in paused and playing state, that set playback rate
 *  can accept positive and negative playback rates and that failure is handled correctly.
 *  
 * Sequence Diagrams:
 *  Set Playback Rate - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Step 1: Set playback rate to 2.0x in paused state
 *   SetPlaybackRate to 2.0x.
 *   Expect that SetPlaybackRate propagated to the server.
 *
 *  Step 2: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 3: Set playback rate to 2.0x in playing state
 *   SetPlaybackRate to 2.0x.
 *   Expect that SetPlaybackRate propagated to the server.
 *
 *  Step 4: Set playback rate to -2.0x in playing state
 *   SetPlaybackRate to -2.0x.
 *   Expect that SetPlaybackRate propagated to the server.
 *
 *  Step 5: Set playback rate failure
 *   SetPlaybackRate to 0.0x.
 *   Expect that SetPlaybackRate propagated to the server and return failure.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Set playback rate succeeds in the playing and pause state.
 *  Set playback rate succeeds with positive and negative playback rates.
 *  Failure is returned to the application.
 *
 * Code:
 */
TEST_F(SetPlaybackRateTest, setPlaybackRate)
{
    // Step 1: Set playback rate to 2.0x in paused state
    MediaPipelineTestMethods::shouldSetPlaybackRate2x();
    MediaPipelineTestMethods::setPlaybackRate2x();

    // Step 2: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 3: Set playback rate to 2.0x in playing state
    MediaPipelineTestMethods::shouldSetPlaybackRate2x();
    MediaPipelineTestMethods::setPlaybackRate2x();

    // Step 4: Set playback rate to -2.0x in playing state
    MediaPipelineTestMethods::shouldSetPlaybackRateNegative2x();
    MediaPipelineTestMethods::setPlaybackRateNegative2x();

    // Step 5: Set playback rate failure
    MediaPipelineTestMethods::shouldSetPlaybackRateFailure();
    MediaPipelineTestMethods::setPlaybackRateFailure();
}
