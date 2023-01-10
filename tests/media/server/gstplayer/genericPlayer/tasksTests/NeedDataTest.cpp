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

#include "tasks/generic/NeedData.h"
#include "GenericPlayerContext.h"
#include "GstGenericPlayerClientMock.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::Return;
using testing::StrictMock;

class NeedDataTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context{};
    StrictMock<firebolt::rialto::server::GstGenericPlayerClientMock> m_gstPlayerClient;
    GstAppSrc m_audioSrc{};
    GstAppSrc m_videoSrc{};

    void setupAppSource()
    {
        m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, GST_ELEMENT(&m_audioSrc));
        m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::VIDEO, GST_ELEMENT(&m_videoSrc));
    }
};

TEST_F(NeedDataTest, shouldDoNothingWhenAudioAppSourceIsNotPresent)
{
    firebolt::rialto::server::NeedData task{m_context, &m_gstPlayerClient, &m_audioSrc};
    task.execute();
}

TEST_F(NeedDataTest, shouldDoNothingWhenVideoAppSourceIsNotPresent)
{
    firebolt::rialto::server::NeedData task{m_context, &m_gstPlayerClient, &m_videoSrc};
    task.execute();
}

TEST_F(NeedDataTest, shouldDoNothingForUnknownAppSource)
{
    GstAppSrc unknownSrc{};
    setupAppSource();
    firebolt::rialto::server::NeedData task{m_context, &m_gstPlayerClient, &unknownSrc};
    task.execute();
}

TEST_F(NeedDataTest, shouldNotifyNeedAudioData)
{
    setupAppSource();
    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::AUDIO)).WillOnce(Return(true));
    firebolt::rialto::server::NeedData task{m_context, &m_gstPlayerClient, &m_audioSrc};
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.audioNeedDataPending);
    EXPECT_FALSE(m_context.videoNeedData);
    EXPECT_FALSE(m_context.videoNeedDataPending);
}

TEST_F(NeedDataTest, shouldFailToNotifyNeedAudioData)
{
    setupAppSource();
    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::AUDIO)).WillOnce(Return(false));
    firebolt::rialto::server::NeedData task{m_context, &m_gstPlayerClient, &m_audioSrc};
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.audioNeedDataPending);
    EXPECT_FALSE(m_context.videoNeedData);
    EXPECT_FALSE(m_context.videoNeedDataPending);
}

TEST_F(NeedDataTest, shouldSkipToNotifyNeedAudioData)
{
    setupAppSource();
    m_context.audioNeedDataPending = true;
    firebolt::rialto::server::NeedData task{m_context, &m_gstPlayerClient, &m_audioSrc};
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.audioNeedDataPending);
    EXPECT_FALSE(m_context.videoNeedData);
    EXPECT_FALSE(m_context.videoNeedDataPending);
}

TEST_F(NeedDataTest, shouldNotifyNeedVideoData)
{
    setupAppSource();
    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO)).WillOnce(Return(true));
    firebolt::rialto::server::NeedData task{m_context, &m_gstPlayerClient, &m_videoSrc};
    task.execute();
    EXPECT_FALSE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.audioNeedDataPending);
    EXPECT_TRUE(m_context.videoNeedData);
    EXPECT_TRUE(m_context.videoNeedDataPending);
}

TEST_F(NeedDataTest, shouldFailToNotifyNeedVideoData)
{
    setupAppSource();
    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO)).WillOnce(Return(false));
    firebolt::rialto::server::NeedData task{m_context, &m_gstPlayerClient, &m_videoSrc};
    task.execute();
    EXPECT_FALSE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.audioNeedDataPending);
    EXPECT_TRUE(m_context.videoNeedData);
    EXPECT_FALSE(m_context.videoNeedDataPending);
}

TEST_F(NeedDataTest, shouldSkipToNotifyNeedVideoData)
{
    setupAppSource();
    m_context.videoNeedDataPending = true;
    firebolt::rialto::server::NeedData task{m_context, &m_gstPlayerClient, &m_videoSrc};
    task.execute();
    EXPECT_FALSE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.audioNeedDataPending);
    EXPECT_TRUE(m_context.videoNeedData);
    EXPECT_TRUE(m_context.videoNeedDataPending);
}
