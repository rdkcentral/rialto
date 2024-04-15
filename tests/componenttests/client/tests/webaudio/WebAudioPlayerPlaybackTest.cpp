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
class WebAudioPlayerPlaybackTest : public ClientComponentTest
{
public:
    WebAudioPlayerPlaybackTest() : ClientComponentTest() { ClientComponentTest::startApplicationRunning(); }

    ~WebAudioPlayerPlaybackTest() { ClientComponentTest::stopApplication(); }
};

/*
 * Component Test: Web Audio Player Playback
 * Test Objective:
 *  Test the playback of web audio player.
 *
 * Sequence Diagrams:
 *  GetBufferAvailable, WriteBuffer, Pause, Play, SetEOS -> https://wiki.rdkcentral.com/display/ASP/Rialto+Web+Audio
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
 *  Step 2: Get the available buffer
 *   getBufferAvailable.
 *   Expect that getBufferAvailable is propagated to the server.
 *   Api call return the available frames and web audio shm info.
 *   Check available frames and web audio shm info.
 *
 *  Step 3: Get the write buffer
 *   writeBuffer.
 *   Expect that writeBuffer is propagated to the server.
 *   Api call return the web audio session, number of frames and data.
 *   Check web audio session, number of frames and data.
 *
 *  Step 4: Pause
 *   pause().
 *   Expect that pause is propagated to the server.
 *   Api call return the status.
 *   Check status is paused.
 *
 *  Step 5: Notify state to PAUSE
 *   WebAudioPlayerStateChange to PAUSE
 *
 *  Step 6: Play
 *   play().
 *   Expect that play is propagated to the server.
 *   Api call return the status.
 *   Check status is play.
 *
 *  Step 7: Notify state to PLAY
 *   WebAudioPlayerStateChange to PLAY
 *
 *  Step 8: Get the available buffer
 *   getBufferAvailable.
 *   Expect that getBufferAvailable is propagated to the server.
 *   Api call return the available frames and web audio shm info.
 *   Check available frames and web audio shm info.
 *
 *  Step 9: Get the write buffer
 *   writeBuffer.
 *   Expect that writeBuffer is propagated to the server.
 *   Api call return the web audio session, number of frames and data.
 *   Check web audio session, number of frames and data.
 *
 *  Step 10: Set end of stream
 *   setEos.
 *   Expect that setEos is propagated to the server.
 *   Api call returns a status of true.
 *   Check return status is true.
 *
 *  Step 11: Notify state to EOS
 *   WebAudioPlayerStateChange to EOS
 *
 *  Step 12: Destroy web audio player session
 *   Destroy instance of WebAudioPlayer.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  To create a web audio player gracefully.
 *  To get the available buffer and write to the buffer correctly.
 *  To pause, play and set end of stream successfully.
 *
 * Code:
 */
TEST_F(WebAudioPlayerPlaybackTest, webAudioPlayerPlayback)
{
    // Step 1: Create a new web audio player session
    WebAudioPlayerTestMethods::shouldCreateWebAudioPlayer();
    WebAudioPlayerTestMethods::createWebAudioPlayer();
    WebAudioPlayerTestMethods::checkWebAudioPlayerClient();

    // Step 2: Get the available buffer
    WebAudioPlayerTestMethods::shouldGetBufferAvailable();
    WebAudioPlayerTestMethods::getBufferAvailable();

    // Step 3: Get the write buffer
    WebAudioPlayerTestMethods::shouldWriteBuffer();
    WebAudioPlayerTestMethods::writeBuffer();
    WebAudioPlayerTestMethods::checkBuffer();

    // Step 4: Pause
    WebAudioPlayerTestMethods::shouldPause();
    WebAudioPlayerTestMethods::pause();

    // Step 5: Notify state to PAUSE
    WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStatePause();
    WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStatePause();

    // Step 6: Play
    WebAudioPlayerTestMethods::shouldPlay();
    WebAudioPlayerTestMethods::play();

    // Step 7: Notify state to PLAY
    WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStatePlay();
    WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStatePlay();

    // Step 8: Get the available buffer
    WebAudioPlayerTestMethods::shouldGetBufferAvailable();
    WebAudioPlayerTestMethods::getBufferAvailable();

    // Step 9: Get the write buffer
    WebAudioPlayerTestMethods::shouldWriteBuffer();
    WebAudioPlayerTestMethods::writeBuffer();
    WebAudioPlayerTestMethods::checkBuffer();

    // Step 10: Set end of stream
    WebAudioPlayerTestMethods::shouldEos();
    WebAudioPlayerTestMethods::setEos();

    // Step 11: Notify state to EOS
    WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStateEos();
    WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStateEos();

    // Step 12: Destroy web audio player session
    WebAudioPlayerTestMethods::shouldDestroyWebAudioPlayer();
    WebAudioPlayerTestMethods::destroyWebAudioPlayer();
}
} // namespace firebolt::rialto::client::ct
