/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include <functional>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrEq;

namespace
{
constexpr unsigned kFramesToPush{1};
const std::string kElementName{"Decoder"};
constexpr gulong kSignalId{123};
} // namespace

namespace firebolt::rialto::server::ct
{
class FirstFrameNotificationTest : public MediaPipelineTest
{
public:
    FirstFrameNotificationTest()
    {
        m_elementFactory = gst_element_factory_find("fakesrc");
        m_videoDecoder = gst_element_factory_create(m_elementFactory, nullptr);
        m_audioDecoder = gst_element_factory_create(m_elementFactory, nullptr);
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(_)).WillRepeatedly(Return(m_elementFactory));
    }

    ~FirstFrameNotificationTest() override
    {
        gst_object_unref(m_audioDecoder);
        gst_object_unref(m_videoDecoder);
        gst_object_unref(m_elementFactory);
    }

    void setupElementsCommon(const char *signalName)
    {
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(_)).WillRepeatedly(Return(kElementName.c_str()));
        EXPECT_CALL(*m_glibWrapperMock, gStrHasPrefix(_, StrEq("amlhalasink"))).WillRepeatedly(Return(FALSE));
        EXPECT_CALL(*m_glibWrapperMock, gStrHasPrefix(_, StrEq("brcmaudiosink"))).WillRepeatedly(Return(FALSE));
        EXPECT_CALL(*m_glibWrapperMock, gStrHasPrefix(_, StrEq("rialtotexttracksink"))).WillRepeatedly(Return(FALSE));
        EXPECT_CALL(*m_gstWrapperMock, gstIsBaseParse(_)).WillRepeatedly(Return(FALSE));
        EXPECT_CALL(*m_glibWrapperMock, gSignalListIds(_, _))
            .WillRepeatedly(Invoke(
                [&](GType itype, guint *n_ids)
                {
                    *n_ids = 1;
                    return m_signals;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gSignalQuery(m_signals[0], _))
            .WillRepeatedly(Invoke([&](guint signal_id, GSignalQuery *query) { query->signal_name = signalName; }));
        EXPECT_CALL(*m_glibWrapperMock, gFree(m_signals)).Times(2);
    }

    void willSetupVideoDecoder()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(m_videoDecoder)).WillOnce(Return(m_videoDecoder));

        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(m_elementFactory, _))
            .WillRepeatedly(Invoke(
                [](GstElementFactory *, GstElementFactoryListType type)
                {
                    if (type == GST_ELEMENT_FACTORY_TYPE_DECODER)
                        return true;
                    if (type == GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO)
                        return true;
                    return false;
                }));

        EXPECT_CALL(*m_glibWrapperMock, gObjectType(m_videoDecoder)).WillRepeatedly(Return(G_TYPE_PARAM));
        EXPECT_CALL(*m_glibWrapperMock, gSignalConnect(_, _, _, _))
            .WillRepeatedly(Invoke(
                [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
                {
                    if (std::strcmp(detailed_signal, "first-video-frame-callback") == 0)
                    {
                        m_firstVideoFrameCallback = c_handler;
                        m_firstVideoFrameData = data;
                    }
                    return kSignalId;
                }));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_videoDecoder))
            .WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void setupVideoDecoder()
    {
        m_gstreamerStub.setupElement(m_videoDecoder);
        waitWorker();
    }

    void firstVideoFrameReceived()
    {
        if (!m_firstVideoFrameCallback || !m_firstVideoFrameData)
        {
            return;
        }

        ExpectMessage<FirstFrameReceivedEvent> expectedFirstFrameReceived{m_clientStub};
        expectedFirstFrameReceived.setFilter([&](const auto &msg) { return msg.source_id() == m_videoSourceId; });
        reinterpret_cast<void (*)(GstElement *, guint, gpointer, gpointer)>(
            m_firstVideoFrameCallback)(m_videoDecoder, 0, nullptr, m_firstVideoFrameData);

        auto receivedFirstFrameReceived{expectedFirstFrameReceived.getMessage()};
        ASSERT_TRUE(receivedFirstFrameReceived);
        EXPECT_EQ(receivedFirstFrameReceived->session_id(), m_sessionId);
        EXPECT_EQ(receivedFirstFrameReceived->source_id(), m_videoSourceId);
    }

    void willSetupAudioDecoder()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(m_audioDecoder)).WillOnce(Return(m_audioDecoder));

        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(m_elementFactory, _))
            .WillRepeatedly(Invoke(
                [](GstElementFactory *, GstElementFactoryListType type)
                {
                    if (type == GST_ELEMENT_FACTORY_TYPE_DECODER)
                        return true;
                    if (type == GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO)
                        return true;
                    if (type == (GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO))
                        return true;
                    return false;
                }));

        EXPECT_CALL(*m_glibWrapperMock, gObjectType(m_audioDecoder)).WillRepeatedly(Return(G_TYPE_PARAM));
        EXPECT_CALL(*m_glibWrapperMock, gSignalConnect(_, _, _, _))
            .WillRepeatedly(Invoke(
                [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
                {
                    if (std::strcmp(detailed_signal, "first-audio-frame") == 0 ||
                        std::strcmp(detailed_signal, "first-audio-frame-callback") == 0)
                    {
                        m_firstFrameCallback = c_handler;
                        m_firstFrameData = data;
                    }
                    return kSignalId;
                }));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_audioDecoder))
            .WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void setupAudioDecoder()
    {
        m_gstreamerStub.setupElement(m_audioDecoder);
        waitWorker();
    }

    void firstAudioFrameReceived()
    {
        if (!m_firstFrameCallback || !m_firstFrameData)
        {
            return;
        }

        ExpectMessage<FirstFrameReceivedEvent> expectedFirstFrameReceived{m_clientStub};
        expectedFirstFrameReceived.setFilter([&](const auto &msg) { return msg.source_id() == m_audioSourceId; });
        reinterpret_cast<void (*)(GstElement *, guint, gpointer, gpointer)>(
            m_firstFrameCallback)(m_audioDecoder, 0, nullptr, m_firstFrameData);

        auto receivedFirstFrameReceived{expectedFirstFrameReceived.getMessage()};
        ASSERT_TRUE(receivedFirstFrameReceived);
        EXPECT_EQ(receivedFirstFrameReceived->session_id(), m_sessionId);
        EXPECT_EQ(receivedFirstFrameReceived->source_id(), m_audioSourceId);
    }

    void createAndLoadSession()
    {
        createSession();
        gstPlayerWillBeCreated();
        load();
    }

    void waitForBufferedState(const std::function<void()> &pushFrame)
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushFrame();

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);

        willNotifyPaused();
        notifyPaused();
    }

    void finishStreamAndDestroy(GstAppSrc *appSrc, std::int32_t sourceId, const std::function<void(unsigned)> &sendEos)
    {
        willEos(appSrc);
        sendEos(kFramesToPush);
        gstNotifyEos();
        removeSource(sourceId);
        willStop();
        stop();
        gstPlayerWillBeDestructed();
        destroySession();
    }

