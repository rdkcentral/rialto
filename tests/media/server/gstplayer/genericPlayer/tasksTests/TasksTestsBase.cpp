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

#include "tasks/generic/SetupElement.h"
#include "tasks/generic/SetVideoGeometry.h"
#include "tasks/generic/SetupSource.h"
#include "tasks/generic/SetVolume.h"
#include "GenericPlayerContext.h"
#include "GlibWrapperMock.h"
#include "GstGenericPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include "TasksTestsBase.h"
#include <gst/gst.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

namespace
{
std::shared_ptr<firebolt::rialto::server::GenericPlayerContext> m_context;
std::shared_ptr<firebolt::rialto::server::GlibWrapperMock> m_glibWrapper;
std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper;
std::shared_ptr<StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock>> m_gstPlayer;
GstElement m_element{};
GstElementFactory *m_elementFactory{};
gulong m_signalId{123};
guint m_signals[1]{123};
GCallback m_audioUnderflowCallback;
GCallback m_videoUnderflowCallback;
firebolt::rialto::server::Rectangle m_rectangle{1, 2, 3, 4};
GstElement m_pipeline{};
constexpr double kVolume{0.7};
} // namespace

TasksTestsBase::TasksTestsBase()
{
    m_context = std::make_shared<firebolt::rialto::server::GenericPlayerContext>();
    m_glibWrapper = std::make_shared<StrictMock<firebolt::rialto::server::GlibWrapperMock>>();
    m_gstWrapper = std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>();
    m_gstPlayer = std::make_shared<StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock>>();

    m_context->pipeline = &m_pipeline;
}

TasksTestsBase::~TasksTestsBase()
{
    m_gstPlayer.reset();
    m_gstWrapper.reset();
    m_glibWrapper.reset();
    m_context.reset();

    m_audioUnderflowCallback = nullptr;
    m_videoUnderflowCallback = nullptr;
}

void TasksTestsBase::initSetupElementTest()
{
    gst_init(nullptr, nullptr);
    m_elementFactory = gst_element_factory_find("fakesrc");
}

void TasksTestsBase::termSetupElementTest()
{
    gst_object_unref(m_elementFactory);
}

void TasksTestsBase::expectSetupVideoElement()
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
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_videoUnderflowCallback = c_handler;
                return m_signalId;
            }));
    EXPECT_CALL(*m_gstWrapper, gstObjectUnref(_));
}

void TasksTestsBase::expectSetupAudioElement()
{
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
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
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_audioUnderflowCallback = c_handler;
                return m_signalId;
            }));
    EXPECT_CALL(*m_gstWrapper, gstObjectUnref(_));
}

void TasksTestsBase::shouldSetupVideoElementOnly()
{
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
    expectSetupVideoElement();
}

void TasksTestsBase::shouldSetupVideoElementWesterossink()
{
    m_context->pendingGeometry = firebolt::rialto::server::Rectangle{1, 2, 3, 4};
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(true));
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
    EXPECT_CALL(*m_gstPlayer, setWesterossinkRectangle());
    expectSetupVideoElement();
}

void TasksTestsBase::shouldSetupVideoElementAmlhalasink()
{
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(true));
    EXPECT_CALL(*m_glibWrapper, gObjectSetStub(G_OBJECT(&m_element), CharStrMatcher("wait-video")));
    EXPECT_CALL(*m_glibWrapper, gObjectSetStub(G_OBJECT(&m_element), CharStrMatcher("a-wait-timeout")));
    EXPECT_CALL(*m_glibWrapper, gObjectSetStub(G_OBJECT(&m_element), CharStrMatcher("disable-xrun")));
    expectSetupVideoElement();
}

void TasksTestsBase::shouldSetupVideoElementPendingGeometryNonWesterissink()
{
    m_context->pendingGeometry = firebolt::rialto::server::Rectangle{1, 2, 3, 4};
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
    expectSetupVideoElement();
}

void TasksTestsBase::shouldSetupAudioElementOnly()
{
    expectSetupAudioElement();
}

void TasksTestsBase::shouldSetVideoUnderflowCallback()
{
    ASSERT_TRUE(m_videoUnderflowCallback);
    EXPECT_CALL(*m_gstPlayer, scheduleVideoUnderflow());
}

void TasksTestsBase::triggerVideoUnderflowCallback()
{
    ((void (*)(GstElement *, guint, gpointer, gpointer))m_videoUnderflowCallback)(&m_element, 0, nullptr, m_gstPlayer.get());
}

void TasksTestsBase::shouldSetAudioUnderflowCallback()
{
    ASSERT_TRUE(m_audioUnderflowCallback);
    EXPECT_CALL(*m_gstPlayer, scheduleAudioUnderflow());
}

void TasksTestsBase::triggerAudioUnderflowCallback()
{
    ((void (*)(GstElement *, guint, gpointer, gpointer))m_audioUnderflowCallback)(&m_element, 0, nullptr, m_gstPlayer.get());
}

void TasksTestsBase::triggerSetupElement()
{
    firebolt::rialto::server::tasks::generic::SetupElement task{*m_context, m_gstWrapper, m_glibWrapper, *m_gstPlayer,
                                                                &m_element};
    task.execute();
}

void TasksTestsBase::setPipelineToNull()
{
    m_context->pipeline = nullptr;
}

void TasksTestsBase::triggerSetVideoGeometryFailure()
{
    firebolt::rialto::server::tasks::generic::SetVideoGeometry task{*m_context, *m_gstPlayer, m_rectangle};
    task.execute();
    EXPECT_EQ(m_context->pendingGeometry, m_rectangle);
}

void TasksTestsBase::shouldSetVideoGeometry()
{
    EXPECT_CALL(*m_gstPlayer, setWesterossinkRectangle());
}

void TasksTestsBase::triggerSetVideoGeometrySuccess()
{
    firebolt::rialto::server::tasks::generic::SetVideoGeometry task{*m_context, *m_gstPlayer, m_rectangle};
    task.execute();
    EXPECT_EQ(m_context->pendingGeometry, m_rectangle);
}

void TasksTestsBase::setAllSourcesAttached()
{
    m_context->wereAllSourcesAttached = true;
}

void TasksTestsBase::shouldScheduleAllSourcesAttached()
{
    EXPECT_CALL(*m_gstPlayer, scheduleAllSourcesAttached());
}

void TasksTestsBase::triggerSetupSource()
{
    firebolt::rialto::server::tasks::generic::SetupSource task{*m_context, *m_gstPlayer, &m_element};
    task.execute();
    EXPECT_EQ(m_context->source, &m_element);
}

void TasksTestsBase::shouldSetGstVolume()
{
    EXPECT_CALL(*m_gstWrapper, gstStreamVolumeSetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR, kVolume));
}

void TasksTestsBase::triggerSetVolume()
{
    firebolt::rialto::server::tasks::generic::SetVolume task{*m_context, m_gstWrapper, kVolume};
    task.execute();
}
