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
class PlayPauseFailuresTest : public ClientComponentTest
{
public:
    PlayPauseFailuresTest() : ClientComponentTest() { ClientComponentTest::startApplicationRunning(); }

    ~PlayPauseFailuresTest() { ClientComponentTest::stopApplication(); }
};

/*
 * Component Test: Play and Pause Api Failures
 * Test Objective:
 *  Check that failures returned directly from the Play and Pause apis and failures returned asyncronously
 *  during server WebAudioPlayer state changes are handled correctly. Subsequent Api requests after failures are successful.
 *
 * Sequence Diagrams:
 *  Play and Pause Api Failures -> https://wiki.rdkcentral.com/display/ASP/Rialto+Web+Audio
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
 *
 *  Step 1: Create a new web audio player session
 *   Create an instance of WebAudioPlayer.
 *   Expect that web audio api is called on the server
 *   Check that the object returned is valid.
 *   Check that web audio player has been added.
 *
 *  Step 2: Play api failure
 *   Play the content.
 *   Expect that play propagated to the server and return failure.
 *
 *  Step 3: Playing state failure
 *   play().
 *   Expect that play propagated to the server.
 *   Server notifies the client that the WebAudioPlayer state has changed to FAILURE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 4: Notify state to FAILURE
 *   WebAudioPlayerStateChange to FAILURE
 *
 *  Step 5: Play
 *   play().
 *   Expect that play is propagated to the server.
 *   Api call return the status.
 *   Check status is play.
 *
 *  Step 6: Notify state to PLAY
 *   WebAudioPlayerStateChange to PLAY
 *
 *  Step 7: Pause api failure
 *   Pause the content.
 *   Expect that pause propagated to the server and return failure.
 *
 *  Step 8: Pause state failure
 *   Pause().
 *   Expect that pause propagated to the server.
 *   Server notifies the client that the WebAudioPlayer state has changed to FAILURE.
 *   Expect that the state change notification is propagated to the client.
 *
 *   Step 9: Notify state to FAILURE
 *   WebAudioPlayerStateChange to FAILURE
 *
 *  Step 10: Pause
 *   pause().
 *   Expect that pause is propagated to the server.
 *   Api call return the status.
 *   Check status is paused.
 *
 *  Step 11: Notify state to PAUSE
 *   WebAudioPlayerStateChange to PAUSE
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
 *  All failures are notified to the calling application.
 *  Failures are recoverable.
 *
 * Code:
 */
TEST_F(PlayPauseFailuresTest, playPauseFailures)
{
    // Step 1: Create a new web audio player session
    WebAudioPlayerTestMethods::shouldCreateWebAudioPlayer();
    WebAudioPlayerTestMethods::createWebAudioPlayer();
    WebAudioPlayerTestMethods::checkWebAudioPlayerClient();

    // Step 2: Play api failure
    WebAudioPlayerTestMethods::shouldNotPlay();
    WebAudioPlayerTestMethods::doesNotPlay();

    // Step 3: Playing state failure
    WebAudioPlayerTestMethods::shouldPlay();
    WebAudioPlayerTestMethods::play();

    // Step 4: Notify state to FAILURE
    WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStateFailure();
    WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStateFailure();

    // Step 5: Play
    WebAudioPlayerTestMethods::shouldPlay();
    WebAudioPlayerTestMethods::play();

    // Step 6: Notify state to PLAY
    WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStatePlay();
    WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStatePlay();

    // Step 7: Pause api failure
    WebAudioPlayerTestMethods::shouldNotPause();
    WebAudioPlayerTestMethods::doesNotPause();

    // Step 8: Pause state failure
    WebAudioPlayerTestMethods::shouldPause();
    WebAudioPlayerTestMethods::pause();

    // Step 9: Notify state to FAILURE
    WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStateFailure();
    WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStateFailure();

    // Step 10: Pause
    WebAudioPlayerTestMethods::shouldPause();
    WebAudioPlayerTestMethods::pause();

    // Step 11: Notify state to PAUSE
    WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStatePause();
    WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStatePause();

    // Step 12: Destroy web audio player session
    WebAudioPlayerTestMethods::shouldDestroyWebAudioPlayer();
    WebAudioPlayerTestMethods::destroyWebAudioPlayer();
}
} // namespace firebolt::rialto::client::ct
