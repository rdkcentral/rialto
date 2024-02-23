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
class WriteBufferFailuresTest : public ClientComponentTest
{
public:
    WriteBufferFailuresTest() : ClientComponentTest() { ClientComponentTest::startApplicationRunning(); }

    ~WriteBufferFailuresTest() { ClientComponentTest::stopApplication(); }
};

/*
 * Component Test: Write Buffer Failure
 * Test Objective:
 *  Test the failure to write buffer
 *
 * Sequence Diagrams:
 *  Write Buffer Failures -> https://wiki.rdkcentral.com/display/ASP/Rialto+Web+Audio
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
 *
 * Test Steps:
 *  Step 1: Create a new web audio player session
 *   Create an instance of WebAudioPlayer.
 *   Expect that web audio api is called on the server
 *   Check that the object returned is valid.
 *   Check that web audio player has been added.
 *
 * Step 2: Get the available buffer
 *   getBufferAvailable.
 *   Expect that getBufferAvailable is propagated to the server.
 *   Api call return the available frames and web audio shm info.
 *   Check available frames and web audio shm info.
 *
 *  Step 3: Get the write buffer failure
 *   writeBuffer.
 *   Expect that writeBuffer is propagated to the server and return failure.
 *   Api call return failure
 *   Check web audio session returns failure.
 *
 *  Step 4: Destroy web audio player session
 *   Destroy instance of WebAudioPlayer.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Fail to write buffer
 *
 * Code:
 */
TEST_F(WriteBufferFailuresTest, writeBufferFailures)
{
    // Step 1: Create a new web audio player session
    WebAudioPlayerTestMethods::shouldCreateWebAudioPlayer();
    WebAudioPlayerTestMethods::createWebAudioPlayer();
    WebAudioPlayerTestMethods::checkWebAudioPlayerClient();

    // Step 2: Get the available buffer
    WebAudioPlayerTestMethods::shouldGetBufferAvailable();
    WebAudioPlayerTestMethods::getBufferAvailable();

    // Step 3: Get the write buffer failure
    WebAudioPlayerTestMethods::shouldNotWriteBuffer();
    WebAudioPlayerTestMethods::doesNotWriteBuffer();
    WebAudioPlayerTestMethods::checkBuffer();

    // Step 4: Destroy web audio player session
    WebAudioPlayerTestMethods::shouldDestroyWebAudioPlayer();
    WebAudioPlayerTestMethods::destroyWebAudioPlayer();
}
} // namespace firebolt::rialto::client::ct
