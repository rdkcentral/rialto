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

using testing::Return;

namespace
{
constexpr unsigned kFramesToPush{1};
constexpr int kFrameCountInPausedState{3};
} // namespace

namespace firebolt::rialto::server::ct
{
class PlayPauseStopFailuresTest : public MediaPipelineTest
{
public:
    PlayPauseStopFailuresTest() = default;
    ~PlayPauseStopFailuresTest() override = default;

    void willFailToPause()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PAUSED))
            .WillOnce(Return(GST_STATE_CHANGE_FAILURE));
    }

    void willFailToPlay()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PLAYING))
            .WillOnce(Return(GST_STATE_CHANGE_FAILURE));
    }

    void willFailToStop()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_NULL))
            .WillOnce(Return(GST_STATE_CHANGE_FAILURE));
    }

    void pauseAndExpectFailure()
    {
        ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange{m_clientStub};

        auto pauseReq{createPauseRequest(m_sessionId)};
        ConfigureAction<Pause>(m_clientStub).send(pauseReq).expectSuccess();

        auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
        ASSERT_TRUE(receivedPlaybackStateChange);
        EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedPlaybackStateChange->state(),
                  ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_FAILURE);
    }

    void playAndExpectFailure()
    {
        ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange{m_clientStub};

        auto playReq{createPlayRequest(m_sessionId)};
        ConfigureAction<Play>(m_clientStub).send(playReq).expectSuccess();

        auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
        ASSERT_TRUE(receivedPlaybackStateChange);
        EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedPlaybackStateChange->state(),
                  ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_FAILURE);
    }

    void stopAndExpectFailure()
    {
        ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange{m_clientStub};

        auto stopReq{createStopRequest(m_sessionId)};
        ConfigureAction<Stop>(m_clientStub).send(stopReq).expectSuccess();

        auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
        ASSERT_TRUE(receivedPlaybackStateChange);
        EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedPlaybackStateChange->state(),
                  ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_FAILURE);
    }
};
/*
 * Component Test: Pause Api Failure
 * Test Objective:
 *  Check that Pause response failure and PlaybackState failure is returned directly after the gstreamer api failure
 *  during server state change to Paused. Subsequent Api requests after failures are successful.
 *
 * Sequence Diagrams:
 *  Start/Resume Playback, Pause Playback, Stop - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Step 4: Pause will fail
 *   Pause the content.
 *   Expect that error response is returned when gstreamer API fails
 *   Expect that server notifies the client that the Playback state has changed to FAILURE.
 *
 *  Step 5: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 6: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  All failures are notified to the calling application.
 *  Failures are recoverable.
 *
 * Code:
 */
TEST_F(PlayPauseStopFailuresTest, PauseFailure)
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

    // Step 4: Pause will fail
    willFailToPause();
    pauseAndExpectFailure();

    // Step 5: Stop
    willStop();
    stop();

    // Step 6: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Play Api Failure
 * Test Objective:
 *  Check that Play response failure and PlaybackState failure is returned directly after the gstreamer api failure
 *  during server state change to Playing. Subsequent Api requests after failures are successful.
 *
 * Sequence Diagrams:
 *  Start/Resume Playback, Play Playback, Stop - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Step 4: Play will fail
 *   Play the content.
 *   Expect that error response is returned when gstreamer API fails
 *   Expect that server notifies the client that the Playback state has changed to FAILURE.
 *
 *  Step 5: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 6: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  All failures are notified to the calling application.
 *  Failures are recoverable.
 *
 * Code:
 */
TEST_F(PlayPauseStopFailuresTest, PlayFailure)
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

    // Step 4: Play will fail
    willFailToPlay();
    playAndExpectFailure();

    // Step 5: Stop
    willStop();
    stop();

    // Step 6: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Stop Api Failure
 * Test Objective:
 *  Check that Stop response failure and PlaybackState failure is returned directly after the gstreamer api failure
 *  during server state change to Stopped. Subsequent Api requests after failures are successful.
 *
 * Sequence Diagrams:
 *  Start/Resume Playback, Stop Playback, Stop - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
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
 *  Step 4: Stop will fail
 *   Stop the content.
 *   Expect that error response is returned when gstreamer API fails
 *   Expect that server notifies the client that the Playback state has changed to FAILURE.
 *
 *  Step 5: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 6: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  All failures are notified to the calling application.
 *  Failures are recoverable.
 *
 * Code:
 */
TEST_F(PlayPauseStopFailuresTest, StopFailure)
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

    // Step 4: Stop will fail
    willFailToStop();
    stopAndExpectFailure();

    // Step 5: Stop
    willStop();
    stop();

    // Step 6: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
