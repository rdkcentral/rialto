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
constexpr int32_t kBufferingLimit{4321};
constexpr bool kUseBuffering{true};
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

    template <typename T>
    void willSetSinkProperty(const std::string &sinkName, const std::string &propertyName, const T &value)
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq(sinkName.c_str()), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_element;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_element))).WillOnce(Return(kElementTypeName.c_str()));
        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq(propertyName.c_str()))).WillOnce(Return(&m_prop));

        if constexpr (std::is_same_v<T, bool>)
        {
            EXPECT_CALL(*m_glibWrapperMock, gObjectSetBoolStub(_, StrEq(propertyName.c_str()), value)).Times(1);
        }
        else
        {
            EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, StrEq(propertyName.c_str()))).Times(1);
        }
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    template <typename T>
    void willGetSinkProperty(const std::string &sinkName, const std::string &propertyName, const T &value)
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq(sinkName.c_str()), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_element;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_element))).WillOnce(Return(kElementTypeName.c_str()));
        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq(propertyName.c_str()))).WillOnce(Return(&m_prop));

        if constexpr (std::is_same_v<T, bool>)
        {
            EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(propertyName.c_str()), _))
                .WillOnce(Invoke(
                    [&](gpointer object, const gchar *first_property_name, void *val)
                    {
                        gboolean *returnVal = reinterpret_cast<gboolean *>(val);
                        *returnVal = value ? TRUE : FALSE;
                    }));
        }
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    template <typename T>
    void willSetDecoderProperty(GstElementFactoryListType type, const std::string &propertyName, const T &value)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinIterateRecurse(_)).WillOnce(Return(&m_it));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_OK));
        EXPECT_CALL(*m_glibWrapperMock, gValueGetObject(_)).WillOnce(Return(m_element));
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(_)).WillOnce(Return(m_factory));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(_, (GST_ELEMENT_FACTORY_TYPE_DECODER | type)))
            .WillOnce(Return(TRUE));
        EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(&m_it));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq(propertyName.c_str()))).WillOnce(Return(&m_prop));
        if constexpr (std::is_same_v<T, bool>)
        {
            EXPECT_CALL(*m_glibWrapperMock, gObjectSetBoolStub(_, StrEq(propertyName.c_str()), value)).Times(1);
        }
        else if constexpr (std::is_same_v<T, int32_t>)
        {
            EXPECT_CALL(*m_glibWrapperMock, gObjectSetIntStub(_, StrEq(propertyName.c_str()), value)).Times(1);
        }
        else
        {
            EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, StrEq(propertyName.c_str()))).Times(1);
        }
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    template <typename T>
    void willGetDecoderProperty(GstElementFactoryListType type, const std::string &propertyName, const T &value)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinIterateRecurse(_)).WillOnce(Return(&m_it));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_OK));
        EXPECT_CALL(*m_glibWrapperMock, gValueGetObject(_)).WillOnce(Return(m_element));
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(_)).WillOnce(Return(m_factory));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(_, (GST_ELEMENT_FACTORY_TYPE_DECODER | type)))
            .WillOnce(Return(TRUE));
        EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(&m_it));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq(propertyName.c_str()))).WillOnce(Return(&m_prop));

        if constexpr (std::is_same_v<T, bool>)
        {
            EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(propertyName.c_str()), _))
                .WillOnce(Invoke(
                    [&](gpointer object, const gchar *first_property_name, void *val)
                    {
                        gboolean *returnVal = reinterpret_cast<gboolean *>(val);
                        *returnVal = value ? TRUE : FALSE;
                    }));
        }
        else if constexpr (std::is_same_v<T, int32_t>)
        {
            EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(propertyName.c_str()), _))
                .WillOnce(Invoke(
                    [&](gpointer object, const gchar *first_property_name, void *val)
                    {
                        gint *returnVal = reinterpret_cast<gint *>(val);
                        *returnVal = value;
                    }));
        }
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void willFailToSetSinkProperty()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, _, _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_element;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_element))).WillOnce(Return(kElementTypeName.c_str()));
        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, _)).WillOnce(Return(nullptr));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void willFailToSetDecoderProperty()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinIterateRecurse(_)).WillOnce(Return(&m_it));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_OK));
        EXPECT_CALL(*m_glibWrapperMock, gValueGetObject(_)).WillOnce(Return(m_element));
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(_)).WillOnce(Return(m_factory));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(_, _)).WillOnce(Return(TRUE));
        EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(&m_it));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, _)).WillOnce(Return(nullptr));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void willFailToGetSink()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, _, _))
            .WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void willFailToGetDecoder()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinIterateRecurse(_)).WillOnce(Return(&m_it));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_ERROR));
        EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
        EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(&m_it)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void setImmediateOutput()
    {
        auto req{createSetImmediateOutputRequest(m_sessionId, m_videoSourceId, kImmediateOutput)};
        ConfigureAction<SetImmediateOutput>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
        waitWorker();
    }

    void getImmediateOutput()
    {
        auto req{createGetImmediateOutputRequest(m_sessionId, m_videoSourceId)};
        ConfigureAction<GetImmediateOutput>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.immediate_output(), kImmediateOutput); });
        waitWorker();
    }

    void setLowLatency()
    {
        auto req{createSetLowLatencyRequest(m_sessionId, kLowLatency)};
        ConfigureAction<SetLowLatency>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
        waitWorker();
    }

    void setSync()
    {
        auto req{createSetSyncRequest(m_sessionId, kSync)};
        ConfigureAction<SetSync>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
        waitWorker();
    }

    void getSync()
    {
        auto req{createGetSyncRequest(m_sessionId)};
        ConfigureAction<GetSync>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.sync(), kSync); });
        waitWorker();
    }

    void setSyncOff()
    {
        auto req{createSetSyncOffRequest(m_sessionId, kSyncOff)};
        ConfigureAction<SetSyncOff>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
        waitWorker();
    }

    void setStreamSyncMode()
    {
        auto req{createSetStreamSyncModeRequest(m_sessionId, m_audioSourceId, kStreamSyncMode)};
        ConfigureAction<SetStreamSyncMode>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
        waitWorker();
    }

    void getStreamSyncMode()
    {
        auto req{createGetStreamSyncModeRequest(m_sessionId)};
        ConfigureAction<GetStreamSyncMode>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.stream_sync_mode(), kStreamSyncMode); });
        waitWorker();
    }

    void setBufferingLimit()
    {
        auto req{createSetBufferingLimitRequest(m_sessionId, kBufferingLimit)};
        ConfigureAction<SetBufferingLimit>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
        waitWorker();
    }

    void getBufferingLimit()
    {
        auto req{createGetBufferingLimitRequest(m_sessionId)};
        ConfigureAction<GetBufferingLimit>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.limit_buffering_ms(), kBufferingLimit); });
        waitWorker();
    }

    void setUseBuffering()
    {
        auto req{createSetUseBufferingRequest(m_sessionId, kUseBuffering)};
        ConfigureAction<SetUseBuffering>(m_clientStub).send(req).expectSuccess().matchResponse([&](const auto &resp) {});
    }

    void getUseBuffering()
    {
        auto req{createGetUseBufferingRequest(m_sessionId)};
        ConfigureAction<GetUseBuffering>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.use_buffering(), kUseBuffering); });
    }

    void setImmediateOutputFailure()
    {
        auto req{createSetImmediateOutputRequest(m_sessionId, m_videoSourceId, true)};
        // We expect success from this test because it's asynchronous (and the return
        // value doesn't reflect that the immediate-output flag wasn't set)
        ConfigureAction<SetImmediateOutput>(m_clientStub).send(req).expectSuccess();
        waitWorker();
    }

    void getImmediateOutputFailure()
    {
        auto req{createGetImmediateOutputRequest(m_sessionId, m_videoSourceId)};
        ConfigureAction<GetImmediateOutput>(m_clientStub).send(req).expectFailure();
        waitWorker();
    }

    void setLowLatencyFailure()
    {
        auto req{createSetLowLatencyRequest(m_sessionId, true)};
        ConfigureAction<SetLowLatency>(m_clientStub).send(req).expectSuccess();
        waitWorker();
    }

    void setSyncFailure()
    {
        auto req{createSetSyncRequest(m_sessionId, true)};
        ConfigureAction<SetSync>(m_clientStub).send(req).expectSuccess();
    }

    void getSyncFailure()
    {
        auto req{createGetSyncRequest(m_sessionId)};
        ConfigureAction<GetSync>(m_clientStub).send(req).expectFailure();
        waitWorker();
    }

    void setSyncOffFailure()
    {
        auto req{createSetSyncOffRequest(m_sessionId, kSyncOff)};
        ConfigureAction<SetSyncOff>(m_clientStub).send(req).expectSuccess();
        waitWorker();
    }

    void setStreamSyncModeFailure()
    {
        auto req{createSetStreamSyncModeRequest(m_sessionId, m_videoSourceId, kStreamSyncMode)};
        ConfigureAction<SetStreamSyncMode>(m_clientStub).send(req).expectSuccess();
        waitWorker();
    }

    void getStreamSyncModeFailure()
    {
        auto req{createGetStreamSyncModeRequest(m_sessionId)};
        ConfigureAction<GetStreamSyncMode>(m_clientStub).send(req).expectFailure();
        waitWorker();
    }

    void setBufferingLimitFailure()
    {
        auto req{createSetBufferingLimitRequest(m_sessionId, kBufferingLimit)};
        ConfigureAction<SetBufferingLimit>(m_clientStub).send(req).expectSuccess();
        waitWorker();
    }

    void getBufferingLimitFailure()
    {
        auto req{createGetBufferingLimitRequest(m_sessionId)};
        ConfigureAction<GetBufferingLimit>(m_clientStub).send(req).expectFailure();
        waitWorker();
    }

    void setUseBufferingFailure()
    {
        auto req{createSetUseBufferingRequest(m_sessionId + 1, kUseBuffering)};
        ConfigureAction<SetUseBuffering>(m_clientStub).send(req).expectFailure();
    }

    void getUseBufferingFailure()
    {
        auto req{createGetUseBufferingRequest(m_sessionId)};
        ConfigureAction<GetUseBuffering>(m_clientStub).send(req).expectFailure();
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
 *  Immediate-output property
 *  - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-ImmediateOutput
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
 *   Will get the StreamSyncMode property of the decoder on the Rialto Server
 *
 *  Step 12: Set Buffering Limit
 *   Rialto client sends SetBufferingLimit request and wait for response.
 *   the property should be set by the RialtoServer.
 *
 *  Step 13: Get Buffering Limit
 *   Will get the BufferingLimit property of the decoder on the Rialto Server
 *
 *  Step 14: Set Use Buffering
 *   Rialto client sends SetUseBuffering request and wait for response.
 *   the property should be cached by the RialtoServer.
 *
 *  Step 15: Get Use Buffering
 *   Will get the UseBuffering property of the decodebin on the Rialto Server
 *
 *  Step 16: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 17: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 18: Destroy media session
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
    willSetSinkProperty("video-sink", "immediate-output", kImmediateOutput);
    setImmediateOutput();

    // Step 5: Get Immediate output
    willGetSinkProperty("video-sink", "immediate-output", kImmediateOutput);
    getImmediateOutput();

    // Step 6: Set Low Latency
    willSetSinkProperty("audio-sink", "low-latency", kLowLatency);
    setLowLatency();

    // Step 7: Set Sync
    willSetSinkProperty("audio-sink", "sync", kSync);
    setSync();

    // Step 8: Get Sync
    willGetSinkProperty("audio-sink", "sync", kSync);
    getSync();

    // Step 9: Set Sync Off
    willSetDecoderProperty(GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO, "sync-off", kSyncOff);
    setSyncOff();

    // Step 10: Set Stream Sync Mode
    willSetDecoderProperty(GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO, "stream-sync-mode", kStreamSyncMode);
    setStreamSyncMode();

    // Step 11: Get Stream Sync Mode
    willGetDecoderProperty(GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO, "stream-sync-mode", kStreamSyncMode);
    getStreamSyncMode();

    // Step 12: Set Buffering Limit
    willSetDecoderProperty(GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO, "limit-buffering-ms", kBufferingLimit);
    setBufferingLimit();

    // Step 13: Get Buffering Limit
    willGetDecoderProperty(GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO, "limit-buffering-ms", kBufferingLimit);
    getBufferingLimit();

    // Step 14: Set Use Buffering
    setUseBuffering();

    // Step 15: Get Use Buffering
    getUseBuffering();

    // Step 16: Remove sources
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 17: Stop
    willStop();
    stop();

    // Step 18: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Test failures to Get and set properties of the pipeline
 * Test Objective:
 *  Test the ability of the Rialto Server to handle failres while
 *  setting and getting properties of the pipeline
 *
 * Sequence Diagrams:
 *  Immediate-output property
 *  - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-ImmediateOutput
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
 *  Step 12: Fail to Set Buffering Limit
 *   Rialto client sends BufferingLimitRequest and waits for response
 *   BufferingLimitResponse is true because the server processes
 *   the request asyncronusly, but the RialtoServer fails gracefully to
 *   process the request (it doesn't crash)
 *
 *  Step 13: Fail to Get Buffering Limit
 *   Rialto client sends GetBufferingLimitRequest and waits for response
 *   GetBufferingLimitResponse is false because the server couldn't process it
 *
 *  Step 14: Fail to Get Use Buffering
 *   Rialto client sends GetUseBufferingRequest and waits for response
 *   GetUseBufferingResponse is false because the server couldn't process it
 *
 *  Step 15: Fail to Set Use Buffering
 *   Rialto client sends UseBufferingRequest and waits for response
 *   UseBufferingResponse is false because the sessionId is wrong
 *
 *  Step 16: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 17: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 18: Destroy media session
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
    willFailToSetSinkProperty();
    setImmediateOutputFailure();

    // Step 5: Fail to get Immediate Output
    willFailToGetSink();
    getImmediateOutputFailure();

    // Step 6: Fail to set Low Latency
    willFailToSetSinkProperty();
    setLowLatencyFailure();

    // Step 7: Fail to set Sync
    willFailToSetSinkProperty();
    setSyncFailure();

    // Step 8: Fail to get Sync
    willFailToGetSink();
    getSyncFailure();

    // Step 9: Fail to set Sync Off
    willFailToSetDecoderProperty();
    setSyncOffFailure();

    // Step 10: Fail to set Stream Sync Mode
    willFailToSetDecoderProperty();
    setStreamSyncModeFailure();

    // Step 11: Fail to get Stream Sync Mode
    willFailToGetDecoder();
    getStreamSyncModeFailure();

    // Step 12: Fail to Set Buffering Limit
    willFailToSetDecoderProperty();
    setBufferingLimitFailure();

    // Step 13: Fail to Get Buffering Limit
    willFailToGetDecoder();
    getBufferingLimitFailure();

    // Step 14: Fail to Get Use Buffering
    getUseBufferingFailure();

    // Step 15: Fail to Set Use Buffering
    setUseBufferingFailure();

    // Step 16: Remove sources
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 17: Stop
    willStop();
    stop();

    // Step 18: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