private:
    GstElementFactory *m_elementFactory{nullptr};
    GstElement *m_videoDecoder{nullptr};
    GstElement *m_audioDecoder{nullptr};
    guint m_signals[1]{123};
    GCallback m_firstVideoFrameCallback{nullptr};
    gpointer m_firstVideoFrameData{nullptr};
    GCallback m_firstFrameCallback{nullptr};
    gpointer m_firstFrameData{nullptr};
};

/*
 * Component Test: First frame notification test
 * Test Objective:
 *  Test if Rialto Server handles gstreamer first frame signals correctly. The notification should be forwarded to
 *  Rialto Client with FirstFrameReceivedEvent message.
 *
 * Sequence Diagrams:
 *  First frame notification
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
 *  Step 3: Setup Video Decoder
 *   Call SetupElement callback with Video Decoder
 *   First frame callback should be registered.
 *
 *  Step 4: Attach video source
 *   Attach the video source.
 *   Expect that video source is attached.
 *   Expect that rialto source is setup.
 *   Expect that all sources are attached.
 *   Expect that the Playback state has changed to IDLE.
 *
 *  Step 5: Pause
 *   Pause the content.
 *   Expect that gstreamer pipeline is paused.
 *
 *  Step 6: Write 1 video frame
 *   Gstreamer Stub notifies, that it needs video data.
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *   Write 1 frame of video data to the shared buffer.
 *   Send HaveData message.
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *
 *  Step 7: Notify buffered and Paused
 *   Expect that server notifies the client that the Network state has changed to BUFFERED.
 *   Gstreamer Stub notifies, that pipeline state is in PAUSED state.
 *   Expect that server notifies the client that the Network state has changed to PAUSED.
 *
 *  Step 8: First video frame received
 *   Rialto Server will receive first video frame signal.
 *   Rialto Server should send FirstFrameReceivedEvent with video source.
 *
 *  Step 9: End of video stream
 *   Send video haveData with one frame and EOS status.
 *   Expect that Gstreamer is notified about end of stream.
 *
 *  Step 10: Notify end of stream
 *   Simulate, that gst_message_eos is received by Rialto Server.
 *   Expect that server notifies the client that the Network state has changed to END_OF_STREAM.
 *
 *  Step 11: Remove source
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 12: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 13: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  First frame signal is handled by Rialto Server.
 *
 * Code:
 */
TEST_F(FirstFrameNotificationTest, firstFrameNotification)
{
    createAndLoadSession();

    setupElementsCommon("first-video-frame-callback");
    willSetupVideoDecoder();
    setupVideoDecoder();

    videoSourceWillBeAttached();
    attachVideoSource();
    sourceWillBeSetup();
    setupSource();
    willSetupAndAddSource(&m_videoAppSrc);
    willFinishSetupAndAddSource();
    indicateAllSourcesAttached({&m_videoAppSrc});

    willPause();
    pause();

    waitForBufferedState([&]() { pushVideoData(kFramesToPush); });

    firstVideoFrameReceived();

    finishStreamAndDestroy(&m_videoAppSrc, m_videoSourceId, [&](unsigned framesToPush) { eosVideo(framesToPush); });
}

TEST_F(FirstFrameNotificationTest, firstAudioFrameNotification)
{
    createAndLoadSession();

    setupElementsCommon("first-audio-frame");
    willSetupAudioDecoder();
    setupAudioDecoder();

    audioSourceWillBeAttached();
    attachAudioSource();
    sourceWillBeSetup();
    setupSource();
    willSetupAndAddSource(&m_audioAppSrc);
    willFinishSetupAndAddSource();
    indicateAllSourcesAttached({&m_audioAppSrc});

    willPause();
    pause();

    waitForBufferedState([&]() { pushAudioData(kFramesToPush); });

    firstAudioFrameReceived();

    finishStreamAndDestroy(&m_audioAppSrc, m_audioSourceId, [&](unsigned framesToPush) { eosAudio(framesToPush); });
}
} // namespace firebolt::rialto::server::ct
