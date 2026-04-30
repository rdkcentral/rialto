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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "ExpectMessage.h"
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"

namespace
{
constexpr unsigned kFramesToPush{1};
constexpr int kFrameCountInPausedState{24};
} // namespace

namespace firebolt::rialto::server::ct
{
class HaveDataFailureTest : public MediaPipelineTest
{
public:
    HaveDataFailureTest() = default;
    ~HaveDataFailureTest() override = default;

    void failHaveData(std::shared_ptr<::firebolt::rialto::NeedMediaDataEvent> &needData)
    {
        ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};

        auto haveDataReq{createHaveDataRequest(m_sessionId, 0, needData->request_id())};
        haveDataReq.set_status(HaveDataRequest_MediaSourceStatus_ERROR);

        ConfigureAction<HaveData>(m_clientStub).send(haveDataReq).expectSuccess();

        auto receivedNeedData{expectedNeedData.getMessage()};
        ASSERT_TRUE(receivedNeedData);
        EXPECT_EQ(receivedNeedData->session_id(), m_sessionId);
        EXPECT_EQ(receivedNeedData->source_id(), needData->source_id());
        EXPECT_EQ(receivedNeedData->frame_count(), kFrameCountInPausedState);
        needData = receivedNeedData;
    }
};
/*
 * Component Test: Have Data Failure Test
 * Test Objective:
 *  Test the behaviour of Rialto Server, when it receives HaveData with Error status. Rialto Server should
 *  send next NeedMediaData to rialto server and be ready to continue playback.
 *
 * Sequence Diagrams:
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
 *  Step 5: Audio HaveData Failure
 *   Gstreamer Stub notifies, that it needs audio data
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *   Client Stub will send HaveData with Error status
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *
 *  Step 6: Video HaveData Failure
 *   Gstreamer Stub notifies, that it needs video data
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *   Client Stub will send HaveData with Error status
 *   Expect that server notifies the client that it needs 3 frames of video data.
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
 *  All API calls are handled by the server.
 *  Rialto Server sends new NeedMediaDataEvent after receiving Error HaveData.
 *
 * Code:
 */
TEST_F(HaveDataFailureTest, HaveDataError)
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
    indicateAllSourcesAttached({&m_audioAppSrc, &m_videoAppSrc});

    // Step 4: Pause
    willPause();
    pause();

    // Step 5: Audio HaveData Failure
    failHaveData(m_lastAudioNeedData);

    // Step 6: Video HaveData Failure
    failHaveData(m_lastVideoNeedData);

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
