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
#include "Matchers.h"
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"

namespace
{
constexpr unsigned kFramesToPush{1};
constexpr int kPositionInPaused{10};
constexpr int kPositionInPlaying{0};
constexpr double kPlaybackRate{1.0};
} // namespace

using testing::Return;

namespace firebolt::rialto::server::ct
{
class SetPositionTest : public MediaPipelineTest
{
public:
    SetPositionTest() = default;
    ~SetPositionTest() override = default;

    void willSetPosition(std::int64_t position)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementSeek(&m_pipeline, kPlaybackRate, GST_FORMAT_TIME,
                                                      static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH), GST_SEEK_TYPE_SET,
                                                      position, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
            .WillOnce(Return(TRUE));
    }

    void setPosition(std::int64_t position)
    {
        // After successful SetPosition, NeedDatas are sent for audio and video. Add expects for them
        ExpectMessage<NeedMediaDataEvent> expectedAudioNeedData{m_clientStub};
        ExpectMessage<NeedMediaDataEvent> expectedVideoNeedData{m_clientStub};
        expectedAudioNeedData.setFilter([&](const NeedMediaDataEvent &event)
                                        { return event.source_id() == m_audioSourceId; });
        expectedVideoNeedData.setFilter([&](const NeedMediaDataEvent &event)
                                        { return event.source_id() == m_videoSourceId; });

        // During SetPosition procedure, two PlaybackStateChange events are received. Add expects for them.
        ExpectMessage<PlaybackStateChangeEvent> expectedSeekingPlaybackStateChange{m_clientStub};
        ExpectMessage<PlaybackStateChangeEvent> expectedSeekDonePlaybackStateChange{m_clientStub};
        expectedSeekingPlaybackStateChange.setFilter(
            [&](const PlaybackStateChangeEvent &event)
            { return event.state() == PlaybackStateChangeEvent_PlaybackState_SEEKING; });
        expectedSeekDonePlaybackStateChange.setFilter(
            [&](const PlaybackStateChangeEvent &event)
            { return event.state() == PlaybackStateChangeEvent_PlaybackState_SEEK_DONE; });

        // Send SetPositionRequest and expect success
        auto request{createSetPositionRequest(m_sessionId, position)};
        ConfigureAction<SetPosition>(m_clientStub).send(request).expectSuccess();

        // Check received PlaybackStateChange events
        auto receivedSeekingPlaybackStateChange{expectedSeekingPlaybackStateChange.getMessage()};
        ASSERT_TRUE(receivedSeekingPlaybackStateChange);
        EXPECT_EQ(receivedSeekingPlaybackStateChange->session_id(), m_sessionId);

        auto receivedSeekDonePlaybackStateChange{expectedSeekDonePlaybackStateChange.getMessage()};
        ASSERT_TRUE(receivedSeekDonePlaybackStateChange);
        EXPECT_EQ(receivedSeekDonePlaybackStateChange->session_id(), m_sessionId);

        // Check received NeedDataReqs
        auto receivedAudioNeedData{expectedAudioNeedData.getMessage()};
        ASSERT_TRUE(receivedAudioNeedData);
        EXPECT_EQ(receivedAudioNeedData->session_id(), m_sessionId);
        EXPECT_EQ(receivedAudioNeedData->source_id(), m_audioSourceId);
        m_lastAudioNeedData = receivedAudioNeedData;

        auto receivedVideoNeedData{expectedVideoNeedData.getMessage()};
        ASSERT_TRUE(receivedVideoNeedData);
        EXPECT_EQ(receivedVideoNeedData->session_id(), m_sessionId);
        EXPECT_EQ(receivedVideoNeedData->source_id(), m_videoSourceId);
        m_lastVideoNeedData = receivedVideoNeedData;
    }

