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
using testing::Invoke;
using testing::Return;
using testing::StrEq;

namespace
{
const std::string kElementTypeName{"GenericSink"};
constexpr bool kImmediateOutput{true};
constexpr bool kLowLatency{true};
constexpr bool kSync{true};
constexpr bool kSyncOff{true};
constexpr int32_t kStreamSyncMode{1};
} // namespace

namespace firebolt::rialto::server::ct
{
class PipelinePropertyTest : public MediaPipelineTest
{
public:
    PipelinePropertyTest()
    {
        GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
        m_element = gst_element_factory_create(elementFactory, nullptr);
        gst_object_unref(elementFactory);
    }

    ~PipelinePropertyTest() override { gst_object_unref(m_element); }

    void willSetImmediateOutput()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_element;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_element))).WillOnce(Return(kElementTypeName.c_str()));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("immediate-output"))).WillOnce(Return(&m_prop));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, _)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);
    }

    void setImmediateOutput()
    {
        auto req{createSetImmediateOutputRequest(m_sessionId, m_videoSourceId, true)};
        ConfigureAction<SetImmediateOutput>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
    }

    void willGetImmediateOutput()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_element;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_element))).WillOnce(Return(kElementTypeName.c_str()));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("immediate-output"))).WillOnce(Return(&m_prop));
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("immediate-output"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *val)
                {
                    gboolean *immediateOutputValue = reinterpret_cast<gboolean *>(val);
                    *immediateOutputValue = kImmediateOutput ? TRUE : FALSE;
                }));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);
    }

    void getImmediateOutput()
    {
        auto req{createGetImmediateOutputRequest(m_sessionId, m_videoSourceId)};
        ConfigureAction<GetImmediateOutput>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.immediate_output(), kImmediateOutput); });
    }

    void willSetLowLatency()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("audio-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_element;
                }));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("low-latency"))).WillOnce(Return(&m_prop));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, _)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);
    }

    void setLowLatency()
    {
        auto req{createSetLowLatencyRequest(m_sessionId, kLowLatency)};
        ConfigureAction<SetLowLatency>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
    }

    void willSetSync()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("audio-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_element;
                }));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("sync"))).WillOnce(Return(&m_prop));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, _)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);
    }

    void setSync()
    {
        auto req{createSetSyncRequest(m_sessionId, kSync)};
        ConfigureAction<SetSync>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
    }

    void willGetSync()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("audio-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_element;
                }));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("sync"))).WillOnce(Return(&m_prop));
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("sync"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *val)
                {
                    gboolean *syncValue = reinterpret_cast<gboolean *>(val);
                    *syncValue = kSync ? TRUE : FALSE;
                }));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);
    }

    void getSync()
    {
        auto req{createGetSyncRequest(m_sessionId)};
        ConfigureAction<GetSync>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.sync(), kSync); });
    }

    void willSetSyncOff()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinIterateElements(_)).WillOnce(Return(&m_it));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_OK));
        EXPECT_CALL(*m_glibWrapperMock, gValueGetObject(_)).WillOnce(Return(m_element));
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(_)).WillOnce(Return(m_factory));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(_, (GST_ELEMENT_FACTORY_TYPE_DECODER |
                                                                       GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO)))
            .WillOnce(Return(TRUE));
        EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(&m_it));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("sync-off"))).WillOnce(Return(&m_prop));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, _)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);
    }

    void setSyncOff()
    {
        auto req{createSetSyncOffRequest(m_sessionId, kSyncOff)};
        ConfigureAction<SetSyncOff>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
    }

    void willSetStreamSyncMode()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinIterateElements(_)).WillOnce(Return(&m_it));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_OK));
        EXPECT_CALL(*m_glibWrapperMock, gValueGetObject(_)).WillOnce(Return(m_element));
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(_)).WillOnce(Return(m_factory));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(_, (GST_ELEMENT_FACTORY_TYPE_DECODER |
                                                                       GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO)))
            .WillOnce(Return(TRUE));
        EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(&m_it));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("stream-sync-mode"))).WillOnce(Return(&m_prop));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, _)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);
    }

    void setStreamSyncMode()
    {
        auto req{createSetStreamSyncModeRequest(m_sessionId, kStreamSyncMode)};
        ConfigureAction<SetStreamSyncMode>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
    }

    void willGetStreamSyncMode()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinIterateElements(_)).WillOnce(Return(&m_it));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_OK));
        EXPECT_CALL(*m_glibWrapperMock, gValueGetObject(_)).WillOnce(Return(m_element));
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(_)).WillOnce(Return(m_factory));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(_, (GST_ELEMENT_FACTORY_TYPE_DECODER |
                                                                       GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO)))
            .WillOnce(Return(TRUE));
        EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(&m_it));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("stream-sync-mode"))).WillOnce(Return(&m_prop));
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("stream-sync-mode"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *val)
                {
                    gboolean *syncValue = reinterpret_cast<gboolean *>(val);
                    *syncValue = kSync ? TRUE : FALSE;
                }));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);
    }

    void getStreamSyncMode()
    {
        auto req{createGetStreamSyncModeRequest(m_sessionId)};
        ConfigureAction<GetStreamSyncMode>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.stream_sync_mode(), kStreamSyncMode); });
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
        // We expect success from this test because it's asynchronous (and the return
        // value doesn't reflect that the immediate-output flag wasn't set)
        ConfigureAction<SetImmediateOutput>(m_clientStub).send(req).expectSuccess();
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

    void willFailToSetLowLatency()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("audio-sink"), _)).Times(1);
    }

    void setLowLatencyFailure()
    {
        auto req{createSetLowLatencyRequest(m_sessionId, true)};
        ConfigureAction<SetLowLatency>(m_clientStub).send(req).expectSuccess();
    }

    void willFailToSetSync()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("audio-sink"), _)).Times(1);
    }

    void setSyncFailure()
    {
        auto req{createSetSyncRequest(m_sessionId, true)};
        ConfigureAction<SetSync>(m_clientStub).send(req).expectSuccess();
    }

    void willFailToGetSync()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("audio-sink"), _)).Times(1);
    }

    void getSyncFailure()
    {
        auto req{createGetSyncRequest(m_sessionId)};
        ConfigureAction<GetSync>(m_clientStub).send(req).expectFailure();
    }

    void willFailToSetSyncOff()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinIterateElements(_)).WillOnce(Return(&m_it));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_ERROR));
    }

    void setSyncOffFailure()
    {
        auto req{createSetSyncOffRequest(m_sessionId, kSyncOff)};
        ConfigureAction<SetSyncOff>(m_clientStub).send(req).expectSuccess();
    }

    void willFailToSetStreamSyncMode()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinIterateElements(_)).WillOnce(Return(&m_it));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_ERROR));
    }

    void setStreamSyncModeFailure()
    {
        auto req{createSetStreamSyncModeRequest(m_sessionId, kStreamSyncMode)};
        ConfigureAction<SetStreamSyncMode>(m_clientStub).send(req).expectSuccess();
    }

    void willFailToGetStreamSyncMode()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinIterateElements(_)).WillOnce(Return(&m_it));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_ERROR));
    }

    void getStreamSyncModeFailure()
    {
        auto req{createGetStreamSyncModeRequest(m_sessionId)};
        ConfigureAction<GetStreamSyncMode>(m_clientStub).send(req).expectFailure();
    }

