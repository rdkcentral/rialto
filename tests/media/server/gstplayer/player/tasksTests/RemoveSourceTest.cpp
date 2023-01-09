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

#include "tasks/RemoveSource.h"
#include "GstPlayerClientMock.h"
#include "GstWrapperMock.h"
#include "PlayerContext.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::Return;
using testing::StrictMock;

class RemoveSourceTest : public testing::Test
{
protected:
    firebolt::rialto::server::PlayerContext m_context;
    StrictMock<firebolt::rialto::server::GstPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    GstElement m_audioSrc{};
    GstElement m_videoSrc{};
    GstEvent m_flushStartEvent{};
    GstEvent m_flushStopEvent{};

    RemoveSourceTest()
    {
        m_context.audioNeedData = true;
        m_context.audioNeedDataPending = true;
        m_context.audioUnderflowEnabled = true;
        m_context.audioSourceRemoved = false;
        m_context.videoNeedData = true;
        m_context.videoNeedDataPending = true;
        m_context.videoUnderflowEnabled = true;
        m_context.videoSourceRemoved = false;
        m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &m_audioSrc);
        m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::VIDEO, &m_videoSrc);
    }
};

TEST_F(RemoveSourceTest, shouldRemoveVideoSourceWithoutFlushing)
{
    m_context.streamInfo.clear();
    constexpr auto kMediaSourceType{firebolt::rialto::MediaSourceType::VIDEO};
    EXPECT_CALL(m_gstPlayerClient, invalidateActiveRequests(kMediaSourceType));
    firebolt::rialto::server::RemoveSource task{m_context, &m_gstPlayerClient, m_gstWrapper, kMediaSourceType};
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.audioNeedDataPending);
    EXPECT_TRUE(m_context.audioUnderflowEnabled);
    EXPECT_FALSE(m_context.audioSourceRemoved);
    EXPECT_FALSE(m_context.videoNeedData);
    EXPECT_FALSE(m_context.videoNeedDataPending);
    EXPECT_FALSE(m_context.videoUnderflowEnabled);
    EXPECT_TRUE(m_context.videoSourceRemoved);
}

TEST_F(RemoveSourceTest, shouldRemoveAudioSourceWithoutFlushing)
{
    m_context.streamInfo.clear();
    constexpr auto kMediaSourceType{firebolt::rialto::MediaSourceType::AUDIO};
    EXPECT_CALL(m_gstPlayerClient, invalidateActiveRequests(kMediaSourceType));
    firebolt::rialto::server::RemoveSource task{m_context, &m_gstPlayerClient, m_gstWrapper, kMediaSourceType};
    task.execute();
    EXPECT_FALSE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.audioNeedDataPending);
    EXPECT_FALSE(m_context.audioUnderflowEnabled);
    EXPECT_TRUE(m_context.audioSourceRemoved);
    EXPECT_TRUE(m_context.videoNeedData);
    EXPECT_TRUE(m_context.videoNeedDataPending);
    EXPECT_TRUE(m_context.videoUnderflowEnabled);
    EXPECT_FALSE(m_context.videoSourceRemoved);
}

TEST_F(RemoveSourceTest, shouldRemoveVideoSource)
{
    constexpr auto kMediaSourceType{firebolt::rialto::MediaSourceType::VIDEO};
    EXPECT_CALL(m_gstPlayerClient, invalidateActiveRequests(kMediaSourceType));
    EXPECT_CALL(*m_gstWrapper, gstEventNewFlushStart()).WillOnce(Return(&m_flushStartEvent));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_videoSrc, &m_flushStartEvent)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapper, gstEventNewFlushStop(FALSE)).WillOnce(Return(&m_flushStopEvent));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_videoSrc, &m_flushStopEvent)).WillOnce(Return(TRUE));
    firebolt::rialto::server::RemoveSource task{m_context, &m_gstPlayerClient, m_gstWrapper, kMediaSourceType};
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.audioNeedDataPending);
    EXPECT_TRUE(m_context.audioUnderflowEnabled);
    EXPECT_FALSE(m_context.audioSourceRemoved);
    EXPECT_FALSE(m_context.videoNeedData);
    EXPECT_FALSE(m_context.videoNeedDataPending);
    EXPECT_FALSE(m_context.videoUnderflowEnabled);
    EXPECT_TRUE(m_context.videoSourceRemoved);
}

TEST_F(RemoveSourceTest, shouldRemoveAudioSource)
{
    constexpr auto kMediaSourceType{firebolt::rialto::MediaSourceType::AUDIO};
    EXPECT_CALL(m_gstPlayerClient, invalidateActiveRequests(kMediaSourceType));
    EXPECT_CALL(*m_gstWrapper, gstEventNewFlushStart()).WillOnce(Return(&m_flushStartEvent));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_audioSrc, &m_flushStartEvent)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapper, gstEventNewFlushStop(FALSE)).WillOnce(Return(&m_flushStopEvent));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_audioSrc, &m_flushStopEvent)).WillOnce(Return(TRUE));
    firebolt::rialto::server::RemoveSource task{m_context, &m_gstPlayerClient, m_gstWrapper, kMediaSourceType};
    task.execute();
    EXPECT_FALSE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.audioNeedDataPending);
    EXPECT_FALSE(m_context.audioUnderflowEnabled);
    EXPECT_TRUE(m_context.audioSourceRemoved);
    EXPECT_TRUE(m_context.videoNeedData);
    EXPECT_TRUE(m_context.videoNeedDataPending);
    EXPECT_TRUE(m_context.videoUnderflowEnabled);
    EXPECT_FALSE(m_context.videoSourceRemoved);
}

TEST_F(RemoveSourceTest, shouldRemoveVideoSourceFlushEventError)
{
    constexpr auto kMediaSourceType{firebolt::rialto::MediaSourceType::VIDEO};
    EXPECT_CALL(m_gstPlayerClient, invalidateActiveRequests(kMediaSourceType));
    EXPECT_CALL(*m_gstWrapper, gstEventNewFlushStart()).WillOnce(Return(&m_flushStartEvent));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_videoSrc, &m_flushStartEvent)).WillOnce(Return(FALSE));
    EXPECT_CALL(*m_gstWrapper, gstEventNewFlushStop(FALSE)).WillOnce(Return(&m_flushStopEvent));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_videoSrc, &m_flushStopEvent)).WillOnce(Return(FALSE));
    firebolt::rialto::server::RemoveSource task{m_context, &m_gstPlayerClient, m_gstWrapper, kMediaSourceType};
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.audioNeedDataPending);
    EXPECT_TRUE(m_context.audioUnderflowEnabled);
    EXPECT_FALSE(m_context.audioSourceRemoved);
    EXPECT_FALSE(m_context.videoNeedData);
    EXPECT_FALSE(m_context.videoNeedDataPending);
    EXPECT_FALSE(m_context.videoUnderflowEnabled);
    EXPECT_TRUE(m_context.videoSourceRemoved);
}
