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

#include "tasks/SetPlaybackRate.h"
#include "GlibWrapperMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include "PlayerContext.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

namespace
{
constexpr double kRate{1.5};
} // namespace

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

struct SetPlaybackRateTest : public testing::Test
{
    firebolt::rialto::server::PlayerContext m_context{};
    std::shared_ptr<firebolt::rialto::server::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GlibWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    GstElement m_pipeline{};
    GstElement m_audioSink{};
    GstStructure m_structure{};
    GstEvent m_event{};
    GstSegment m_segment{};
};

TEST_F(SetPlaybackRateTest, shouldNotChangePlaybackRateIfItsAlreadySet)
{
    m_context.playbackRate = kRate;
    firebolt::rialto::server::SetPlaybackRate task{m_context, m_gstWrapper, m_glibWrapper, kRate};
    task.execute();
    EXPECT_EQ(m_context.pendingPlaybackRate, firebolt::rialto::server::kNoPendingPlaybackRate);
    EXPECT_EQ(m_context.playbackRate, kRate);
}

TEST_F(SetPlaybackRateTest, shouldNotChangePlaybackRateIfPipelineIsNull)
{
    firebolt::rialto::server::SetPlaybackRate task{m_context, m_gstWrapper, m_glibWrapper, kRate};
    task.execute();
    EXPECT_EQ(m_context.pendingPlaybackRate, kRate);
    EXPECT_EQ(m_context.playbackRate, 1.0);
}

TEST_F(SetPlaybackRateTest, shouldNotChangePlaybackRateIfPipelineStateIsBelowPlaying)
{
    m_context.pipeline = &m_pipeline;
    firebolt::rialto::server::SetPlaybackRate task{m_context, m_gstWrapper, m_glibWrapper, kRate};
    task.execute();
    EXPECT_EQ(m_context.pendingPlaybackRate, kRate);
    EXPECT_EQ(m_context.playbackRate, 1.0);
}

TEST_F(SetPlaybackRateTest, shouldSetPlaybackRateAudioSinkNull)
{
    GST_STATE(&m_pipeline) = GST_STATE_PLAYING;
    m_context.pipeline = &m_pipeline;
    firebolt::rialto::server::SetPlaybackRate task{m_context, m_gstWrapper, m_glibWrapper, kRate};
    EXPECT_CALL(*m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _));
    EXPECT_CALL(*m_gstWrapper, gstStructureNewStub(CharStrMatcher("custom-instant-rate-change"), CharStrMatcher("rate"),
                                                   G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&m_structure));
    EXPECT_CALL(*m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &m_structure)).WillOnce(Return(&m_event));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(_, &m_event)).WillOnce(Return(TRUE));
    task.execute();
    EXPECT_EQ(m_context.playbackRate, kRate);
    EXPECT_EQ(m_context.pendingPlaybackRate, firebolt::rialto::server::kNoPendingPlaybackRate);
}

TEST_F(SetPlaybackRateTest, shouldFailToSetPlaybackRateAudioSinkNull)
{
    GST_STATE(&m_pipeline) = GST_STATE_PLAYING;
    m_context.pipeline = &m_pipeline;
    firebolt::rialto::server::SetPlaybackRate task{m_context, m_gstWrapper, m_glibWrapper, kRate};
    EXPECT_CALL(*m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _));
    EXPECT_CALL(*m_gstWrapper, gstStructureNewStub(CharStrMatcher("custom-instant-rate-change"), CharStrMatcher("rate"),
                                                   G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&m_structure));
    EXPECT_CALL(*m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &m_structure)).WillOnce(Return(&m_event));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(_, &m_event)).WillOnce(Return(FALSE));
    task.execute();
    EXPECT_EQ(m_context.playbackRate, 1.0);
    EXPECT_EQ(m_context.pendingPlaybackRate, firebolt::rialto::server::kNoPendingPlaybackRate);
}

TEST_F(SetPlaybackRateTest, shouldSetPlaybackRateAudioSinkOtherThanAmlhala)
{
    GST_STATE(&m_pipeline) = GST_STATE_PLAYING;
    m_context.pipeline = &m_pipeline;
    firebolt::rialto::server::SetPlaybackRate task{m_context, m_gstWrapper, m_glibWrapper, kRate};
    EXPECT_CALL(*m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _))
        .WillOnce(Invoke([&](gpointer object, const gchar *first_property_name, void *element) {
            GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
            *elementPtr = &m_audioSink;
        }));
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(*m_gstWrapper, gstStructureNewStub(CharStrMatcher("custom-instant-rate-change"), CharStrMatcher("rate"),
                                                   G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&m_structure));
    EXPECT_CALL(*m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &m_structure)).WillOnce(Return(&m_event));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(_, &m_event)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_glibWrapper, gObjectUnref(&m_audioSink));
    task.execute();
    EXPECT_EQ(m_context.playbackRate, kRate);
    EXPECT_EQ(m_context.pendingPlaybackRate, firebolt::rialto::server::kNoPendingPlaybackRate);
}

