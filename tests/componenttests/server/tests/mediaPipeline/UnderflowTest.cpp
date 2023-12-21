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

#include "ExpectMessage.h"
#include "Matchers.h"
#include "MediaPipelineTest.h"
#include <gst/gst.h>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrEq;

namespace
{
constexpr unsigned kFramesToPush{1};
constexpr int kFrameCountInPausedState{3};
constexpr int kFrameCountInPlayingState{24};
const std::string kElementName{"Decoder"};
constexpr gulong kSignalId{123};
} // namespace

namespace firebolt::rialto::server::ct
{
class UnderflowTest : public MediaPipelineTest
{
public:
    UnderflowTest()
    {
        gst_init(nullptr, nullptr);
        m_elementFactory = gst_element_factory_find("fakesrc");
        m_audioDecoder = gst_element_factory_create(m_elementFactory, nullptr);
        m_videoDecoder = gst_element_factory_create(m_elementFactory, nullptr);
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(_)).WillRepeatedly(Return(m_elementFactory));
    }

    ~UnderflowTest() override
    {
        gst_object_unref(m_audioDecoder);
        gst_object_unref(m_videoDecoder);
        gst_object_unref(m_elementFactory);
    }

    void setupElementsCommon()
    {
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(_)).WillRepeatedly(Return(kElementName.c_str()));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_DECODER))
            .WillRepeatedly(Return(TRUE));
        EXPECT_CALL(*m_glibWrapperMock, gStrHasPrefix(_, StrEq("amlhalasink"))).WillRepeatedly(Return(FALSE));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstElementFactoryListIsType(m_elementFactory,
                                                GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
            .WillRepeatedly(Return(FALSE));
        EXPECT_CALL(*m_glibWrapperMock, gSignalListIds(_, _))
            .WillRepeatedly(Invoke(
                [&](GType itype, guint *n_ids)
                {
                    *n_ids = 1;
                    return m_signals;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gSignalQuery(m_signals[0], _))
            .WillRepeatedly(Invoke([&](guint signal_id, GSignalQuery *query)
                                   { query->signal_name = "buffer-underflow-callback"; }));
        EXPECT_CALL(*m_glibWrapperMock, gFree(m_signals)).Times(2);
    }

    void willSetupAudioDecoder()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(m_audioDecoder)).WillOnce(Return(m_audioDecoder));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
            .WillOnce(Return(FALSE))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock,
                    gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO))
            .WillOnce(Return(TRUE))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_glibWrapperMock, gObjectType(m_audioDecoder)).WillRepeatedly(Return(G_TYPE_PARAM));
        EXPECT_CALL(*m_glibWrapperMock, gSignalConnect(_, _, _, _))
            .WillOnce(Invoke(
                [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
                {
                    m_audioUnderflowCallback = c_handler;
                    m_audioUnderflowData = data;
                    return kSignalId;
                }))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_audioDecoder))
            .WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void willSetupVideoDecoder()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(m_videoDecoder)).WillOnce(Return(m_videoDecoder));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
            .WillOnce(Return(TRUE))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_glibWrapperMock, gObjectType(m_videoDecoder)).WillRepeatedly(Return(G_TYPE_PARAM));
        EXPECT_CALL(*m_glibWrapperMock, gSignalConnect(_, StrEq("buffer-underflow-callback"), _, _))
            .WillOnce(Invoke(
                [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
                {
                    m_videoUnderflowCallback = c_handler;
                    m_videoUnderflowData = data;
                    return kSignalId;
                }))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_videoDecoder))
            .WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void setupAudioDecoder() 
    { 
        m_gstreamerStub.setupElement(m_audioDecoder); 
        waitWorker();
    }

    void setupVideoDecoder() 
    { 
        m_gstreamerStub.setupElement(m_videoDecoder);
        waitWorker();
    }

    void audioUnderflow()
    {
        ExpectMessage<BufferUnderflowEvent> expectedBufferUnderflow{m_clientStub};
        expectedBufferUnderflow.setFilter([&](const auto &msg) { return msg.source_id() == m_audioSourceId; });

        ASSERT_TRUE(m_audioUnderflowCallback);
        ASSERT_TRUE(m_audioUnderflowData);
        ((void (*)(GstElement *, guint, gpointer, gpointer))m_audioUnderflowCallback)(m_audioDecoder, 0, nullptr,
                                                                                      m_audioUnderflowData);

        auto receivedBufferUnderflow{expectedBufferUnderflow.getMessage()};
        ASSERT_TRUE(receivedBufferUnderflow);
        EXPECT_EQ(receivedBufferUnderflow->session_id(), m_sessionId);
        EXPECT_EQ(receivedBufferUnderflow->source_id(), m_audioSourceId);
    }

    void videoUnderflow()
    {
        ExpectMessage<BufferUnderflowEvent> expectedBufferUnderflow{m_clientStub};
        expectedBufferUnderflow.setFilter([&](const auto &msg) { return msg.source_id() == m_videoSourceId; });

        ASSERT_TRUE(m_videoUnderflowCallback);
        ASSERT_TRUE(m_videoUnderflowData);
        ((void (*)(GstElement *, guint, gpointer, gpointer))m_videoUnderflowCallback)(m_audioDecoder, 0, nullptr,
                                                                                      m_videoUnderflowData);

        auto receivedBufferUnderflow{expectedBufferUnderflow.getMessage()};
        ASSERT_TRUE(receivedBufferUnderflow);
        EXPECT_EQ(receivedBufferUnderflow->session_id(), m_sessionId);
        EXPECT_EQ(receivedBufferUnderflow->source_id(), m_videoSourceId);
    }

