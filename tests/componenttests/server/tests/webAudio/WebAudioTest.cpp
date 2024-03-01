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
class WebAudioTest : public WebAudioTestMethods
{
public:
    WebAudioTest() {}
    virtual ~WebAudioTest() {}
};

/*
 * Component Test: Web Audio Player comprehensive tests
 * Test Objective:
 *  Test the complete web audio API with every protobuf call
 *  and gstreamer generated events.
 *
 * Sequence Diagrams:
 *  https://wiki.rdkcentral.com/display/ASP/Rialto+Web+Audio
 *  Initialisation & Termination
 *  GetBufferAvailable, WriteBuffer, Pause, Play, SetEOS
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
 *   We expect that a specific (and large) number of gstreamer calls are made
 *     to initialise playback
 *   Create an instance of WebAudioPlayer.
 *   gstreamer state becomes ready
 *   the server sends a WebAudioPlayerStateEvent message to the client to signal that it is IDLE
 *
 *  Step 2: Open and write data to shared mem
 *   A getSharedMemoryRequest is sent to the server (the server creates the shared mem)
 *   A webAudioGetBufferAvailable message is sent to the server.
 *   The return message indicates the free region(s) of shared memory
 *   and data is written that occupies 3/4 of the buffer. At the
 *   same time the data is stored in a FIFO structure so that the
 *   test harness can check that the same data is delivered to gstreamer
 *
 *  Step 3: Notify server that some shared memory is written
 *   A WebAudioWriteBuffer message will be sent to the server to
 *   notify that an area of the shared memory now contains audio data
 *   Expect that the audio data will be passed to gstreamer in the correct
 *   order (using the FIFO). When the expect calls are setup...
 *   the WebAudioWriteBuffer is sent
 *
 *  Step 4: Pause
 *   Expect that pause is propagated to the server (a gstreamer call will be made)
 *   Send WebAudioPause message to the server
 *   gstreamer notifies the server that it has paused
 *   the server notifies the clientApi that the state has changed to pause
 *
 *  Step 5: Play
 *   Expect that play is propagated to the server (a gstreamer call will be made)
 *   Send WebAudioPlay message to the server
 *   gstreamer notifies the server that it has started to play
 *   the server notifies the clientApi that the state has changed to play
 *
 *  Step 6: Check the audio data has been sent to gstreamer
 *   Verify that the data in the FIFO is now empty
 *   Send a webAudioGetBufferAvailable to the server to verify
 *   that the server thinks that the whole shared memory buffer
 *   is available again
 *
 *  Step 7: Write more data to shared memory IN CIRCLE
 *   The same as the previous step that wrote data to shared memory
 *   except that this time the data will be split because it circles
 *   back to the start of the buffer
 *       |------------------------------------------| BUFFER
 *       |*********************9-----------0********| Data circled 0***9
 *
 *  Step 8: Notify server that some shared memory is written
 *    The same as the previous step with the same name
 *
 *  Step 9: Check the audio data has been sent to gstreamer
 *    The same as the previous step with the same name
 *
 *  Step 10: Get Buffer Delay
 *   Expect that getBufferDelay is propagated to the server.
 *   Api call return the delay frames.
 *   Check delay frames.
 *
 *  Step 11: Get the device info
 *   Expect that getDeviceInfo is propagated to the server.
 *   Api call return the preferred frames, maximum frames and the supported deferred play.
 *   Check supported preferred frames, maximum frames and the supported deferred play.
 *
 *  Step 12: Set Volume
 *   Send a webAudioSetVolume message to the server for a certain level
 *   Expect that the server asks gstreamer to set the volume at the same level
 *
 *  Step 13: Get Volume
 *   Send a webAudioGetVolume message to the server
 *   Expect that the server asks gstreamer for the current volume level
 *     gstreamer returns a test value for the volume
 *   The test volume level is returned, via the server, to the client API
 *
 *  Step 14: Notify state to EOS
 *   Expect a call to gstreamer to notify that the end of stream is reached
 *   Client sends WebAudioSetEos to the server
 *   The server notifies gstreamer
 *   gstreamer notifies the server of the change of state
 *   The server notifies the client of the state change
 *
 *  Step 15: Destroy web audio player session
 *   A DestroyWebAudioPlayer message is sent to the server
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  To create a web audio player and process all of the above steps
 *  without any error
 *
 * Code:
 */
TEST_F(WebAudioTest, testAllApisWithMultipleQueries)
{
    // step 1: Create a new web audio player
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

    // Step 4: Pause
    willWebAudioPause();
    webAudioPause();
    sendStateChanged(GST_STATE_READY, GST_STATE_PAUSED, GST_STATE_NULL);
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_PAUSED);

    // Step 5: Play
    willWebAudioPlay();
    webAudioPlay();
    sendStateChanged(GST_STATE_PAUSED, GST_STATE_PLAYING, GST_STATE_NULL);
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_PLAYING);

    // Step 6: Check the audio data has been sent to gsteamer
    int avail = getBufferAvailable();
    EXPECT_EQ(avail, wholeLength);
    EXPECT_TRUE(m_dataFifo.empty());

    // Step 7: Write more data to shared memory IN CIRCLE
    sendDataToShm(lengthToWrite);

    // Step 8: Notify server that some shared memory is written
    willWebAudioWriteBuffer(lengthToWrite);
    webAudioWriteBuffer(lengthToWrite);

    // Step 9: Check the audio data has been sent to gsteamer
    avail = getBufferAvailable();
    EXPECT_EQ(avail, wholeLength);
    EXPECT_TRUE(m_dataFifo.empty());

    // Step 10: Get Buffer Delay
    willWebAudioGetBufferDelay();
    webAudioGetBufferDelay();

    // Step 11: Get the device info
    webAudioGetDeviceInfo();

    // Step 12: Set Volume
    willWebAudioSetVolume();
    webAudioSetVolume();

    // Step 13: Get Volume
    willWebAudioGetVolume();
    webAudioGetVolume();

    // Step 14: Notify state to EOS
    willWebAudioSetEos();
    webAudioSetEos();
    willGstSendEos();
    gstSendEos();
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_END_OF_STREAM);

    // Step 15: Destroy web audio player session
    destroyWebAudioPlayer();
}

} // namespace firebolt::rialto::server::ct
