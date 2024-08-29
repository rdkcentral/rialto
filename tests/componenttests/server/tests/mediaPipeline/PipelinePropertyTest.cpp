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
const std::string kElementTypeName{"GenericSink"};
constexpr bool kImmediateOutput{true};
} // namespace

namespace firebolt::rialto::server::ct
{
class PipelinePropertyTest : public MediaPipelineTest
{
public:
    PipelinePropertyTest()
    {
        GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
        m_videoSink = gst_element_factory_create(elementFactory, nullptr);
        gst_object_unref(elementFactory);
    }

    ~PipelinePropertyTest() override { gst_object_unref(m_videoSink); }

    void waitForPositionUpdate()
    {
        ExpectMessage<PositionChangeEvent> expectPositionUpdate{m_clientStub};
        expectPositionUpdate.setTimeout(std::chrono::milliseconds(300)); // Timeout is received every 250ms
        auto receivedPositionUpdate = expectPositionUpdate.getMessage();
        ASSERT_TRUE(receivedPositionUpdate);
        EXPECT_EQ(receivedPositionUpdate->session_id(), m_sessionId);
        EXPECT_EQ(receivedPositionUpdate->position(), kCurrentPosition);
    }

    void getPosition()
    {
        auto req{createGetPositionRequest(m_sessionId)};
        ConfigureAction<GetPosition>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.position(), kCurrentPosition); });
    }

    void getPositionFailure()
    {
        auto req{createGetPositionRequest(m_sessionId)};
        ConfigureAction<GetPosition>(m_clientStub).send(req).expectFailure();
    }

    void willSetImmediateOutput()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_videoSink;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_videoSink))).WillOnce(Return(kElementTypeName.c_str()));

        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, _)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_videoSink)).Times(1);
    }

    void setImmediateOutput()
    {
        auto req{createSetImmediateOutputRequest(m_sessionId, m_videoSourceId, true)};
        ConfigureAction<SetImmediateOutput>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
    }

    void willFailToSetImmediateOutput()
    {
        // Failure to get the video sync will cause setImmediateOutput() to fail
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _)).Times(1);
        // We expect no further calls to gst or glib as would be the case if
        // the video-sink had been returned. See "willSetImmediateOutput()"
    }

    void setImmediateOutputFailure()
    {
        auto req{createSetImmediateOutputRequest(m_sessionId, m_videoSourceId, true)};
        // We expect success from this test because it's asyncronous (and the return
        // value doesn't reflect that the immediate-output flag wasn't set)
        ConfigureAction<SetImmediateOutput>(m_clientStub).send(req).expectSuccess();
    }

    void willGetImmediateOutput()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_videoSink;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_videoSink))).WillOnce(Return(kElementTypeName.c_str()));

        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("immediate-output"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *val)
                {
                    gboolean *immediateOutputValue = reinterpret_cast<gboolean *>(val);
                    *immediateOutputValue = kImmediateOutput ? TRUE : FALSE;
                }));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_videoSink)).Times(1);
    }

    void getImmediateOutput()
    {
        auto req{createGetImmediateOutputRequest(m_sessionId, m_videoSourceId)};
        ConfigureAction<GetImmediateOutput>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.immediate_output(), kImmediateOutput); });
    }

    void willFailToGetImmediateOutput()
    {
        // Failure to get the video sync will cause getImmediateOutput() to fail
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _)).Times(1);
    }

    void getImmediateOutputFailure()
    {
        auto req{createGetImmediateOutputRequest(m_sessionId, m_videoSourceId)};
        ConfigureAction<GetImmediateOutput>(m_clientStub).send(req).expectFailure();
    }

    void willGetStats()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_videoSink;
                }));
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

    void willFailToGetStats()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_videoSink;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_videoSink))).WillOnce(Return(kElementTypeName.c_str()));

        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("stats"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstStructure **elementPtr = reinterpret_cast<GstStructure **>(element);
                    *elementPtr = &m_testStructure;
                }));

        // Emulate a situation that the stats structure doesn't contain the number
        // of rendered frames...
        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&m_testStructure, StrEq("rendered"), _)).WillOnce(Return(false));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_videoSink)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstStructureFree(&m_testStructure)).Times(1);
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
 * Component Test: Get position test
 * Test Objective:
 *  Test the GetPosition method in rialto
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
 *  Step 4: Set Immediate Output
 *   Rialto should send SetImmediateOutput request and wait for response
 *
 *  Step 5: Get Immediate output
 *   Will get the immediate output property of the sink on the Rialto Server
 *
 *  Step 6: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 7: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 8: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  GetPosition is successful in >= PAUSED state
 *
 * Code:
 */
TEST_F(PipelinePropertyTest, GetPositionSuccess)
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

    // Step 4: Set Immediate Output
    willSetImmediateOutput();
    setImmediateOutput();

    // Step 5: Get Immediate output
    willGetImmediateOutput();
    getImmediateOutput();

    // Step 6: Remove sources
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 7: Stop
    willStop();
    stop();

    // Step 8: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Get position failure test
 * Test Objective:
 *  Test the GetPosition method in rialto. It should fail, when pipeline is below PAUSED state
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
 *  Step 4: Fail to Set Immediate Output
 *   Rialto client sends SetImmediateOutput request and waits for response
 *   SetImmediateOutputResponse is false because the server couldn't process it
 *
 *  Step 5: Fail to Get Immediate Output
 *   Rialto client sends GetImmediateOutput request and waits for response
 *   GetImmediateOutputResponse is false because the server couldn't process it
 *
 *  Step 6: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 7: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 8: Destroy media session
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
TEST_F(PipelinePropertyTest, getPositionFailure)
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

    // Step 4: Fail to set Immediate Output
    willFailToSetImmediateOutput();
    setImmediateOutputFailure();

    // Step 5: Fail to get Immediate Output
    willFailToGetImmediateOutput();
    getImmediateOutputFailure();

    // Step 6: Remove sources
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 7: Stop
    willStop();
    stop();

    // Step 8: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
