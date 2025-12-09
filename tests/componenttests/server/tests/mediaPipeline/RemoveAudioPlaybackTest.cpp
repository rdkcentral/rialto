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

#include "Constants.h"
#include "ExpectMessage.h"
#include "Matchers.h"
#include "MediaPipelineTest.h"

using testing::_;
using testing::Return;
using testing::StrEq;

namespace
{
constexpr int kFramesToPush{3};
} // namespace

namespace firebolt::rialto::server::ct
{
class RemoveAudioPlaybackTest : public MediaPipelineTest
{
public:
    RemoveAudioPlaybackTest() = default;
    ~RemoveAudioPlaybackTest() = default;

    void willReattachAudioSource()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(StrEq("audio/mpeg"))).WillOnce(Return(&m_audioCaps));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleStringStub(&m_audioCaps, StrEq("alignment"), G_TYPE_STRING, StrEq("nal")));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleStringStub(&m_audioCaps, StrEq("stream-format"), G_TYPE_STRING, StrEq("raw")));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_audioCaps, StrEq("mpegversion"), G_TYPE_INT, 4));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleIntStub(&m_audioCaps, StrEq("channels"), G_TYPE_INT, kNumOfChannels));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_audioCaps, StrEq("rate"), G_TYPE_INT, kSampleRate));
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(&m_audioAppSrc)).WillOnce(Return(&m_oldCaps));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsIsEqual(&m_audioCaps, &m_oldCaps)).WillOnce(Return(TRUE));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_oldCaps));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_audioCaps)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));

        willSetAudioAndVideoFlags();
    }

    void reattachAudioSource()
    {
        ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};

        attachAudioSource();

        auto receivedNeedData{expectedNeedData.getMessage()};
        ASSERT_TRUE(receivedNeedData);
        EXPECT_EQ(receivedNeedData->session_id(), m_sessionId);
        EXPECT_EQ(receivedNeedData->source_id(), m_audioSourceId);
        EXPECT_EQ(receivedNeedData->frame_count(), 24);
        m_lastAudioNeedData = receivedNeedData;
    }

private:
    GstCaps m_oldCaps{};
    gchar m_oldCapsStr{};
};

/*
 * Component Test: Playback content when audio source has been removed and reattached.
 * Test Objective:
 *  Test that video only playback can continue if the audio source is removed, and that audio can be restarted
 *  when it is reattached.
 *
 * Sequence Diagrams:
 *  Rialto Dynamic Audio Stream Switching
 *   - https://wiki.rdkcentral.com/pages/viewpage.action?spaceKey=ASP&title=Rialto+Dynamic+Audio+Stream+Switching
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
 *  Step 5: Write video and audio frames
 *   Write video frames.
 *   Write audio frames.
 *   Expect that server notifies the client that the Network state has changed to BUFFERED.
 *   Expect that server notifies the client that the Network state has changed to PAUSED.
 *
 *  Step 6: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is playing.
 *   Expect that server notifies the client that the Network state has changed to PLAYING.
 *
 *  Step 7: Pause
 *   Pause the content.
 *   Expect that gstreamer pipeline is paused.
 *   Expect that server notifies the client that the Network state has changed to PAUSED.
 *
 *  Step 8: Remove Audio Source
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *
 *  Step 9: Write video frames
 *   Write video frames.
 *
 *  Step 10: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is playing.
 *   Expect that server notifies the client that the Network state has changed to PLAYING.
 *
 *  Step 11: Pause
 *   Pause the content.
 *   Expect that gstreamer pipeline is paused.
 *   Expect that server notifies the client that the Network state has changed to PAUSED.
 *
 *  Step 12: Reattach audio source
 *   Attach the audio source again.
 *   Expect that reattach procedure is triggered.
 *   Expect that audio source is attached.
 *
 *  Step 13: Write video and audio frames
 *   Write video frames.
 *   Write audio frames.
 *
 *  Step 14: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is playing.
 *   Expect that server notifies the client that the Network state has changed to PLAYING.
 *
 *  Step 15: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 16: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 17: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  Video playback is unaffected when audio source is removed mid playback.
 *  Audio can be reattached and playback of both video and audio resumes.
 *
 * Code:
 */
TEST_F(RemoveAudioPlaybackTest, RemoveAudio)
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

    // Step 5: Write video and audio frames
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

    // Step 6: Play
    willPlay();
    play();

    // Step 7: Pause
    willPause();
    pause();
    willNotifyPaused();
    notifyPaused();

    // Step 8: Remove Audio Source
    willRemoveAudioSource();
    removeSource(m_audioSourceId);

    // Step 9: Write video frames
    pushVideoData(kFramesToPush);

    // Step 10: Play
    willPlay();
    play();

    // Step 11: Pause
    willPause();
    pause();
    willNotifyPaused();
    notifyPaused();

    // Step 12: Reattach audio source
    willReattachAudioSource();
    reattachAudioSource();

    // Step 13: Write video and audio frames
    pushAudioData(kFramesToPush);
    pushVideoData(kFramesToPush);

    // Step 14: Play
    willPlay();
    play();

    // Step 15: Remove sources
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 16: Stop
    willStop();
    stop();

    // Step 17: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
