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

#include "tasks/generic/CheckAudioUnderflow.h"
#include "GstGenericPlayerClientMock.h"
#include "GstGenericPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include "GenericPlayerContext.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::Invoke;
using testing::StrictMock;
namespace
{
constexpr gint64 position = 350 * 1000000 + 1;
} // namespace
class CheckAudioUnderflowTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context{};
    StrictMock<firebolt::rialto::server::GstGenericPlayerClientMock> m_gstPlayerClient;
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    GstElement m_pipeline{};

    CheckAudioUnderflowTest() { m_context.pipeline = &m_pipeline; }
};

TEST_F(CheckAudioUnderflowTest, shouldNotTriggerAudioUnderflow)
{
    EXPECT_CALL(*m_gstWrapper, gstElementQueryPosition(&m_pipeline, GST_FORMAT_TIME, NotNullMatcher()))
        .WillOnce(Invoke(
            [this](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = 0;
                return TRUE;
            }));
    GstElement audioAppSrc;
    m_context.audioAppSrc = &audioAppSrc;
    m_context.lastAudioSampleTimestamps = position;
    m_context.audioUnderflowOccured = false;
    firebolt::rialto::server::CheckAudioUnderflow task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper};
    task.execute();

    EXPECT_FALSE(m_context.audioUnderflowOccured);
}

TEST_F(CheckAudioUnderflowTest, shouldTriggerAudioUnderflow)
{
    EXPECT_CALL(*m_gstWrapper, gstElementQueryPosition(&m_pipeline, GST_FORMAT_TIME, NotNullMatcher()))
        .WillOnce(Invoke(
            [this](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = position;
                return TRUE;
            }));
    EXPECT_CALL(*m_gstWrapper, gstElementGetState(&m_pipeline))
        .WillOnce(Invoke([this](GstElement *element) { return GST_STATE_PLAYING; }));
    EXPECT_CALL(*m_gstWrapper, gstElementGetPendingState(&m_pipeline))
        .WillOnce(Invoke([this](GstElement *element) { return GST_STATE_VOID_PENDING; }));
    EXPECT_CALL(m_gstPlayer, stopPositionReportingAndCheckAudioUnderflowTimer());
    EXPECT_CALL(m_gstPlayer, changePipelineState(GST_STATE_PAUSED));
    EXPECT_CALL(m_gstPlayerClient, notifyNetworkState(firebolt::rialto::NetworkState::STALLED));
    GstElement audioAppSrc;
    m_context.audioAppSrc = &audioAppSrc;
    m_context.lastAudioSampleTimestamps = 0;
    m_context.audioUnderflowOccured = false;
    firebolt::rialto::server::CheckAudioUnderflow task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper};
    task.execute();
}
