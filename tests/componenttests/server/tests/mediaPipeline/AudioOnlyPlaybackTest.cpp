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

#include "ExpectMessage.h"
#include "MediaPipelineTest.h"

namespace
{
constexpr unsigned kFramesToPush{1};
constexpr int kFrameCountInPausedState{3};
constexpr int kFrameCountInPlayingState{24};
} // namespace

/*
 * Component Test: Audio only Playback Sequence
 * Test Objective:
 *  Test the playback of single audio MSE content. The test transitions through the playback states
 *  buffering 1 frame of audio content before preroll and 1 frame of audio
 *  content after preroll. The session is then terminated. All the metadata and media data written to the
 *  shared buffer is checked for accuracy.
 *
 * Sequence Diagrams:
 *  Create, Destroy - https://wiki.rdkcentral.com/pages/viewpage.action?pageId=226375556
 *  Start/Resume Playback, Pause Playback, Stop, End of stream
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *  Map Shared Memory
 *
 * Test Steps:
 *  Step 1: Create a new media session
 *   Send CreateSessionRequest to Rialto Server
 *   Expect that successful CreateSessionResponse is received
 *   Save returned session id
 *
 *  Step 2: Load content
 *   Send LoadRequest to Rialto Server
 *   Expect that successful LoadResponse is received
 *   Expect that GstPlayer instance is created.
 *   Expect that client is notified that the NetworkState has changed to BUFFERING.
 *
 *  Step 3: Attach audio source
 *   Attach the audio source.
 *   Expect that audio source is attached.
 *   Expect that rialto source is setup
 *   Expect that all sources are attached.
 *   Expect that the Playback state has changed to IDLE.
 *
 *  Step 4: Pause
 *   Pause the content.
 *   Expect that gstreamer pipeline is paused.
 *
 *  Step 5: Write 1 audio frame
 *   Gstreamer Stub notifies, that it needs audio data
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *   Write 1 frame of audio data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *
 *  Step 6: Notify buffered
 *   Expect that server notifies the client that the Network state has changed to BUFFERED.
 *
 *  Step 7: Notify Paused
 *   Gstreamer Stub notifies, that pipeline state is in PAUSED state
 *   Expect that server notifies the client that the Network state has changed to PAUSED.
 *
 *  Step 8: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is in playing state
 *   Expect that server notifies the client that the Playback state has changed to PLAYING.
 *
 *  Step 9: Write 1 audio frame
 *   Write 1 frame of audio data to the shared buffer.
 *   Send HaveData
 *   Expect that server notifies the client that it needs 24 frames of audio data.
 *
 *  Step 10: End of audio stream
 *   Send audio haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 11: Notify end of stream
 *   Simulate, that gst_message_eos is received by Rialto Server
 *   Expect that server notifies the client that the Network state has changed to END_OF_STREAM.
 *
 *  Step 12: Remove source
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *
 *  Step 13: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 14: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are handled by the server.
 *  The state of the Gstreamer Pipeline is successfully negotiationed in the normal playback scenario.
 *  Data is successfully read from the shared memory and pushed to gstreamer pipeline for both audio and video.
 *
 * Code:
 */
namespace firebolt::rialto::server::ct
{
TEST_F(MediaPipelineTest, AudioOnlyPlayback)
{
    // Step 1: Create a new media session
    createSession();

    // Step 2: Load content
    gstPlayerWillBeCreated();
    load();

    // Step 3: Attach audio source
    audioSourceWillBeAttached();
    attachAudioSource();
    sourceWillBeSetup();
    setupSource();
    willSetupAndAddSource(&m_audioAppSrc);
    willFinishSetupAndAddSource();
    indicateAllSourcesAttached({&m_audioAppSrc});

    // Step 4: Pause
    willPause();
    pause();

    // Step 5: Write 1 audio frame
    // Step 6: Notify buffered
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushAudioData(kFramesToPush);

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);
    }

    // Step 7: Notify Paused
    willNotifyPaused();
    notifyPaused();

    // Step 8: Play
    willPlay();
    play();

    // Step 9: Write 1 audio frame
    pushAudioData(kFramesToPush);

    // Step 10: End of audio stream
    willEos(&m_audioAppSrc);
    eosAudio(kFramesToPush);

    // Step 11: Notify end of stream
    gstNotifyEos();

    // Step 12: Remove source
    removeSource(m_audioSourceId);

    // Step 13: Stop
    willStop();
    stop();

    // Step 14: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
