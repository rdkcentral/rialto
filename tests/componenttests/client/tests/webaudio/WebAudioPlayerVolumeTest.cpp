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
class WebAudioPlayerVolumeTest : public ClientComponentTest
{
public:
    double m_volume = 0;

    WebAudioPlayerVolumeTest() : ClientComponentTest() { ClientComponentTest::startApplicationRunning(); }

    ~WebAudioPlayerVolumeTest() { ClientComponentTest::stopApplication(); }
};
/*
 * Component Test: Volume
 * Test Objective:
 *  Test the set volume and get delay api
 *
 * Sequence Diagrams:
 *  Set Volume and Get Volume -> https://wiki.rdkcentral.com/display/ASP/Rialto+Web+Audio
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: WebAudioPlayer
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
 *  Step 2: Set Volume(0.1)
 *   SetVolume to 10%.
 *   Expect that SetVolume propagated to the server.
 *
 *  Step 3: Get Volume
 *   GetVolume.
 *   Expect that GetVolume propagated to the server and returns the volume.
 *
 *  Step 4: Set Volume(1.0)
 *   SetVolume to 100%.
 *   Expect that SetVolume propagated to the server.
 *
 *  Step 5: Get Volume
 *   GetVolume.
 *   Expect that GetVolume propagated to the server and returns the volume.
 *
 *  Step 6: Destroy web audio player session
 *   Destroy instance of WebAudioPlayer.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can set the volume and get the volume succesfully.
 *
 * Code:
 */
TEST_F(WebAudioPlayerVolumeTest, webAudioPlayerVolume)
{
    // Step 1: Create a new web audio player session
    WebAudioPlayerTestMethods::shouldCreateWebAudioPlayer();
    WebAudioPlayerTestMethods::createWebAudioPlayer();
    WebAudioPlayerTestMethods::checkWebAudioPlayerClient();

    // Step 2:  Set Volume(0.1)
    m_volume = 0.1;
    WebAudioPlayerTestMethods::shouldSetVolume(m_volume);
    WebAudioPlayerTestMethods::setVolume(m_volume);

    // Step 3: Get Volume
    WebAudioPlayerTestMethods::shouldGetVolume(m_volume);
    WebAudioPlayerTestMethods::getVolume(m_volume);

    // Step 4: Set Volume(1)
    m_volume = 1.0;
    WebAudioPlayerTestMethods::shouldSetVolume(m_volume);
    WebAudioPlayerTestMethods::setVolume(m_volume);

    // Step 5: Get Volume
    WebAudioPlayerTestMethods::shouldGetVolume(m_volume);
    WebAudioPlayerTestMethods::getVolume(m_volume);

    // Step 6: Destroy web audio player session
    WebAudioPlayerTestMethods::shouldDestroyWebAudioPlayer();
    WebAudioPlayerTestMethods::destroyWebAudioPlayer();
}
} // namespace firebolt::rialto::client::ct
