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

#include "GenericTasksTestsBase.h"
#include "GenericTasksTestsContext.h"
#include "Matchers.h"
#include "tasks/generic/SetVideoGeometry.h"
#include "tasks/generic/SetVolume.h"
#include "tasks/generic/SetupElement.h"
#include "tasks/generic/SetupSource.h"
#include <gst/gst.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

namespace
{
std::shared_ptr<GenericTasksTestsContext> testContext;

constexpr firebolt::rialto::server::Rectangle kRectangle{1, 2, 3, 4};
constexpr double kVolume{0.7};
constexpr gulong kSignalId{123};
} // namespace

GenericTasksTestsBase::GenericTasksTestsBase()
{
    testContext = std::make_shared<GenericTasksTestsContext>();

    gst_init(nullptr, nullptr);

    testContext->m_elementFactory = gst_element_factory_find("fakesrc");
    testContext->m_context.pipeline = &testContext->m_pipeline;
}

GenericTasksTestsBase::~GenericTasksTestsBase()
{
    gst_object_unref(testContext->m_elementFactory);

    testContext.reset();
}

void GenericTasksTestsBase::expectSetupVideoElement()
{
    EXPECT_CALL(*(testContext->m_gstWrapper), gstElementGetFactory(_)).WillOnce(Return(testContext->m_elementFactory));
    EXPECT_CALL(*(testContext->m_gstWrapper),
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_DECODER))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*(testContext->m_gstWrapper),
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*(testContext->m_glibWrapper), gObjectType(&(testContext->m_element))).WillRepeatedly(Return(G_TYPE_PARAM));
    EXPECT_CALL(*(testContext->m_glibWrapper), gSignalListIds(_, _))
        .WillOnce(Invoke(
            [&](GType itype, guint *n_ids)
            {
                *n_ids = 1;
                return testContext->m_signals;
            }));
    EXPECT_CALL(*(testContext->m_glibWrapper), gSignalQuery(testContext->m_signals[0], _))
        .WillOnce(Invoke([&](guint signal_id, GSignalQuery *query) { query->signal_name = "buffer-underflow-callback"; }));
    EXPECT_CALL(*(testContext->m_glibWrapper), gFree(testContext->m_signals));
    EXPECT_CALL(*(testContext->m_glibWrapper), gSignalConnect(_, CharStrMatcher("buffer-underflow-callback"), _, _))
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                testContext->m_videoUnderflowCallback = c_handler;
                return kSignalId;
            }));
    EXPECT_CALL(*(testContext->m_gstWrapper), gstObjectUnref(_));
}

void GenericTasksTestsBase::expectSetupAudioElement()
{
    EXPECT_CALL(*(testContext->m_glibWrapper), gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*(testContext->m_glibWrapper), gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
    EXPECT_CALL(*(testContext->m_gstWrapper), gstElementGetFactory(_)).WillRepeatedly(Return(testContext->m_elementFactory));
    EXPECT_CALL(*(testContext->m_gstWrapper),
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_DECODER))
        .WillRepeatedly(Return(TRUE));
    EXPECT_CALL(*(testContext->m_gstWrapper),
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(FALSE));
    EXPECT_CALL(*(testContext->m_gstWrapper),
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*(testContext->m_glibWrapper), gObjectType(&(testContext->m_element))).WillRepeatedly(Return(G_TYPE_PARAM));
    EXPECT_CALL(*(testContext->m_glibWrapper), gSignalListIds(_, _))
        .WillOnce(Invoke(
            [&](GType itype, guint *n_ids)
            {
                *n_ids = 1;
                return testContext->m_signals;
            }));
    EXPECT_CALL(*(testContext->m_glibWrapper), gSignalQuery(testContext->m_signals[0], _))
        .WillOnce(Invoke([&](guint signal_id, GSignalQuery *query) { query->signal_name = "buffer-underflow-callback"; }));
    EXPECT_CALL(*(testContext->m_glibWrapper), gFree(testContext->m_signals));
    EXPECT_CALL(*(testContext->m_glibWrapper), gSignalConnect(_, _, _, _))
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                testContext->m_audioUnderflowCallback = c_handler;
                return kSignalId;
            }));
    EXPECT_CALL(*(testContext->m_gstWrapper), gstObjectUnref(_));
}