private:
    GstElement *m_element{nullptr};
    GstStructure m_testStructure;
    GParamSpec m_prop{};
    GstIterator m_it{};
    char m_dummy{0};
    GstElementFactory *m_factory = reinterpret_cast<GstElementFactory *>(&m_dummy);
};

/*
 * Component Test: Get and set properties of the pipeline
 * Test Objective:
 *  Test the ability of the Rialto Server to set and get properties of the pipeline
 *
 * Sequence Diagrams:
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
 *   Rialto client sends SetImmediateOutput request and wait for response.
 *   the property should be set by the RialtoServer.
 *
 *  Step 5: Get Immediate output
 *   Will get the immediate-output property of the sink on the Rialto Server
 *
 *  Step 6: Set Low Latency
 *   Rialto client sends SetLowLatency request and wait for response.
 *   the property should be set by the RialtoServer.
 *
 *  Step 7: Set Sync
 *   Rialto client sends SetSync request and wait for response.
 *   the property should be set by the RialtoServer.
 *
 *  Step 8: Get Sync
 *   Will get the sync property of the sink on the Rialto Server
 *
 *  Step 9: Set Sync Off
 *   Rialto client sends SetSyncOff request and wait for response.
 *   the property should be set by the RialtoServer.
 *
 *  Step 10: Set Stream Sync Mode
 *   Rialto client sends SetStreamSyncMode request and wait for response.
 *   the property should be set by the RialtoServer.
 *
 *  Step 11: Get Stream Sync Mode
 *   Will get the StreamSyncMode property of the sink on the Rialto Server
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
 *  GetPosition is successful in >= PAUSED state
 *
 * Code:
 */
