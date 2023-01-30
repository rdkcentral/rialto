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

#include "tasks/generic/EnoughData.h"
#include "GenericPlayerContext.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

class EnoughDataTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context{};
    GstAppSrc m_audioSrc{};
    GstAppSrc m_videoSrc{};

    EnoughDataTest()
    {
        m_context.audioNeedData = true;
        m_context.videoNeedData = true;
    }
};

TEST_F(EnoughDataTest, shouldDoNothingWhenAudioAppSourceIsNotPresent)
{
    firebolt::rialto::server::generic::EnoughData task{m_context, &m_audioSrc};
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.videoNeedData);
}

TEST_F(EnoughDataTest, shouldDoNothingWhenVideoAppSourceIsNotPresent)
{
    firebolt::rialto::server::generic::EnoughData task{m_context, &m_videoSrc};
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.videoNeedData);
}

TEST_F(EnoughDataTest, shouldMarkEnoughAudioData)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, GST_ELEMENT(&m_audioSrc));
    firebolt::rialto::server::generic::EnoughData task{m_context, &m_audioSrc};
    task.execute();
    EXPECT_FALSE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.videoNeedData);
}

TEST_F(EnoughDataTest, shouldMarkEnoughVideoData)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::VIDEO, GST_ELEMENT(&m_videoSrc));
    firebolt::rialto::server::generic::EnoughData task{m_context, &m_videoSrc};
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.videoNeedData);
}