private:
    GstElementFactory *m_elementFactory{nullptr};
    GstElement *m_audioDecoder{nullptr};
    GstElement *m_videoDecoder{nullptr};
    guint m_signals[1]{123};
    GCallback m_audioUnderflowCallback;
    gpointer m_audioUnderflowData{nullptr};
    GCallback m_videoUnderflowCallback;
    gpointer m_videoUnderflowData{nullptr};
};
/*
 * Component Test:Underflow test
 * Test Objective:
 *  Test if Rialto Server handles gstreamer underflow signals correctly. Underflow should be forwarded to Rialto Client
 *  with BufferUnderflowEvent message
 *
 * Sequence Diagrams:
 *  Underflow
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
 *  Step 3: Setup Audio Decoder
 *   Call SetupElement callback with Audio Decoder
 *   Audio Underflow callback should be registered
 *
 *  Step 4: Setup Video Decoder
 *   Call SetupElement callback with Video Decoder
 *   Video Underflow callback should be registered
 *
 *  Step 5: Attach all sources
 *   Attach the audio source.
 *   Expect that audio source is attached.
 *   Attach the video source.
 *   Expect that video source is attached.
 *   Expect that rialto source is setup
 *   Expect that all sources are attached.
 *   Expect that the Playback state has changed to IDLE.
 *
 *  Step 6: Pause
 *   Pause the content.
 *   Expect that gstreamer pipeline is paused.
 *   Expect that server notifies the client that the Network state has changed to PAUSED.
 *
 *  Step 7: Write 1 audio frame
 *   Gstreamer Stub notifies, that it needs audio data
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *   Write 1 frame of audio data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *
 *  Step 8: Write 1 video frame
 *   Gstreamer Stub notifies, that it needs video data
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *   Write 1 frame of video data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *
 *  Step 9: Notify buffered
 *   Expect that server notifies the client that the Network state has changed to BUFFERED.
 *
 *  Step 10: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is in playing state
 *   Expect that server notifies the client that the Playback state has changed to PLAYING.
 *
 *  Step 11: Audio Underflow
 *   Rialto Server will receive Audio Underflow signal
 *   Rialto Server should send BufferUnderflowEvent with audio source
 *
 *  Step 12: Video Underflow
 *   Rialto Server will receive Video Underflow signal
 *   Rialto Server should send BufferUnderflowEvent with video source
 *
 *  Step 13: End of audio stream
 *   Send audio haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 14: End of video stream
 *   Send video haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 15: Notify end of stream
 *   Simulate, that gst_message_eos is received by Rialto Server
 *   Expect that server notifies the client that the Network state has changed to END_OF_STREAM.
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
 *  Underflow signals are handled by Rialto Server
 *
 * Code:
 */
TEST_F(UnderflowTest, underflow)
{
    // Step 1: Create a new media session
    createSession();

    // Step 2: Load content
    gstPlayerWillBeCreated();
    load();

    // Step 3: Setup Audio Decoder
    setupElementsCommon();
    willSetupAudioDecoder();
    setupAudioDecoder();

    // Step 4: Setup Video Decoder
    willSetupVideoDecoder();
    setupVideoDecoder();

    // Step 5: Attach all sources
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

    // Step 6: Pause
    willPause();
    pause();

    // Step 7: Write 1 audio frame
    // Step 8: Write 1 video frame
    // Step 9: Notify buffered
    gstNeedData(&m_audioAppSrc, kFrameCountInPausedState);
    gstNeedData(&m_videoAppSrc, kFrameCountInPausedState);
    {
        std::cout << "NetworkStateChangeEvent 32" << std::endl;
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushAudioData(kFramesToPush, kFrameCountInPausedState);
        pushVideoData(kFramesToPush, kFrameCountInPausedState);

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);
    }

    // Step 10: Play
    willPlay();
    play();

    // Step 11: Audio Underflow
    audioUnderflow();

    // Step 12: Video Underflow
    videoUnderflow();

    // Step 13: End of audio stream
    // Step 14: End of video stream
    willEos(&m_audioAppSrc);
    eosAudio(kFramesToPush);
    willEos(&m_videoAppSrc);
    eosVideo(kFramesToPush);

    // Step 15: Notify end of stream
    gstNotifyEos();
    willRemoveAudioSource();

    // Step 16: Remove sources
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
