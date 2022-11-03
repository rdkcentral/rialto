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

#include "tasks/AttachSource.h"
#include "GlibWrapperMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include "PlayerContext.h"
#include <gst/gst.h>
#include <gtest/gtest.h>
#include <string>

using testing::_;
using testing::Return;
using testing::StrictMock;

struct AttachSourceTest : public testing::Test
{
    firebolt::rialto::server::PlayerContext m_context{};
    std::shared_ptr<firebolt::rialto::server::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GlibWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    GstCaps m_gstCaps1{};
    std::string m_capsStr2{"caps2"};
    GstCaps m_gstCaps2{};
    GstElement m_appSrc{};
    std::string m_vidName{"vidsrc"};
    std::string m_audName{"audsrc"};
};

TEST_F(AttachSourceTest, shouldNotAttachUnknownSource)
{
    firebolt::rialto::server::Source source{firebolt::rialto::MediaSourceType::UNKNOWN, &m_gstCaps1};
    firebolt::rialto::server::AttachSource task{m_context, m_gstWrapper, m_glibWrapper, source};
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    task.execute();
    EXPECT_EQ(0, m_context.streamInfo.size());
}

TEST_F(AttachSourceTest, shouldAttachAudioSource)
{
    firebolt::rialto::server::Source source{firebolt::rialto::MediaSourceType::AUDIO, &m_gstCaps1};
    firebolt::rialto::server::AttachSource task{m_context, m_gstWrapper, m_glibWrapper, source};
    EXPECT_CALL(*m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(m_audName.c_str()))).WillOnce(Return(&m_appSrc));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
    EXPECT_EQ(&m_appSrc, m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO));
}

TEST_F(AttachSourceTest, shouldAttachVideoSource)
{
    firebolt::rialto::server::Source source{firebolt::rialto::MediaSourceType::VIDEO, &m_gstCaps1};
    firebolt::rialto::server::AttachSource task{m_context, m_gstWrapper, m_glibWrapper, source};
    EXPECT_CALL(*m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(m_vidName.c_str()))).WillOnce(Return(&m_appSrc));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_EQ(&m_appSrc, m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO));
}

TEST_F(AttachSourceTest, shouldUpdateEmptyCapsInAudioSource)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &m_appSrc);
    firebolt::rialto::server::Source source{firebolt::rialto::MediaSourceType::AUDIO, &m_gstCaps2};
    firebolt::rialto::server::AttachSource task{m_context, m_gstWrapper, m_glibWrapper, source};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps2))
        .WillOnce(Return(const_cast<gchar *>(reinterpret_cast<const gchar *>(m_capsStr2.c_str()))));
    EXPECT_CALL(*m_glibWrapper, gFree(reinterpret_cast<gpointer *>(&m_capsStr2[0])));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps2));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps2));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
}

TEST_F(AttachSourceTest, shouldUpdateExistingCapsInAudioSource)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &m_appSrc);
    firebolt::rialto::server::Source source{firebolt::rialto::MediaSourceType::AUDIO, &m_gstCaps2};
    firebolt::rialto::server::AttachSource task{m_context, m_gstWrapper, m_glibWrapper, source};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsIsEqual(&m_gstCaps1, &m_gstCaps2)).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps2))
        .WillOnce(Return(const_cast<gchar *>(reinterpret_cast<const gchar *>(m_capsStr2.c_str()))));
    EXPECT_CALL(*m_glibWrapper, gFree(reinterpret_cast<gpointer *>(&m_capsStr2[0])));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps2));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps2));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
}

TEST_F(AttachSourceTest, shouldUpdateEmptyCapsInVideoSource)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::VIDEO, &m_appSrc);
    firebolt::rialto::server::Source source{firebolt::rialto::MediaSourceType::VIDEO, &m_gstCaps2};
    firebolt::rialto::server::AttachSource task{m_context, m_gstWrapper, m_glibWrapper, source};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps2))
        .WillOnce(Return(const_cast<gchar *>(reinterpret_cast<const gchar *>(m_capsStr2.c_str()))));
    EXPECT_CALL(*m_glibWrapper, gFree(reinterpret_cast<gpointer *>(&m_capsStr2[0])));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps2));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps2));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
}

TEST_F(AttachSourceTest, shouldUpdateExistingCapsInVideoSource)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::VIDEO, &m_appSrc);
    firebolt::rialto::server::Source source{firebolt::rialto::MediaSourceType::VIDEO, &m_gstCaps2};
    firebolt::rialto::server::AttachSource task{m_context, m_gstWrapper, m_glibWrapper, source};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsIsEqual(&m_gstCaps1, &m_gstCaps2)).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps2))
        .WillOnce(Return(const_cast<gchar *>(reinterpret_cast<const gchar *>(m_capsStr2.c_str()))));
    EXPECT_CALL(*m_glibWrapper, gFree(reinterpret_cast<gpointer *>(&m_capsStr2[0])));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps2));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps2));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
}

TEST_F(AttachSourceTest, shouldNotUpdateAudioSource)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &m_appSrc);
    firebolt::rialto::server::Source source{firebolt::rialto::MediaSourceType::AUDIO, &m_gstCaps2};
    firebolt::rialto::server::AttachSource task{m_context, m_gstWrapper, m_glibWrapper, source};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsIsEqual(&m_gstCaps1, &m_gstCaps2)).WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps2));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
}

TEST_F(AttachSourceTest, shouldNotUpdateVideoSource)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::VIDEO, &m_appSrc);
    firebolt::rialto::server::Source source{firebolt::rialto::MediaSourceType::VIDEO, &m_gstCaps2};
    firebolt::rialto::server::AttachSource task{m_context, m_gstWrapper, m_glibWrapper, source};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsIsEqual(&m_gstCaps1, &m_gstCaps2)).WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps2));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
}
