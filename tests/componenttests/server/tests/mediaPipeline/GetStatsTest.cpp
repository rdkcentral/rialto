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
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgumentPointee;
using testing::StrEq;

namespace
{
constexpr unsigned kFramesToPush{1};
constexpr int kFrameCountInPausedState{3};
constexpr int kFrameCountInPlayingState{24};
} // namespace

namespace firebolt::rialto::server::ct
{
class GetStatsTest : public MediaPipelineTest
{
public:
    GetStatsTest()
    {
        GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
        m_videoSink = gst_element_factory_create(elementFactory, nullptr);
        gst_object_unref(elementFactory);
    }

    ~GetStatsTest() override { gst_object_unref(m_videoSink); }

    void willGetStats()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_videoSink;
                }));
        const std::string kElementTypeName{"GenericSink"};
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_videoSink))).WillOnce(Return(kElementTypeName.c_str()));

        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("stats"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstStructure **elementPtr = reinterpret_cast<GstStructure **>(element);
                    *elementPtr = &m_testStructure;
                }));

        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&m_testStructure, StrEq("rendered"), _))
            .WillOnce(DoAll(SetArgumentPointee<2>(kRenderedFrames), Return(true)));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&m_testStructure, StrEq("dropped"), _))
            .WillOnce(DoAll(SetArgumentPointee<2>(kDroppedFrames), Return(true)));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_videoSink)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstStructureFree(&m_testStructure)).Times(1);
    }

    void willFailToGetStats()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_videoSink;
                }));
        const std::string kElementTypeName{"GenericSink"};
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_videoSink))).WillOnce(Return(kElementTypeName.c_str()));

        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("stats"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstStructure **elementPtr = reinterpret_cast<GstStructure **>(element);
                    *elementPtr = &m_testStructure;
                }));

        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&m_testStructure, StrEq("rendered"), _)).WillOnce(Return(false));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_videoSink)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstStructureFree(&m_testStructure)).Times(1);
    }

    void getStats()
    {
        auto req{createGetStatsRequest(m_sessionId, m_videoSourceId)};
        ConfigureAction<GetStats>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse(
                [&](const auto &resp)
                {
                    EXPECT_EQ(resp.rendered_frames(), kRenderedFrames);
                    EXPECT_EQ(resp.dropped_frames(), kDroppedFrames);
                });
    }

    void getStatsFailure()
    {
        auto req{createGetStatsRequest(m_sessionId, m_videoSourceId)};
        ConfigureAction<GetStats>(m_clientStub).send(req).expectFailure();
    }

private:
    GstElement *m_videoSink{nullptr};
    GstStructure m_testStructure;
};
/*
 * Component Test: Position Reporting test
 * Test Objective:
 *  Test the periodical position reporting in rialto
 *
 * Sequence Diagrams:
 *  Position Reporting:
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
 *  Step 8: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is in playing state
 *   Expect that server notifies the client that the Playback state has changed to PLAYING.
 *
 *  Step 9: todo
 *
 *  Step 10: todo
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
 *  Position update notification is received in playing state
 *
 * Code:
 */
TEST_F(GetStatsTest, WillSuccessfullyGetStats)
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
    gstNeedData(&m_audioAppSrc, kFrameCountInPausedState);
    gstNeedData(&m_videoAppSrc, kFrameCountInPausedState);
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushAudioData(kFramesToPush, kFrameCountInPausedState);
        pushVideoData(kFramesToPush, kFrameCountInPausedState);

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

    // Step 9:
    willGetStats();
    getStats();

    // Step 10:
    willFailToGetStats();
    getStatsFailure();

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
} // namespace firebolt::rialto::server::ct
