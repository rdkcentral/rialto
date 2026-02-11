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

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrEq;

namespace
{
constexpr int kFramesToPush{3};
constexpr bool kResetTime{false};
constexpr bool kAsync{true};
} // namespace

namespace firebolt::rialto::server::ct
{
class SwitchAudioPlaybackTest : public MediaPipelineTest
{
public:
    SwitchAudioPlaybackTest() = default;
    ~SwitchAudioPlaybackTest() = default;

    void willFlushAudioSource()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstEventNewFlushStart()).WillOnce(Return(&m_flushStartEvent));
        EXPECT_CALL(*m_gstWrapperMock, gstElementSendEvent(GST_ELEMENT(&m_audioAppSrc), &m_flushStartEvent))
            .WillOnce(Return(true));
        EXPECT_CALL(*m_gstWrapperMock, gstEventNewFlushStop(kResetTime)).WillOnce(Return(&m_flushStopEvent));
        EXPECT_CALL(*m_gstWrapperMock, gstElementSendEvent(GST_ELEMENT(&m_audioAppSrc), &m_flushStopEvent))
            .WillOnce(Return(true));
    }

    void flushAudioSource()
    {
        // After successful Flush procedure, SourceFlushedEvent is sent.
        ExpectMessage<SourceFlushedEvent> expectedSourceFlushed{m_clientStub};
        expectedSourceFlushed.setFilter([&](const SourceFlushedEvent &event)
                                        { return event.source_id() == m_audioSourceId; });

        // After successful Flush, NeedData for source is sent.
        ExpectMessage<NeedMediaDataEvent> expectedAudioNeedData{m_clientStub};
        expectedAudioNeedData.setFilter([&](const NeedMediaDataEvent &event)
                                        { return event.source_id() == m_audioSourceId; });

        // Send FlushRequest and expect success
        auto request{createFlushRequest(m_sessionId, m_audioSourceId, kResetTime)};
        ConfigureAction<Flush>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const FlushResponse &response) { EXPECT_EQ(kAsync, response.async()); });

        // Check received SourceFlushedEvent events
        auto receivedSourceFlushed{expectedSourceFlushed.getMessage()};
        ASSERT_TRUE(receivedSourceFlushed);
        EXPECT_EQ(receivedSourceFlushed->session_id(), m_sessionId);
        EXPECT_EQ(receivedSourceFlushed->source_id(), m_audioSourceId);

        // Check received NeedDataReqs
        auto receivedAudioNeedData{expectedAudioNeedData.getMessage()};
        ASSERT_TRUE(receivedAudioNeedData);
        EXPECT_EQ(receivedAudioNeedData->session_id(), m_sessionId);
        EXPECT_EQ(receivedAudioNeedData->source_id(), m_audioSourceId);
        m_lastAudioNeedData = receivedAudioNeedData;
    }

    void willSwitchAudioSource()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(StrEq("audio/mpeg"))).WillOnce(Return(&m_newCaps));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleStringStub(&m_newCaps, StrEq("alignment"), G_TYPE_STRING, StrEq("nal")));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleStringStub(&m_newCaps, StrEq("stream-format"), G_TYPE_STRING, StrEq("raw")));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_newCaps, StrEq("mpegversion"), G_TYPE_INT, 4));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleIntStub(&m_newCaps, StrEq("channels"), G_TYPE_INT, kNumOfChannels));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_newCaps, StrEq("rate"), G_TYPE_INT, kSampleRate));
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(&m_audioAppSrc)).WillOnce(Return(&m_audioCaps));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsIsEqual(&m_newCaps, &m_audioCaps)).WillOnce(Return(true));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_newCaps));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_audioCaps)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void switchAudioSource()
    {
        auto attachAudioSourceReq{createAttachAudioSourceRequest(m_sessionId)};
        attachAudioSourceReq.set_switch_source(true);
        ConfigureAction<AttachSource>(m_clientStub).send(attachAudioSourceReq).expectSuccess();
        waitWorker();
    }

private:
    GstCaps m_newCaps{};
};

/*
 * Component Test: Playback content when audio source has been switched.
 * Test Objective:
 *  Test that audio source can be switched mid playback and that video playback is unaffected.
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
 *  Step 8: Flush Audio Source
 *   Flush the audio source.
 *   Expect that audio source is flushed.
 *
 *  Step 9: Switch Audio Source
 *   Switch the audio source.
 *   Expect that audio source is switched.
 *
 *  Step 10: Write video and audio frames
 *   Write video frames.
 *   Write audio frames.
 *
 *  Step 11: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is playing.
 *   Expect that server notifies the client that the Network state has changed to PLAYING.
 *
 *  Step 12: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
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
 *  Video playback is unaffected when audio source is removed mid playback.
 *  Audio can be reattached and playback of both video and audio resumes.
 *
 * Code:
 */
TEST_F(SwitchAudioPlaybackTest, SwitchAudio)
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

    // Step 5: Write video and audio frames
    gstNeedData(&m_audioAppSrc, kFramesToPush);
    gstNeedData(&m_videoAppSrc, kFramesToPush);
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushAudioData(kFramesToPush, kFramesToPush);
        pushVideoData(kFramesToPush, kFramesToPush);

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
    GST_STATE(&m_pipeline) = GST_STATE_PAUSED;

    // Step 8: Flush Audio Source
    willFlushAudioSource();
    flushAudioSource();

<<<<<<< HEAD:tests/componenttests/server/tests/mediaPipeline/RemoveAudioPlaybackTest.cpp
    // Step 9: Write video frames
    pushVideoData(kFramesToPush, kFramesToPush);

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
    pushAudioData(kFramesToPush, kFramesToPush);
    pushVideoData(kFramesToPush, kFramesToPush);
=======
    // Step 9: Switch Audio Source
    willSwitchAudioSource();
    switchAudioSource();

    // Step 10: Write video and audio frames
    pushAudioData(kFramesToPush);
    pushVideoData(kFramesToPush);
>>>>>>> 4b4efb39 (ct fix):tests/componenttests/server/tests/mediaPipeline/SwitchAudioPlaybackTest.cpp

    // Step 11: Play
    willPlay();
    play();

    // Step 12: Remove sources
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 13: Stop
    willStop();
    stop();

    // Step 14: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
