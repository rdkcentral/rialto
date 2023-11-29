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
 *  Step 2: Load content
 *   Load MSE.
 *   Expect that load is propagated to the server.
 *   Server notifys the client that the NetworkState has changed to BUFFERING.
 *   Expect that the state change notification is propagated to the client.
 * 
 *  Step 3: Pause
 *
 *  Step 4: Attach all sources
 *   Attach the video source.
 *   Expect that attach source for video propagated to the server.
 *   Attach the audio source.
 *   Expect thatattach source for audio propagated to the server.
 *   Set all sources attached.
 *   Expect that all source attached is propagated to the server.
 *   Server notifys the client that the Playback state has changed to IDLE.
 *   Expect that the state change notification is propagated to the client.
 * 
 *  Step 5: Add samples x ?
 *   Server notifys the client that it needsData for 3 frames.
 *   Writes data to the shared buffer.
 *   Notify the server that the data has been written.
 *   Server notifys the client that the Network state has changed to BUFFERED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 6: Server paused
 *   Server notifys the client that the Network state has changed to PAUSED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 7: Set video window
 *
 *  Step 8: Play
 *   Play.
 *   Expect that play propagated to the server.
 *   Server notifys the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *   
 *  Step 9: Add samples x ?
 *   Server notifys the client that it needsData for 20 frames.
 *   Writes data to the shared buffer.
 *   Notify the server that the data has been written.
 *   Server notifys the client that the Network state has changed to BUFFERED.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 10: Add last samples
 *   Server notifys the client that it needsData for 20 frames.
 *   Writes data to the shared buffer.
 *   Send EOS with samples.
 *   Server notifys the client that the Network state has changed to END_OF_STREAM.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 11: Remove sources
 *   Remove the video source.
 *   Expect that remove source for video propagated to the server.
 *   Remove the audio source.
 *   Expect that remove source for audio propagated to the server.
 *
 *  Step 11: Stop
 *
 *  Step 12: Destroy media session
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
