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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "Constants.h"
#include "ExpectMessage.h"
#include "Matchers.h"
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"

namespace
{
constexpr unsigned kFramesToPush{1};
constexpr int kFrameCount{3};
constexpr bool kResetTime{false};
constexpr double kAppliedRate{2.0};
} // namespace

using testing::Return;

namespace firebolt::rialto::server::ct
{
class SetSourcePositionTest : public MediaPipelineTest
{
public:
    SetSourcePositionTest() = default;
    ~SetSourcePositionTest() override = default;

    void setSourcePosition(int sourceId)
    {
        // Send SetSourcePositionRequest and expect success
        auto request{
            createSetSourcePositionRequest(m_sessionId, sourceId, kPosition, kResetTime, kAppliedRate, kStopPosition)};
        ConfigureAction<SetSourcePosition>(m_clientStub).send(request).expectSuccess();
    }

    void setSourcePositionFailure()
    {
        auto request{createSetSourcePositionRequest(m_sessionId, m_audioSourceId, kPosition, kResetTime, kAppliedRate,
                                                    kStopPosition)};
        ConfigureAction<SetSourcePosition>(m_clientStub).send(request).expectFailure();
    }
};

/*
 * Component Test: Set Source Position success
 * Test Objective:
 *  Test that Set Source Position is successfully handled.
 *
 * Sequence Diagrams:
 *  Set Source Position - https://wiki.rdkcentral.com/display/ASP/Rialto+Flush+and+Seek+Design
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
 *  Step 5: Write 1 audio frame
 *   Gstreamer Stub notifies, that it needs audio data
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *   Write 1 frame of audio data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *
 *  Step 6: Write 1 video frame
 *   Gstreamer Stub notifies, that it needs video data
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *   Write 1 frame of video data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *
 *  Step 7: Notify buffered and Paused
 *   Expect that server notifies the client that the Network state has changed to BUFFERED.
 *   Gstreamer Stub notifies, that pipeline state is in PAUSED state
 *   Expect that server notifies the client that the Network state has changed to PAUSED.
 *
 *  Step 8: Set Audio Source Position
 *   Trigger set source position procedure for audio source
 *   Expect that SetSourcePositionResponse has success status
 *   Expect that after HaveData, new audio sample is pushed
 *
 *  Step 9: Set Video Source Position
 *   Trigger set source position procedure for video source
 *   Expect that SetSourcePositionResponse has success status
 *   Expect that after HaveData, new video sample is pushed
 *
 *  Step 10: End of audio stream
 *   Send audio haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 11: End of video stream
 *   Send video haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 12: Notify end of stream
 *   Simulate, that gst_message_eos is received by Rialto Server
 *   Expect that server notifies the client that the Network state has changed to END_OF_STREAM.
 *
 *  Step 13: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 14: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 15: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  Set Source Position is handled and forwarded to the gstreamer.
 *
 * Code:
 */
TEST_F(SetSourcePositionTest, setSourcePositionSuccess)
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

    // Step 5: Write 1 audio frame
    // Step 6: Write 1 video frame
    // Step 7: Notify buffered and Paused
    gstNeedData(&m_audioAppSrc, kFrameCount);
    gstNeedData(&m_videoAppSrc, kFrameCount);
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushAudioData(kFramesToPush, kFrameCount);
        pushVideoData(kFramesToPush, kFrameCount);

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);
    }
    willNotifyPaused();
    notifyPaused();

    // Step 8: Set Audio Source Position
    setSourcePosition(m_audioSourceId);
    pushAudioSample(kFrameCount);

    // Step 9: Set Video Source Position
    setSourcePosition(m_videoSourceId);
    pushVideoSample(kFrameCount);

    // Step 10: End of audio stream
    // Step 11: End of video stream
    willEos(&m_audioAppSrc);
    eosAudio(kFramesToPush);
    willEos(&m_videoAppSrc);
    eosVideo(kFramesToPush);

    // Step 12: Notify end of stream
    gstNotifyEos();

    // Step 13: Remove sources
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 14: Stop
    willStop();
    stop();

    // Step 15: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Set Source Position failure
 * Test Objective:
 *  Test that Set Source Position failure is handled.
 *
 * Sequence Diagrams:
 *  Set Source Position - https://wiki.rdkcentral.com/display/ASP/Rialto+Flush+and+Seek+Design
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
 *  Step 3: Set Source Position Failure
 *   SetSourcePositionRequest sent for unknown source
 *   Expect that SetSourcePositionResponse has error status
 *
 *  Step 4: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 5: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  Set Source Position failure status is forwarded to client
 *
 * Code:
 */
TEST_F(SetSourcePositionTest, SetSourcePositionFailure)
{
    // Step 1: Create a new media session
    createSession();

    // Step 2: Load content
    gstPlayerWillBeCreated();
    load();

    // Step 3: Set Source Position Failure
    setSourcePositionFailure();

    // Step 4: Stop
    willStop();
    stop();

    // Step 5: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
