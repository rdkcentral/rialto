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

#include "tasks/ReportPosition.h"
#include "GstPlayerClientMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include "PlayerContext.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::Invoke;
using testing::StrictMock;

namespace
{
gint64 position{1234};
} // namespace

class ReportPositionTest : public testing::Test
{
protected:
    firebolt::rialto::server::PlayerContext m_context{};
    StrictMock<firebolt::rialto::server::GstPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    GstElement m_pipeline{};

    ReportPositionTest() { m_context.pipeline = &m_pipeline; }
};

TEST_F(ReportPositionTest, shouldReportPosition)
{
    EXPECT_CALL(*m_gstWrapper, gstElementQueryPosition(&m_pipeline, GST_FORMAT_TIME, NotNullMatcher()))
        .WillOnce(Invoke(
            [this](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = position;
                return TRUE;
            }));
    EXPECT_CALL(m_gstPlayerClient, notifyPosition(position));
    firebolt::rialto::server::ReportPosition task{m_context, &m_gstPlayerClient, m_gstWrapper};
    task.execute();
}

TEST_F(ReportPositionTest, shouldFailToReportPosition)
{
    EXPECT_CALL(*m_gstWrapper, gstElementQueryPosition(&m_pipeline, GST_FORMAT_TIME, NotNullMatcher()))
        .WillOnce(Invoke(
            [this](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = -1;
                return TRUE;
            }));
    firebolt::rialto::server::ReportPosition task{m_context, &m_gstPlayerClient, m_gstWrapper};
    task.execute();
}
