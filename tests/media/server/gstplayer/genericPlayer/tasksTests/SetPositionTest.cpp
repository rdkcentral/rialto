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

#include "tasks/generic/SetPosition.h"
#include "GenericPlayerContext.h"
#include "GstGenericPlayerClientMock.h"
#include "GstGenericPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::Return;
using testing::StrictMock;

namespace
{
std::int64_t position{12345};
} // namespace

class SetPositionTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context;
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    StrictMock<firebolt::rialto::server::GstGenericPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    GstElement m_pipeline{};
    GstBuffer m_audioBuffer{};
    GstBuffer m_videoBuffer{};
    GstAppSrc m_audioSrc{};
    GstAppSrc m_videoSrc{};

    SetPositionTest()
    {
        m_context.pipeline = &m_pipeline;
        m_context.audioNeedData = true;
        m_context.videoNeedData = true;
        m_context.audioNeedDataPending = true;
        m_context.videoNeedDataPending = true;
        m_context.audioBuffers.emplace_back(&m_audioBuffer);
        m_context.videoBuffers.emplace_back(&m_videoBuffer);
        m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, GST_ELEMENT(&m_audioSrc));
        m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::VIDEO, GST_ELEMENT(&m_videoSrc));
    }
};

TEST_F(SetPositionTest, shouldFailToSetPositionWhenClientIsNull)
{
    firebolt::rialto::server::tasks::generic::SetPosition task{m_context, m_gstPlayer, nullptr, m_gstWrapper, position};
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.videoNeedData);
    EXPECT_TRUE(m_context.audioNeedDataPending);
    EXPECT_TRUE(m_context.videoNeedDataPending);
    EXPECT_FALSE(m_context.audioBuffers.empty());
    EXPECT_FALSE(m_context.videoBuffers.empty());
}

TEST_F(SetPositionTest, shouldFailToSetPositionWhenPipelineIsNull)
{
    m_context.pipeline = nullptr;
    firebolt::rialto::server::tasks::generic::SetPosition task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                               position};
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::SEEKING));
    EXPECT_CALL(m_gstPlayerClient, clearActiveRequestsCache());
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&m_audioBuffer));
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&m_videoBuffer));
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::FAILURE));
    task.execute();
    EXPECT_FALSE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.videoNeedData);
    EXPECT_FALSE(m_context.audioNeedDataPending);
    EXPECT_FALSE(m_context.videoNeedDataPending);
    EXPECT_TRUE(m_context.audioBuffers.empty());
    EXPECT_TRUE(m_context.videoBuffers.empty());
}

TEST_F(SetPositionTest, shouldFailToSetPositionWhenSeekFailed)
{
    firebolt::rialto::server::tasks::generic::SetPosition task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                               position};
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::SEEKING));
    EXPECT_CALL(m_gstPlayerClient, clearActiveRequestsCache());
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&m_audioBuffer));
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&m_videoBuffer));
    EXPECT_CALL(*m_gstWrapper,
                gstElementSeek(&m_pipeline, 1.0, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH),
                               GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        .WillOnce(Return(false));
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::FAILURE));
    task.execute();
    EXPECT_FALSE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.videoNeedData);
    EXPECT_FALSE(m_context.audioNeedDataPending);
    EXPECT_FALSE(m_context.videoNeedDataPending);
    EXPECT_TRUE(m_context.audioBuffers.empty());
    EXPECT_TRUE(m_context.videoBuffers.empty());
}

TEST_F(SetPositionTest, shouldSetPosition)
{
    firebolt::rialto::server::tasks::generic::SetPosition task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                               position};
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::SEEKING));
    EXPECT_CALL(m_gstPlayerClient, clearActiveRequestsCache());
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&m_audioBuffer));
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&m_videoBuffer));
    EXPECT_CALL(*m_gstWrapper,
                gstElementSeek(&m_pipeline, 1.0, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH),
                               GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        .WillOnce(Return(true));
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::FLUSHED));
    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::AUDIO)).WillOnce(Return(true));
    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO)).WillOnce(Return(true));
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.videoNeedData);
    EXPECT_TRUE(m_context.audioNeedDataPending);
    EXPECT_TRUE(m_context.videoNeedDataPending);
    EXPECT_TRUE(m_context.audioBuffers.empty());
    EXPECT_TRUE(m_context.videoBuffers.empty());
}

TEST_F(SetPositionTest, shouldSetPositionWithChangedPlaybackRate)
{
    constexpr double kRate{1.5};
    m_context.playbackRate = kRate;
    firebolt::rialto::server::tasks::generic::SetPosition task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                               position};
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::SEEKING));
    EXPECT_CALL(m_gstPlayerClient, clearActiveRequestsCache());
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&m_audioBuffer));
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&m_videoBuffer));
    EXPECT_CALL(*m_gstWrapper,
                gstElementSeek(&m_pipeline, kRate, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH),
                               GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        .WillOnce(Return(true));
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::FLUSHED));
    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::AUDIO)).WillOnce(Return(true));
    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO)).WillOnce(Return(true));
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.videoNeedData);
    EXPECT_TRUE(m_context.audioNeedDataPending);
    EXPECT_TRUE(m_context.videoNeedDataPending);
    EXPECT_TRUE(m_context.audioBuffers.empty());
    EXPECT_TRUE(m_context.videoBuffers.empty());
}
