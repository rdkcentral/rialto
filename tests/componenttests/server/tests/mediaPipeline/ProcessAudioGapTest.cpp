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
using testing::Return;
using testing::StrEq;

namespace
{
constexpr uint32_t kDuration{123};
constexpr int64_t kDiscontinuityGap{1};
constexpr bool kIsAudioAac{false};
} // namespace

namespace firebolt::rialto::server::ct
{
class ProcessAudioGapTest : public MediaPipelineTest
{
public:
    ProcessAudioGapTest() = default;
    ~ProcessAudioGapTest() = default;

    void willProcessAudioGap()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&m_audioAppSrc))).WillOnce(Return(&m_audioCaps));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsToString(&m_audioCaps)).WillOnce(Return(&m_audioCapsStr));
        EXPECT_CALL(*m_glibWrapperMock, gFree(&m_audioCapsStr));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_audioCaps));
        EXPECT_CALL(*(m_rdkGstreamerUtilsWrapperMock),
                    processAudioGap(&m_pipeline, kPosition, kDuration, kDiscontinuityGap, kIsAudioAac));
    }

    void processAudioGap()
    {
        auto req{createProcessAudioGapRequest(m_sessionId, kPosition, kDuration, kDiscontinuityGap)};
        ConfigureAction<ProcessAudioGap>(m_clientStub).send(req).expectSuccess();
    }

private:
    GstStructure m_structure{};
    GstEvent m_event{};
};

/*
 * Component Test: Process Audio Gap
 * Test Objective:
 *  Test that Process Audio Gap can be handled successfully.
 *
 * Sequence Diagrams:
 *  Process Audio Gap - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams
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
 *  Step 5: ProcessAudioGap
 *   Send ProcessAudioGapRequest
 *   Expect that rdk gstreamer utils is requested to process audio gap
 *   Expect that successful ProcessAudioGapResp is received
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
 *  Process Audio Gap succeeds.
 *  Success response is returned to client.
 *
 * Code:
 */
TEST_F(ProcessAudioGapTest, ProcessAudioGap)
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

    // Step 4: Play
    willPlay();
    play();

    // Step 5: ProcessAudioGap
    willProcessAudioGap();
    processAudioGap();

    // Step 6: Stop
    willStop();
    stop();

    // Step 7: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Process Audio Gap Failure
 * Test Objective:
 *  Test that Process Audio Gap fails when session id is wrong.
 *
 * Sequence Diagrams:
 *  Process Audio Gap - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams
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
 *  Step 5: ProcessAudioGap failure
 *   Send ProcessAudioGapRequest with wrong session id. Expect failure.
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
 *  Process Audio Gap fails.
 *  Failure is returned to the client.
 *
 * Code:
 */
TEST_F(ProcessAudioGapTest, ProcessAudioGapFailure)
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

    // Step 4: Play
    willPlay();
    play();

    // Step 5: ProcessAudioGap failure
    auto req{createProcessAudioGapRequest(m_sessionId + 1, kPosition, kDuration, kDiscontinuityGap)};
    ConfigureAction<ProcessAudioGap>(m_clientStub).send(req).expectFailure();

    // Step 6: Stop
    willStop();
    stop();

    // Step 7: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