TEST_F(SetPlaybackRateTest, shouldFailToSetPlaybackRateAudioSinkOtherThanAmlhala)
{
    GST_STATE(&m_pipeline) = GST_STATE_PLAYING;
    m_context.pipeline = &m_pipeline;
    firebolt::rialto::server::SetPlaybackRate task{m_context, m_gstWrapper, m_glibWrapper, kRate};
    EXPECT_CALL(*m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _))
        .WillOnce(Invoke([&](gpointer object, const gchar *first_property_name, void *element) {
            GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
            *elementPtr = &m_audioSink;
        }));
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(*m_gstWrapper, gstStructureNewStub(CharStrMatcher("custom-instant-rate-change"), CharStrMatcher("rate"),
                                                   G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&m_structure));
    EXPECT_CALL(*m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &m_structure)).WillOnce(Return(&m_event));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(_, &m_event)).WillOnce(Return(FALSE));
    EXPECT_CALL(*m_glibWrapper, gObjectUnref(&m_audioSink));
    task.execute();
    EXPECT_EQ(m_context.playbackRate, 1.0);
    EXPECT_EQ(m_context.pendingPlaybackRate, firebolt::rialto::server::kNoPendingPlaybackRate);
}

TEST_F(SetPlaybackRateTest, shouldSetPlaybackRateAmlhalaAudioSink)
{
    GST_STATE(&m_pipeline) = GST_STATE_PLAYING;
    m_context.pipeline = &m_pipeline;
    firebolt::rialto::server::SetPlaybackRate task{m_context, m_gstWrapper, m_glibWrapper, kRate};
    EXPECT_CALL(*m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _))
        .WillOnce(Invoke([&](gpointer object, const gchar *first_property_name, void *element) {
            GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
            *elementPtr = &m_audioSink;
        }));
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapper, gstSegmentNew()).WillOnce(Return(&m_segment));
    EXPECT_CALL(*m_gstWrapper, gstSegmentInit(&m_segment, GST_FORMAT_TIME));
    EXPECT_CALL(*m_gstWrapper, gstEventNewSegment(&m_segment)).WillOnce(Return(&m_event));
    EXPECT_CALL(*m_gstWrapper, gstPadSendEvent(_, &m_event)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapper, gstSegmentFree(&m_segment));
    EXPECT_CALL(*m_glibWrapper, gObjectUnref(&m_audioSink));
    task.execute();
    EXPECT_EQ(m_segment.rate, kRate);
    EXPECT_EQ(m_segment.start, GST_CLOCK_TIME_NONE);
    EXPECT_EQ(m_segment.position, GST_CLOCK_TIME_NONE);
    EXPECT_EQ(m_context.playbackRate, kRate);
    EXPECT_EQ(m_context.pendingPlaybackRate, firebolt::rialto::server::kNoPendingPlaybackRate);
}

TEST_F(SetPlaybackRateTest, shouldFailToSetPlaybackRateAmlhalaAudioSink)
{
    GST_STATE(&m_pipeline) = GST_STATE_PLAYING;
    m_context.pipeline = &m_pipeline;
    firebolt::rialto::server::SetPlaybackRate task{m_context, m_gstWrapper, m_glibWrapper, kRate};
    EXPECT_CALL(*m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _))
        .WillOnce(Invoke([&](gpointer object, const gchar *first_property_name, void *element) {
            GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
            *elementPtr = &m_audioSink;
        }));
    EXPECT_CALL(*m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapper, gstSegmentNew()).WillOnce(Return(&m_segment));
    EXPECT_CALL(*m_gstWrapper, gstSegmentInit(&m_segment, GST_FORMAT_TIME));
    EXPECT_CALL(*m_gstWrapper, gstEventNewSegment(&m_segment)).WillOnce(Return(&m_event));
    EXPECT_CALL(*m_gstWrapper, gstPadSendEvent(_, &m_event)).WillOnce(Return(FALSE));
    EXPECT_CALL(*m_gstWrapper, gstSegmentFree(&m_segment));
    EXPECT_CALL(*m_glibWrapper, gObjectUnref(&m_audioSink));
    task.execute();
    EXPECT_EQ(m_segment.rate, kRate);
    EXPECT_EQ(m_segment.start, GST_CLOCK_TIME_NONE);
    EXPECT_EQ(m_segment.position, GST_CLOCK_TIME_NONE);
    EXPECT_EQ(m_context.playbackRate, 1.0);
    EXPECT_EQ(m_context.pendingPlaybackRate, firebolt::rialto::server::kNoPendingPlaybackRate);
}
