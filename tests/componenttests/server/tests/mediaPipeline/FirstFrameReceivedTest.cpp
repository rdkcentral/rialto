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
const std::string kElementName{"AudioSink"};
constexpr gulong kSignalId{123};
} // namespace

namespace firebolt::rialto::server::ct
{
class FirstFrameReceivedTest : public MediaPipelineTest
{
public:
    FirstFrameReceivedTest()
    {
        m_elementFactory = gst_element_factory_find("fakesrc");
        m_audioSink = gst_element_factory_create(m_elementFactory, nullptr);
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(_)).WillRepeatedly(Return(m_elementFactory));
    }

    ~FirstFrameReceivedTest() override
    {
        gst_object_unref(m_audioSink);
        gst_object_unref(m_elementFactory);
    }

    void setupElementsCommon()
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
            .WillOnce(Invoke(
                [&](guint signal_id, GSignalQuery *query) { query->signal_name = "buffer-underflow-callback"; }))
            .WillOnce(Invoke(
                [&](guint signal_id, GSignalQuery *query) { query->signal_name = "first-audio-frame"; }));
        EXPECT_CALL(*m_glibWrapperMock, gFree(m_signals)).Times(2);
    }

    void willSetupAudioSink()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_DECODER))
            .WillOnce(Return(FALSE));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_SINK))
            .WillOnce(Return(TRUE));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO))
            .WillOnce(Return(TRUE));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstElementFactoryListIsType(m_elementFactory,
                                                GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
            .WillOnce(Return(FALSE));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstElementFactoryListIsType(m_elementFactory,
                                                GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO))
            .WillOnce(Return(FALSE));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstElementFactoryListIsType(m_elementFactory,
                                                GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO))
            .WillOnce(Return(TRUE));

        EXPECT_CALL(*m_glibWrapperMock, gObjectType(m_audioSink)).WillRepeatedly(Return(G_TYPE_PARAM));
        EXPECT_CALL(*m_glibWrapperMock, gSignalConnect(_, StrEq("buffer-underflow-callback"), _, _))
            .WillOnce(Return(kSignalId));
        EXPECT_CALL(*m_glibWrapperMock, gSignalConnect(_, StrEq("first-audio-frame"), _, _))
            .WillOnce(Invoke(
                [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
                {
                    m_firstFrameCallback = c_handler;
                    m_firstFrameData = data;
                    return kSignalId;
                }));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_audioSink))
            .WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void setupAudioSink()
    {
        m_gstreamerStub.setupElement(m_audioSink);
        waitWorker();
    }

    void firstAudioFrameReceived()
    {
        ExpectMessage<FirstFrameReceivedEvent> expectedFirstFrameReceived{m_clientStub};
        expectedFirstFrameReceived.setFilter([&](const auto &msg) { return msg.source_id() == m_audioSourceId; });

        ASSERT_TRUE(m_firstFrameCallback);
        ASSERT_TRUE(m_firstFrameData);
        reinterpret_cast<void (*)(GstElement *, gpointer)>(m_firstFrameCallback)(m_audioSink, m_firstFrameData);

        auto receivedFirstFrameReceived{expectedFirstFrameReceived.getMessage()};
        ASSERT_TRUE(receivedFirstFrameReceived);
        EXPECT_EQ(receivedFirstFrameReceived->session_id(), m_sessionId);
        EXPECT_EQ(receivedFirstFrameReceived->source_id(), m_audioSourceId);
    }

private:
    GstElementFactory *m_elementFactory{nullptr};
    GstElement *m_audioSink{nullptr};
    guint m_signals[1]{123};
    GCallback m_firstFrameCallback{nullptr};
    gpointer m_firstFrameData{nullptr};
};

TEST_F(FirstFrameReceivedTest, shouldPropagateFirstAudioFrameSignal)
{
    createSession();

    gstPlayerWillBeCreated();
    load();

    setupElementsCommon();
    willSetupAudioSink();
    setupAudioSink();

    attachAudio();
    attachVideo();
    pause();

    firstAudioFrameReceived();

    stop();
    destroySession();
}
} // namespace firebolt::rialto::server::ct