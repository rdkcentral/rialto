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

#include "tasks/generic/UpdatePlaybackGroup.h"
#include "GenericPlayerContext.h"
#include "GlibWrapperMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include <gst/gst.h>
#include <gtest/gtest.h>
#include <memory>

using testing::Return;
using testing::StrictMock;

class UpdatePlaybackGroupTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context{};
    std::shared_ptr<firebolt::rialto::server::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GlibWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    GstElement m_typefind{};
    GstCaps m_caps{};
};

TEST_F(UpdatePlaybackGroupTest, shouldDoNothingWhenCapsAreNull)
{
    firebolt::rialto::server::generic::UpdatePlaybackGroup task{m_context, m_gstWrapper, m_glibWrapper, &m_typefind,
                                                                nullptr};
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecodeBin, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
}

TEST_F(UpdatePlaybackGroupTest, shouldDoNothingWhenCapsStrIsNull)
{
    firebolt::rialto::server::generic::UpdatePlaybackGroup task{m_context, m_gstWrapper, m_glibWrapper, &m_typefind,
                                                                &m_caps};
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_caps)).WillOnce(Return(nullptr));
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecodeBin, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
}

TEST_F(UpdatePlaybackGroupTest, shouldDoNothingForVideoCaps)
{
    gchar capsStr[]{"video/x-h264"};
    firebolt::rialto::server::generic::UpdatePlaybackGroup task{m_context, m_gstWrapper, m_glibWrapper, &m_typefind,
                                                                &m_caps};
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_caps)).WillOnce(Return(capsStr));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(capsStr, CharStrMatcher("audio/"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gFree(capsStr));
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecodeBin, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
}

TEST_F(UpdatePlaybackGroupTest, shouldDoNothingWhenTypefindParentIsNull)
{
    gchar capsStr[]{"audio/mp4"};
    firebolt::rialto::server::generic::UpdatePlaybackGroup task{m_context, m_gstWrapper, m_glibWrapper, &m_typefind,
                                                                &m_caps};
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_caps)).WillOnce(Return(capsStr));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(capsStr, CharStrMatcher("audio/"))).WillOnce(Return(capsStr));
    EXPECT_CALL(*m_gstWrapper, gstElementGetParent(&m_typefind)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gFree(capsStr));
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecodeBin, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
}

TEST_F(UpdatePlaybackGroupTest, shouldDoNothingWhenElementOtherThanDecodebin)
{
    gchar capsStr[]{"audio/mp4"};
    gchar elementName[]{"sink"};
    GstElement typefindParent{};
    firebolt::rialto::server::generic::UpdatePlaybackGroup task{m_context, m_gstWrapper, m_glibWrapper, &m_typefind,
                                                                &m_caps};
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_caps)).WillOnce(Return(capsStr));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(capsStr, CharStrMatcher("audio/"))).WillOnce(Return(capsStr));
    EXPECT_CALL(*m_gstWrapper, gstElementGetParent(&m_typefind)).WillOnce(Return(GST_OBJECT(&typefindParent)));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&typefindParent)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("decodebin"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    EXPECT_CALL(*m_glibWrapper, gFree(capsStr));
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecodeBin, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
}

TEST_F(UpdatePlaybackGroupTest, shouldSuccessfullyFindTypefindAndParent)
{
    gchar capsStr[]{"audio/mp4"};
    gchar elementName[]{"decodebin"};
    gchar typefindName[]{"typefind"};
    GstElement typefindParent{};
    firebolt::rialto::server::generic::UpdatePlaybackGroup task{m_context, m_gstWrapper, m_glibWrapper, &m_typefind,
                                                                &m_caps};
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_caps)).WillOnce(Return(capsStr));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(capsStr, CharStrMatcher("audio/"))).WillOnce(Return(capsStr));
    EXPECT_CALL(*m_gstWrapper, gstElementGetParent(&m_typefind)).WillOnce(Return(GST_OBJECT(&typefindParent)));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&typefindParent)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("decodebin"))).WillOnce(Return(elementName));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_typefind)).WillOnce(Return(typefindName));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    EXPECT_CALL(*m_glibWrapper, gFree(capsStr));
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecodeBin, &typefindParent);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, &m_typefind);
}