void GenericTasksTestsBase::shouldSetupVideoElementOnly()
{
    EXPECT_CALL(*(testContext->m_glibWrapper), gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*(testContext->m_glibWrapper), gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
    expectSetupVideoElement();
}

void GenericTasksTestsBase::shouldSetupVideoElementWesterossink()
{
    testContext->m_context.pendingGeometry = kRectangle;
    EXPECT_CALL(*(testContext->m_glibWrapper), gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(true));
    EXPECT_CALL(*(testContext->m_glibWrapper), gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
    EXPECT_CALL(testContext->m_gstPlayer, setWesterossinkRectangle());
    expectSetupVideoElement();
}

void GenericTasksTestsBase::shouldSetupVideoElementAmlhalasink()
{
    EXPECT_CALL(*(testContext->m_glibWrapper), gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*(testContext->m_glibWrapper), gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(true));
    EXPECT_CALL(*(testContext->m_glibWrapper),
                gObjectSetStub(G_OBJECT(&(testContext->m_element)), CharStrMatcher("wait-video")));
    EXPECT_CALL(*(testContext->m_glibWrapper),
                gObjectSetStub(G_OBJECT(&(testContext->m_element)), CharStrMatcher("a-wait-timeout")));
    EXPECT_CALL(*(testContext->m_glibWrapper),
                gObjectSetStub(G_OBJECT(&(testContext->m_element)), CharStrMatcher("disable-xrun")));
    expectSetupVideoElement();
}

void GenericTasksTestsBase::shouldSetupVideoElementPendingGeometryNonWesterissink()
{
    testContext->m_context.pendingGeometry = kRectangle;
    EXPECT_CALL(*(testContext->m_glibWrapper), gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*(testContext->m_glibWrapper), gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
    expectSetupVideoElement();
}

void GenericTasksTestsBase::shouldSetupAudioElementOnly()
{
    expectSetupAudioElement();
}

void GenericTasksTestsBase::shouldSetVideoUnderflowCallback()
{
    ASSERT_TRUE(testContext->m_videoUnderflowCallback);
    EXPECT_CALL(testContext->m_gstPlayer, scheduleVideoUnderflow());
}

void GenericTasksTestsBase::triggerVideoUnderflowCallback()
{
    ((void (*)(GstElement *, guint, gpointer,
               gpointer))testContext->m_videoUnderflowCallback)(&(testContext->m_element), 0, nullptr,
                                                                &(testContext->m_gstPlayer));
}

void GenericTasksTestsBase::shouldSetAudioUnderflowCallback()
{
    ASSERT_TRUE(testContext->m_audioUnderflowCallback);
    EXPECT_CALL(testContext->m_gstPlayer, scheduleAudioUnderflow());
}

void GenericTasksTestsBase::triggerAudioUnderflowCallback()
{
    ((void (*)(GstElement *, guint, gpointer,
               gpointer))testContext->m_audioUnderflowCallback)(&(testContext->m_element), 0, nullptr,
                                                                &(testContext->m_gstPlayer));
}

void GenericTasksTestsBase::triggerSetupElement()
{
    firebolt::rialto::server::tasks::generic::SetupElement task{testContext->m_context, testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper, testContext->m_gstPlayer,
                                                                &(testContext->m_element)};
    task.execute();
}

void GenericTasksTestsBase::setPipelineToNull()
{
    testContext->m_context.pipeline = nullptr;
}

void GenericTasksTestsBase::triggerSetVideoGeometryFailure()
{
    firebolt::rialto::server::tasks::generic::SetVideoGeometry task{testContext->m_context, testContext->m_gstPlayer,
                                                                    kRectangle};
    task.execute();
    EXPECT_EQ(testContext->m_context.pendingGeometry, kRectangle);
}

void GenericTasksTestsBase::shouldSetVideoGeometry()
{
    EXPECT_CALL(testContext->m_gstPlayer, setWesterossinkRectangle());
}

void GenericTasksTestsBase::triggerSetVideoGeometrySuccess()
{
    firebolt::rialto::server::tasks::generic::SetVideoGeometry task{testContext->m_context, testContext->m_gstPlayer,
                                                                    kRectangle};
    task.execute();
    EXPECT_EQ(testContext->m_context.pendingGeometry, kRectangle);
}

void GenericTasksTestsBase::setAllSourcesAttached()
{
    testContext->m_context.wereAllSourcesAttached = true;
}

void GenericTasksTestsBase::shouldScheduleAllSourcesAttached()
{
    EXPECT_CALL(testContext->m_gstPlayer, scheduleAllSourcesAttached());
}

void GenericTasksTestsBase::triggerSetupSource()
{
    firebolt::rialto::server::tasks::generic::SetupSource task{testContext->m_context, testContext->m_gstPlayer,
                                                               &(testContext->m_element)};
    task.execute();
    EXPECT_EQ(testContext->m_context.source, &(testContext->m_element));
}

void GenericTasksTestsBase::shouldSetGstVolume()
{
    EXPECT_CALL(*(testContext->m_gstWrapper), gstStreamVolumeSetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR, kVolume));
}

void GenericTasksTestsBase::triggerSetVolume()
{
    firebolt::rialto::server::tasks::generic::SetVolume task{testContext->m_context, testContext->m_gstWrapper, kVolume};
    task.execute();
}
