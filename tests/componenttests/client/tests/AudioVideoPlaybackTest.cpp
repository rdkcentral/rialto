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

class AudioVideoPlaybackTest : public ClientComponentTest
{
public:
    AudioVideoPlaybackTest()
        : ClientComponentTest()
    {
        startApplicationRunning();
    }

    ~AudioVideoPlaybackTest()
    {
        stopApplication();
    }
};

/*
 * Component Test:
 * Test Objective:
 * 
 * 
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: 
 *
 * Test Initialize:
 *  Create memory region for the shared buffer.
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *
 * Test Steps:
 *  Step 1: Create a new media session
 *   Create an instance of MediaPipeline.
 *   Expect that a session is created on the server.
 *   Check that the object returned is valid.
 * 
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 * 
 *  
 *
 * Code:
 */
TEST_F(AudioVideoPlaybackTest, playback)
{
    // Step 1: Create a new media session
    MediaPipelineTestMethods::shouldCreateMediaSession();
    MediaPipelineTestMethods::createMediaPipeline();
}
