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

#include "WebAudioTestMethods.h"

using ::firebolt::rialto::WebAudioPlayerStateEvent;

namespace firebolt::rialto::server::ct
{
class WebAudioFailuresTest : public WebAudioTestMethods
{
public:
    WebAudioFailuresTest() {}
    virtual ~WebAudioFailuresTest() {}
};

/*
 * Component Test: Web Audio Player failure tests
 * Test Objective:
 *  Test the Play and Pause failure cases documented on the sequence diagram
 *
 * Sequence Diagrams:
 *  https://wiki.rdkcentral.com/display/ASP/Rialto+Web+Audio
 *    Play & Pause
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: WebAudioPlayer
 *
 * Test Initialize:
 *  Create stubs for client and gstreamer
 *  Start the server running in it's own thread
 *
 * Test Steps:
 *  Step 1: Create a new web audio player
 *   Same as Step 1 in WebAudioTest.cpp
 *   We expect that a specific (and large) number of gstreamer calls are made
 *     to initialise playback
 *   Create an instance of WebAudioPlayer.
 *   gstreamer state becomes ready
 *   the server sends a WebAudioPlayerStateEvent message to the client to signal that it is IDLE
 *
 *  Step 2: Open and write data to shared mem
 *   Same as Step 2 in WebAudioTest.cpp
 *   A getSharedMemoryRequest is sent to the server (the server creates the shared mem)
 *   A webAudioGetBufferAvailable message is sent to the server.
 *   The return message indicates the free region(s) of shared memory
 *   and data is written that occupies 3/4 of the buffer. At the
 *   same time the data is stored in a FIFO structure so that the
 *   test harness can check that the same data is delivered to gstreamer
 *
 *  Step 3: Notify server that some shared memory is written
 *   Same as Step 3 in WebAudioTest.cpp
 *   A WebAudioWriteBuffer message will be sent to the server to
 *   notify that an area of the shared memory now contains audio data
 *   Expect that the audio data will be passed to gstreamer in the correct
 *   order (using the FIFO). When the expect calls are setup...
 *   the WebAudioWriteBuffer is sent
 *
 *  Step 4: Fail to Play
 *   Expect that play is propagated to the server (a gstreamer call will be made)
 *   Send WebAudioPlay message to the server
 *   gstreamer notifies the server that an error has occurred
 *   the server notifies the clientApi of the error
 *
 *  Step 5: Play
 *   Same as Step 5 in WebAudioTest.cpp
 *   Expect that play is propagated to the server (a gstreamer call will be made)
 *   Send WebAudioPlay message to the server
 *   gstreamer notifies the server that it has started to play
 *   the server notifies the clientApi that the state has changed to play
 *
 *  Step 6: Fail to Pause
 *   Expect that pause is propagated to the server (a gstreamer call will be made)
 *   Send WebAudioPause message to the server
 *   gstreamer notifies the server that an error has occurred
 *   the server notifies the clientApi that an error has occurred
 *
 *  Step 7: Notify state to EOS
 *   Expect a call to gstreamer to notify that the end of stream is reached
 *   Client sends WebAudioSetEos to the server
 *   The server notifies gstreamer
 *   gstreamer notifies the server of the change of state
 *   The server notifies the client of the state change
 *
 *  Step 8: Destroy web audio player session
 *   A DestroyWebAudioPlayer message is sent to the server
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  To create a web audio player and emulate playback and pause failures
 *  and the server correctly notifies the client
 *
 * Code:
 */
TEST_F(WebAudioFailuresTest, testPlayAndPauseFailures)
{
    // Step 1: Create a new web audio player
    willCreateWebAudioPlayer();
    createWebAudioPlayer();
    sendStateChanged(GST_STATE_NULL, GST_STATE_READY, GST_STATE_NULL);
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_IDLE);

    // Step 2: Open and write data to shared mem
    int wholeLength = checkInitialBufferAvailable();
    int lengthToWrite = wholeLength * 3 / 4;
    initShm();
    sendDataToShm(lengthToWrite);

    // Step 3: Notify server that some shared memory is written
    willWebAudioWriteBuffer(lengthToWrite);
    webAudioWriteBuffer(lengthToWrite);

    // Step 4: Fail to play
    willWebAudioPlayFail();
    webAudioPlay();
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_FAILURE);

    // Step 5: Play
    willWebAudioPlay();
    webAudioPlay();
    sendStateChanged(GST_STATE_NULL, GST_STATE_PLAYING, GST_STATE_NULL);
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_PLAYING);

    // Step 6: Fail to pause
    willWebAudioPauseFail();
    webAudioPause();
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_FAILURE);

    // Step 7: Notify state to EOS
    willWebAudioSetEos();
    webAudioSetEos();
    willGstSendEos();
    gstSendEos();
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_END_OF_STREAM);

    // Step 8: Destroy web audio player session
    destroyWebAudioPlayer();
}

/*
 * Component Test: Web Audio Player creation failure test
 * Test Objective:
 *  Test a failure to create the Web Audio Player
 *
 * Sequence Diagrams:
 *  https://wiki.rdkcentral.com/display/ASP/Rialto+Web+Audio
 *    Create Web Audio Player session
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: WebAudioPlayer
 *
 * Test Initialize:
 *  Create stubs for client and gstreamer
 *  Start the server running in it's own thread
 *
 * Test Steps:
 *  Step 1: Failure to create a new web audio player
 *   We emlate failure of the gstreamer calls
 *   Attempt to create an instance of WebAudioPlayer.
 *   gstreamer fails
 *   the server sends a failure reponse message to the client
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  Failure is reported to the client successfully
 *
 * Code:
 */
TEST_F(WebAudioFailuresTest, failToCreateWebAudio)
{
    // Step 1: Create a new web audio player
    willFailToCreateWebAudioPlayer();
    failToCreateWebAudioPlayer();
}

} // namespace firebolt::rialto::server::ct
