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

#include "tasks/generic/RenderFrame.h"

#include "GenericPlayerContext.h"
#include "GlibWrapperMock.h"
#include "GstGenericPlayerClientMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrEq;
using testing::StrictMock;

class RenderFrameTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context{};
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GlibWrapperMock>>()};
    GstElement m_pipeline{};
    GstElement m_videoSink{};

    RenderFrameTest() { m_context.pipeline = &m_pipeline; }
};

TEST_F(RenderFrameTest, shouldRenderFrame)
{
    GParamSpec paramSpec{};
    GstEvent event{};

    EXPECT_CALL(*m_glibWrapper, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = &m_videoSink;
            }));

    EXPECT_CALL(*m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(&m_videoSink), StrEq("frame-step-on-preroll")))
        .WillOnce(Return(&paramSpec));
    EXPECT_CALL(*m_glibWrapper, gObjectSetStub(&m_videoSink, StrEq("frame-step-on-preroll"))).Times(2);
    EXPECT_CALL(*m_gstWrapper, gstEventNewStep(GST_FORMAT_BUFFERS, 1, 1.0, true, false)).WillOnce(Return(&event));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_videoSink, &event));
    EXPECT_CALL(*m_gstWrapper, gstObjectUnref(GST_OBJECT(&m_videoSink)));

    firebolt::rialto::server::tasks::generic::RenderFrame task{m_context, m_gstWrapper, m_glibWrapper};
    task.execute();
}

TEST_F(RenderFrameTest, RenderFrameFailsOnGettingSink)
{
    EXPECT_CALL(*m_glibWrapper, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = nullptr;
            }));

    firebolt::rialto::server::tasks::generic::RenderFrame task{m_context, m_gstWrapper, m_glibWrapper};
    task.execute();
}

TEST_F(RenderFrameTest, RenderFrameFailsOnFindProperty)
{
    EXPECT_CALL(*m_glibWrapper, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = &m_videoSink;
            }));

    EXPECT_CALL(*m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(&m_videoSink), StrEq("frame-step-on-preroll")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstObjectUnref(GST_OBJECT(&m_videoSink)));

    firebolt::rialto::server::tasks::generic::RenderFrame task{m_context, m_gstWrapper, m_glibWrapper};
    task.execute();
}
