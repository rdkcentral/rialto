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
#include "Constants.h"
#include "ExpectMessage.h"
#include "Matchers.h"
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"

using testing::_;
using testing::AtLeast;
using testing::Return;
using testing::StrEq;

namespace firebolt::rialto::server::ct
{
class SetPlaybackRateTest : public MediaPipelineTest
{
public:
    SetPlaybackRateTest() = default;
    ~SetPlaybackRateTest() = default;

    void setPipelineStatePlaying() { GST_STATE(&m_pipeline) = GST_STATE_PLAYING; }

    void willSetPlaybackRate()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("audio-sink"), _)).Times(AtLeast(1));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureNewDoubleStub(StrEq("custom-instant-rate-change"), StrEq("rate"),
                                                                 G_TYPE_DOUBLE, kPlaybackRate))
            .WillOnce(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &m_structure))
            .WillOnce(Return(&m_event));
        EXPECT_CALL(*m_gstWrapperMock, gstElementSendEvent(&m_pipeline, &m_event)).WillOnce(Return(TRUE));
    }

    void setPlaybackRate()
    {
        auto req{createSetPlaybackRateRequest(m_sessionId)};
        ConfigureAction<SetPlaybackRate>(m_clientStub).send(req).expectSuccess();
    }

private:
    GstStructure m_structure{};
    GstEvent m_event{};
};

/*
 * Component Test: Set Playback Rate
 * Test Objective:
 *  Test that set playback rate can be handled successfully.
 *
 * Sequence Diagrams:
 *  Set Playback Rate - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Step 4: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is in playing state
 *   Expect that server notifies the client that the Playback state has changed to PLAYING.
 *
 *  Step 5: SetPlaybackRate
 *   Send SetPlaybackRateRequest
 *   Expect that gstreamer is requested to change playback rate
 *   Expect that successful SetPlaybackRateResp is received
 *
 *  Step 6: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 7: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  Set playback rate succeeds in the playing state.
 *  Success response is returned to client.
 *
 * Code:
 */
TEST_F(SetPlaybackRateTest, SetPlaybackRate)
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

    // Step 4: Play
    willPlay();
    play();
    setPipelineStatePlaying();

    // Step 5: SetPlaybackRate
    willSetPlaybackRate();
    setPlaybackRate();

    // Step 6: Stop
    willStop();
    stop();

    // Step 7: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Set Playback Rate Failure
 * Test Objective:
 *  Test that set playback rate fails when session id is wrong.
 *
 * Sequence Diagrams:
 *  Set Playback Rate - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Step 4: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is in playing state
 *   Expect that server notifies the client that the Playback state has changed to PLAYING.
 *
 *  Step 5: SetPlaybackRate failure
 *   Send SetPlaybackRateRequest with wrong session id. Expect failure.
 *
 *  Step 6: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 7: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  Set playback rate fails.
 *  Failure is returned to the client.
 *
 * Code:
 */
TEST_F(SetPlaybackRateTest, SetPlaybackRateFailure)
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

    // Step 4: Play
    willPlay();
    play();
    setPipelineStatePlaying();

    // Step 5: SetPlaybackRate failure
    auto req{createSetPlaybackRateRequest(m_sessionId + 1)};
    ConfigureAction<SetPlaybackRate>(m_clientStub).send(req).expectFailure();

    // Step 6: Stop
    willStop();
    stop();

    // Step 7: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
