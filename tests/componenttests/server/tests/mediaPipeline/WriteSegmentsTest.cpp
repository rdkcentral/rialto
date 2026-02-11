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
constexpr unsigned kFramesToPushBeforePreroll{3};
constexpr unsigned kFramesToPushAfterPreroll{4};
constexpr int kFrameCountInPausedState{3};
constexpr int kFrameCountInPlayingState{24};
} // namespace

/*
 * Component Test: Audio Video Playback Write Segments
 * Test Objective:
 *  Test the writting of audio and video segments during playback. The test starts off with a media session in the
 *  wait for preroll state ready for media segments to be injected. The test buffers 3 frames of both audio
 *  and video before preroll and 4 frames of both audio and video after preroll, this is to check that
 *  needData/haveData is fullilled with more than one frame added to the media session. All the metadata and media
 *  data written to the shared buffer is checked for accuracy.
 *
 * Sequence Diagrams:
 *  Shared memory buffer refill - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Step 3: Attach all sources
 *   Attach the audio source.
 *   Expect that audio source is attached.
 *   Attach the video source.
 *   Expect that video source is attached.
 *   Expect that rialto source is setup
 *   Expect that all sources are attached.
 *   Expect that the Playback state has changed to IDLE.
 *
 *  Step 4: Pause
 *   Pause the content.
 *   Expect that gstreamer pipeline is paused.
 *
 *  Step 5: Write 3 audio frames
 *   Gstreamer Stub notifies, that it needs audio data
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *   Write 3 frames of audio data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *
 *  Step 6: Write 3 video frames
 *   Gstreamer Stub notifies, that it needs video data
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *   Write 3 frames of video data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *
 *  Step 7: Notify buffered and Paused
 *   Expect that server notifies the client that the Network state has changed to BUFFERED.
 *   Gstreamer Stub notifies, that pipeline state is in PAUSED state
 *   Expect that server notifies the client that the Network state has changed to PAUSED.
 *
 *  Step 8: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is in playing state
 *   Expect that server notifies the client that the Playback state has changed to PLAYING.
 *
 *  Step 9: Write 3 audio frames
 *   Write 3 frames of audio data to the shared buffer.
 *   Send HaveData
 *   Expect that server notifies the client that it needs 24 frames of audio data.
 *
 *  Step 10: Write 3 video frames
 *   Write 3 frames of video data to the shared buffer.
 *   Send HaveData
 *   Expect that server notifies the client that it needs 24 frames of video data.
 *
 *  Step 11: Send 4 frames and end of audio stream
 *   Send audio haveData with 4 frames and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 12: Send 4 frames and end of video stream
 *   Send video haveData with 4 frames and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 13: Notify end of stream
 *   Simulate, that gst_message_eos is received by Rialto Server
 *   Expect that server notifies the client that the Network state has changed to END_OF_STREAM.
 *
 *  Step 14: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 15: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 16: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  Rialto client can handle the addition of > 1 frames
 *  before and after preroll. Data is successfully written to the shared memory for both audio and video.
 *
 * Code:
 */
namespace firebolt::rialto::server::ct
{
TEST_F(MediaPipelineTest, WriteSegments)
{
    // Step 1: Create a new media session
    createSession();

    // Step 2: Load content
    gstPlayerWillBeCreated();
    load();

    // Step 3: Attach all sources
    audioSourceWillBeAttached();
    attachAudioSource();
    videoSourceWillBeAttached();
    attachVideoSource();
    sourceWillBeSetup();
    setupSource();
    willSetupAndAddSource(&m_audioAppSrc);
    willSetupAndAddSource(&m_videoAppSrc);
    willFinishSetupAndAddSource();
    indicateAllSourcesAttached();

    // Step 4: Pause
    willPause();
    pause();

    // Step 5: Write 3 audio frames
    // Step 6: Write 3 video frames
    // Step 7: Notify buffered and Paused
    gstNeedData(&m_audioAppSrc, kFrameCountInPausedState);
    gstNeedData(&m_videoAppSrc, kFrameCountInPausedState);
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushAudioData(kFramesToPushBeforePreroll, kFrameCountInPausedState);
        pushVideoData(kFramesToPushBeforePreroll, kFrameCountInPausedState);

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);
    }
    willNotifyPaused();
    notifyPaused();

    // Step 8: Play
    willPlay();
    play();

    // Step 9: Write 3 audio frames
    // Step 10: Write 3 video frames
    pushAudioData(kFramesToPushBeforePreroll, kFrameCountInPlayingState);
    pushVideoData(kFramesToPushBeforePreroll, kFrameCountInPlayingState);

    // Step 11: Send 4 frames and end of audio stream
    // Step 12: Send 4 frames and end of video stream
    willEos(&m_audioAppSrc);
    eosAudio(kFramesToPushAfterPreroll);
    willEos(&m_videoAppSrc);
    eosVideo(kFramesToPushAfterPreroll);

    // Step 13: Notify end of stream
    gstNotifyEos();

    // Step 14: Remove sources
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 15: Stop
    willStop();
    stop();

    // Step 16: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