    void willFailToSetPosition()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementSeek(&m_pipeline, kPlaybackRate, GST_FORMAT_TIME,
                                                      static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH), GST_SEEK_TYPE_SET,
                                                      kPositionInPaused, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
            .WillOnce(Return(FALSE));
    }

    void SetPositionFailure()
    {
        // During SetPosition procedure, two PlaybackStateChange events are received. Add expects for them.
        ExpectMessage<PlaybackStateChangeEvent> expectedSeekingPlaybackStateChange{m_clientStub};
        ExpectMessage<PlaybackStateChangeEvent> expectedFailurePlaybackStateChange{m_clientStub};
        expectedSeekingPlaybackStateChange.setFilter(
            [&](const PlaybackStateChangeEvent &event)
            { return event.state() == PlaybackStateChangeEvent_PlaybackState_SEEKING; });
        expectedFailurePlaybackStateChange.setFilter(
            [&](const PlaybackStateChangeEvent &event)
            { return event.state() == PlaybackStateChangeEvent_PlaybackState_FAILURE; });

        // Send SetPositionRequest and expect success
        auto request{createSetPositionRequest(m_sessionId, kPositionInPaused)};
        ConfigureAction<SetPosition>(m_clientStub).send(request).expectSuccess();

        // Check received PlaybackStateChange events
        auto receivedSeekingPlaybackStateChange{expectedSeekingPlaybackStateChange.getMessage()};
        ASSERT_TRUE(receivedSeekingPlaybackStateChange);
        EXPECT_EQ(receivedSeekingPlaybackStateChange->session_id(), m_sessionId);

        auto receivedSeekDonePlaybackStateChange{expectedFailurePlaybackStateChange.getMessage()};
        ASSERT_TRUE(receivedSeekDonePlaybackStateChange);
        EXPECT_EQ(receivedSeekDonePlaybackStateChange->session_id(), m_sessionId);
    }
};

/*
 * Component Test: Set position success states
 * Test Objective:
 *  Test that seek is successfully handled.
 *
 * Sequence Diagrams:
 *  Seek - https://wiki.rdkcentral.com/display/ASP/Rialto+Flush+and+Seek+Design
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
 *
 *  Step 8: SetPosition in paused state
 *   SetPosition to position 10s.
 *   Server should notify the client that the Playback state has changed to SEEKING.
 *   Expect that SetPositionResponse has success status
 *   Server should notify the client that the Playback state has changed to SEEK_DONE.
 *
 *  Step 9: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is in playing state
 *   Expect that server notifies the client that the Playback state has changed to PLAYING.
 *
 *  Step 10: Set position in playing state
 *   SetPosition to position 0s.
 *   Server should notify the client that the Playback state has changed to SEEKING.
 *   Expect that SetPositionResponse has success status
 *   Server should notify the client that the Playback state has changed to SEEK_DONE.
 *
 *  Step 11: End of audio stream
 *   Send audio haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 12: End of video stream
 *   Send video haveData with one frame and EOS status
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
 *  Set position is handled and forwarded to the gstreamer.
 *
 * Code:
 */
TEST_F(SetPositionTest, SetPosition)
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

    // Step 5: Write 1 audio frame
    // Step 6: Write 1 video frame
    // Step 7: Notify buffered and Paused
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushAudioData(kFramesToPush);
        pushVideoData(kFramesToPush);

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);
    }
    willNotifyPaused();
    notifyPaused();

    // Step 8: SetPosition in paused state
    willSetPosition(kPositionInPaused);
    setPosition(kPositionInPaused);

    // Step 9: Play
    willPlay();
    play();

    // Step 10: Set position in playing state
    willSetPosition(kPositionInPlaying);
    setPosition(kPositionInPlaying);

    // Step 11: End of audio stream
    // Step 12: End of video stream
    willEos(&m_audioAppSrc);
    eosAudio(kFramesToPush);
    willEos(&m_videoAppSrc);
    eosVideo(kFramesToPush);

    // Step 13: Notify end of stream
    gstNotifyEos();

    // Step 14: Remove sources
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 15: Stop
    willStop();
    stop();

    // Step 16: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Set position failure states
 * Test Objective:
 *  Test that seek failure is successfully handled.
 *
 * Sequence Diagrams:
 *  Seek - https://wiki.rdkcentral.com/display/ASP/Rialto+Flush+and+Seek+Design
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
 *  Step 8: SetPosition Failure
 *   SetPosition to position 10s.
 *   Server should notify the client that the Playback state has changed to SEEKING.
 *   Expect that SetPositionResponse has success status
 *   Server should notify the client that the Playback state has changed to FAILED.
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
 *  Set position is handled and forwarded to the gstreamer.
 *
 * Code:
 */
TEST_F(SetPositionTest, SetPositionFailure)
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

    // Step 5: Write 1 audio frame
    // Step 6: Write 1 video frame
    // Step 7: Notify buffered and Paused
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushAudioData(kFramesToPush);
        pushVideoData(kFramesToPush);

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);
    }
    willNotifyPaused();
    notifyPaused();

    // Step 8: SetPosition Failure
    willFailToSetPosition();
    SetPositionFailure();

    // Step 9: Remove sources
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 10: Stop
    willStop();
    stop();

    // Step 11: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