TEST_F(PipelinePropertyTest, pipelinePropertyGetAndSetSuccess)
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

    // Step 6: Set Low Latency
    willSetLowLatency();
    setLowLatency();

    // Step 7: Set Sync
    willSetSync();
    setSync();

    // Step 8: Get Sync
    willGetSync();
    getSync();

    // Step 9: Set Sync Off
    willSetSyncOff();
    setSyncOff();

    // Step 10: Set Stream Sync Mode
    willSetStreamSyncMode();
    setStreamSyncMode();

    // Step 11: Get Stream Sync Mode
    willGetStreamSyncMode();
    getStreamSyncMode();

    // Step 12: Remove sources
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 13: Stop
    willStop();
    stop();

    // Step 14: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Test falures to Get and set properties of the pipeline
 * Test Objective:
 *  Test the ability of the Rialto Server to handle failres while
 *  setting and getting properties of the pipeline
 *
 * Sequence Diagrams:
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
 *   SetImmediateOutputResponse is true because the server processes
 *   the request asyncronusly, but the RialtoServer fails gracefully to
 *   process the request (it doesn't crash)
 *
 *  Step 5: Fail to Get Immediate Output
 *   Rialto client sends GetImmediateOutput request and waits for response
 *   GetImmediateOutputResponse is false because the server couldn't process it
 *
 *  Step 6: Fail to Set Low Latency
 *   Rialto client sends SetLowLatency request and waits for response
 *   SetLowLatencyResponse is true because the server processes
 *   the request asyncronusly, but the RialtoServer fails gracefully to
 *   process the request (it doesn't crash)
 *
 *  Step 7: Fail to Set Sync
 *   Rialto client sends SetSync request and waits for response
 *   SetSyncResponse is true because the server processes
 *   the request asyncronusly, but the RialtoServer fails gracefully to
 *   process the request (it doesn't crash)
 *
 *  Step 8: Fail to Get Sync
 *   Rialto client sends GetSync request and waits for response
 *   GetSyncResponse is false because the server couldn't process it
 *
 *  Step 9: Fail to Set SyncOff
 *   Rialto client sends SetSyncOff request and waits for response
 *   SetSyncOffResponse is true because the server processes
 *   the request asyncronusly, but the RialtoServer fails gracefully to
 *   process the request (it doesn't crash)
 *
 *  Step 10: Fail to Set Stream Sync Mode
 *   Rialto client sends SetStreamSyncMode request and waits for response
 *   SetStreamSyncModeResponse is true because the server processes
 *   the request asyncronusly, but the RialtoServer fails gracefully to
 *   process the request (it doesn't crash)
 *
 *  Step 11: Fail to Get Stream Sync Mode
 *   Rialto client sends GetStreamSyncMode request and waits for response
 *   GetStreamSyncModeResponse is false because the server couldn't process it
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
 *  Position update notification is received in playing state
 *
 * Code:
 */
TEST_F(PipelinePropertyTest, pipelinePropertyGetAndSetFailures)
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

    // Step 6: Fail to set Low Latency
    willFailToSetLowLatency();
    setLowLatencyFailure();

    // Step 7: Fail to set Sync
    willFailToSetSync();
    setSyncFailure();

    // Step 8: Fail to get Sync
    willFailToGetSync();
    getSyncFailure();

    // Step 9: Fail to set Sync Off
    willFailToSetSyncOff();
    setSyncOffFailure();

    // Step 10: Fail to set Stream Sync Mode
    willFailToSetStreamSyncMode();
    setStreamSyncModeFailure();

    // Step 11: Fail to get Stream Sync Mode
    willFailToGetStreamSyncMode();
    getStreamSyncModeFailure();

    // Step 12: Remove sources
    willRemoveAudioSource();
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
