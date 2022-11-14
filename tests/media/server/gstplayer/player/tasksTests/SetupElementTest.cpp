/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "tasks/SetupElement.h"
#include "GlibWrapperMock.h"
#include "GstPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include "PlayerContext.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

class SetupElementTest : public testing::Test
{
protected:
    firebolt::rialto::server::PlayerContext m_context{};
    std::shared_ptr<firebolt::rialto::server::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GlibWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    StrictMock<firebolt::rialto::server::GstPlayerPrivateMock> m_gstPlayer;
    GstElement m_element{};
    GstElementFactory *m_elementFactory{};
    guint m_signals[1]{123};
    GCallback m_audioUnderflowCallback;
    GCallback m_videoUnderflowCallback;

    SetupElementTest()
    {
        gst_init(nullptr, nullptr);
        m_elementFactory = gst_element_factory_find("fakesrc");
    }

    ~SetupElementTest() { gst_object_unref(m_elementFactory); }

    void expectSetupVideoElement()
    {
        EXPECT_CALL(*m_gstWrapper, gstElementGetFactory(_)).WillOnce(Return(m_elementFactory));
        EXPECT_CALL(*m_gstWrapper, gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_DECODER))
            .WillOnce(Return(TRUE));
        EXPECT_CALL(*m_gstWrapper, gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
            .WillOnce(Return(TRUE));
        EXPECT_CALL(*m_glibWrapper, gObjectType(&m_element)).WillRepeatedly(Return(G_TYPE_PARAM));
        EXPECT_CALL(*m_glibWrapper, gSignalListIds(_, _))
            .WillOnce(Invoke(
                [&](GType itype, guint *n_ids)
                {
                    *n_ids = 1;
                    return m_signals;
                }));
        EXPECT_CALL(*m_glibWrapper, gSignalQuery(m_signals[0], _))
            .WillOnce(Invoke([&](guint signal_id, GSignalQuery *query)
                             { query->signal_name = "buffer-underflow-callback"; }));
        EXPECT_CALL(*m_glibWrapper, gFree(m_signals));
        EXPECT_CALL(*m_glibWrapper, gSignalConnect(_, CharStrMatcher("buffer-underflow-callback"), _, _))
            .WillOnce(Invoke([&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
                             { m_videoUnderflowCallback = c_handler; }));
        EXPECT_CALL(*m_gstWrapper, gstObjectUnref(_));
    }

    void expectSetupAudioElement()
    {
        EXPECT_CALL(*m_gstWrapper, gstElementGetFactory(_)).WillRepeatedly(Return(m_elementFactory));
        EXPECT_CALL(*m_gstWrapper, gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_DECODER))
            .WillRepeatedly(Return(TRUE));
        EXPECT_CALL(*m_gstWrapper, gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
            .WillOnce(Return(FALSE));
        EXPECT_CALL(*m_gstWrapper, gstElementFactoryListIsType(m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO))
            .WillOnce(Return(TRUE));
        EXPECT_CALL(*m_glibWrapper, gObjectType(&m_element)).WillRepeatedly(Return(G_TYPE_PARAM));
        EXPECT_CALL(*m_glibWrapper, gSignalListIds(_, _))
            .WillOnce(Invoke(
                [&](GType itype, guint *n_ids)
                {
                    *n_ids = 1;
                    return m_signals;
                }));
        EXPECT_CALL(*m_glibWrapper, gSignalQuery(m_signals[0], _))
            .WillOnce(Invoke([&](guint signal_id, GSignalQuery *query)
                             { query->signal_name = "buffer-underflow-callback"; }));
        EXPECT_CALL(*m_glibWrapper, gFree(m_signals));
        EXPECT_CALL(*m_glibWrapper, gSignalConnect(_, _, _, _))
            .WillOnce(Invoke([&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
                             { m_audioUnderflowCallback = c_handler; }));
        EXPECT_CALL(*m_gstWrapper, gstObjectUnref(_));
    }
};

TEST_F(SetupElementTest, shouldSetupVideoElement)
{
    firebolt::rialto::server::SetupElement task{m_context, m_gstWrapper, m_glibWrapper, m_gstPlayer, &m_element};
    expectSetupVideoElement();
    task.execute();
}

TEST_F(SetupElementTest, shouldSetupVideoElementWithPendingGeometry)
{
    m_context.pendingGeometry = firebolt::rialto::server::Rectangle{1, 2, 3, 4};
    firebolt::rialto::server::SetupElement task{m_context, m_gstWrapper, m_glibWrapper, m_gstPlayer, &m_element};
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(true));
    EXPECT_CALL(m_gstPlayer, setWesterossinkRectangle());
    expectSetupVideoElement();
    task.execute();
}

TEST_F(SetupElementTest, shouldSetupVideoElementWithPendingGeometryOtherThanWesterosSink)
{
    m_context.pendingGeometry = firebolt::rialto::server::Rectangle{1, 2, 3, 4};
    firebolt::rialto::server::SetupElement task{m_context, m_gstWrapper, m_glibWrapper, m_gstPlayer, &m_element};
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    expectSetupVideoElement();
    task.execute();
}

TEST_F(SetupElementTest, shouldSetupAudioElement)
{
    firebolt::rialto::server::SetupElement task{m_context, m_gstWrapper, m_glibWrapper, m_gstPlayer, &m_element};
    expectSetupAudioElement();
    task.execute();
}

TEST_F(SetupElementTest, shouldReportVideoUnderflow)
{
    firebolt::rialto::server::SetupElement task{m_context, m_gstWrapper, m_glibWrapper, m_gstPlayer, &m_element};
    expectSetupVideoElement();
    task.execute();
    EXPECT_TRUE(m_videoUnderflowCallback);
    EXPECT_CALL(m_gstPlayer, scheduleVideoUnderflow());
    ((void (*)(GstElement *, guint, gpointer, gpointer))m_videoUnderflowCallback)(&m_element, 0, nullptr, &m_gstPlayer);
}

TEST_F(SetupElementTest, shouldReportAudioUnderflow)
{
    firebolt::rialto::server::SetupElement task{m_context, m_gstWrapper, m_glibWrapper, m_gstPlayer, &m_element};
    expectSetupAudioElement();
    task.execute();
    EXPECT_TRUE(m_audioUnderflowCallback);
    EXPECT_CALL(m_gstPlayer, scheduleAudioUnderflow());
    ((void (*)(GstElement *, guint, gpointer, gpointer))m_audioUnderflowCallback)(&m_element, 0, nullptr, &m_gstPlayer);
}
