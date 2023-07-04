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

#include "tasks/generic/AttachSource.h"
#include "GenericPlayerContext.h"
#include "GlibWrapperMock.h"
#include "GstGenericPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include "RdkGstreamerUtilsWrapperMock.h"
#include <gst/gst.h>
#include <gtest/gtest.h>
#include <string>

using testing::_;
using testing::ElementsAreArray;
using testing::Invoke;
using testing::Return;
using testing::StrEq;
using testing::StrictMock;

class AttachSourceTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context{};
    std::shared_ptr<firebolt::rialto::server::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GlibWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::RdkGstreamerUtilsWrapperMock> m_rdkGstreamerUtilsWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::RdkGstreamerUtilsWrapperMock>>()};
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    GstCaps m_gstCaps1{};
    std::string m_mimeType2{"video/mpeg"};
    GstCaps m_gstCaps2{};
    GstElement m_appSrc{};
    std::string m_vidName{"vidsrc"};
    std::string m_audName{"audsrc"};
    gchar m_capsStr{};
};

TEST_F(AttachSourceTest, shouldAttachAudioSource)
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>("audio/aac", false);
    firebolt::rialto::server::tasks::generic::AttachSource task{m_context,     m_gstWrapper,
                                                                m_glibWrapper, m_rdkGstreamerUtilsWrapper,
                                                                m_gstPlayer,   source};
    EXPECT_CALL(*m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/mpeg"))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("mpegversion"), G_TYPE_INT, 4));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps1)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapper, gFree(&m_capsStr));
    EXPECT_CALL(*m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(m_audName.c_str()))).WillOnce(Return(&m_appSrc));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
    EXPECT_EQ(&m_appSrc, m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).appSrc);
    EXPECT_FALSE(m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).hasDrm);
}

TEST_F(AttachSourceTest, shouldAttachAudioSourceWithChannelsAndRate)
{
    firebolt::rialto::AudioConfig audioConfig{6, 48000, {}};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>("audio/x-eac3", true, audioConfig);
    firebolt::rialto::server::tasks::generic::AttachSource task{m_context,     m_gstWrapper,
                                                                m_glibWrapper, m_rdkGstreamerUtilsWrapper,
                                                                m_gstPlayer,   source};
    EXPECT_CALL(*m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/x-eac3"))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("channels"), G_TYPE_INT, 6));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("rate"), G_TYPE_INT, 48000));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps1)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapper, gFree(&m_capsStr));
    EXPECT_CALL(*m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(m_audName.c_str()))).WillOnce(Return(&m_appSrc));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
    EXPECT_EQ(&m_appSrc, m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).appSrc);
}

TEST_F(AttachSourceTest, shouldAttachOpusWithAudioSpecificConf)
{
    firebolt::rialto::AudioConfig audioConfig{0, 0, {'T', 'E', 'S', 'T'}};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>("audio/x-opus", true, audioConfig);
    firebolt::rialto::server::tasks::generic::AttachSource task{m_context,     m_gstWrapper,
                                                                m_glibWrapper, m_rdkGstreamerUtilsWrapper,
                                                                m_gstPlayer,   source};
    EXPECT_CALL(*m_gstWrapper, gstCodecUtilsOpusCreateCapsFromHeader(arrayMatcher(audioConfig.codecSpecificConfig), 4))
        .WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps1)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapper, gFree(&m_capsStr));
    EXPECT_CALL(*m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(m_audName.c_str()))).WillOnce(Return(&m_appSrc));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
    EXPECT_EQ(&m_appSrc, m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).appSrc);
}

TEST_F(AttachSourceTest, shouldAttachVideoSource)
{
    int width = 1920;
    int height = 1080;
    gpointer memory = nullptr;
    GstBuffer buf;
    std::shared_ptr<firebolt::rialto::CodecData> codecData{std::make_shared<firebolt::rialto::CodecData>()};
    codecData->data = std::vector<std::uint8_t>{'T', 'E', 'S', 'T'};
    codecData->type = firebolt::rialto::CodecDataType::BUFFER;
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264", true, width, height,
                                                                             firebolt::rialto::SegmentAlignment::AU,
                                                                             firebolt::rialto::StreamFormat::AVC,
                                                                             codecData);
    firebolt::rialto::server::tasks::generic::AttachSource task{m_context,     m_gstWrapper,
                                                                m_glibWrapper, m_rdkGstreamerUtilsWrapper,
                                                                m_gstPlayer,   source};
    EXPECT_CALL(*m_gstWrapper, gstCapsNewEmptySimple(StrEq("video/x-h264"))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("width"), G_TYPE_INT, width));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("height"), G_TYPE_INT, height));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("alignment"), _, StrEq("au")));
    EXPECT_CALL(*m_glibWrapper, gMemdup(arrayMatcher(codecData->data), codecData->data.size())).WillOnce(Return(memory));
    EXPECT_CALL(*m_gstWrapper, gstBufferNewWrapped(memory, codecData->data.size())).WillOnce(Return(&buf));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleBufferStub(&m_gstCaps1, StrEq("codec_data"), _, &buf));
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&buf));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("stream-format"), _, StrEq("avc")));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps1)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapper, gFree(&m_capsStr));
    EXPECT_CALL(*m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(m_vidName.c_str()))).WillOnce(Return(&m_appSrc));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_EQ(&m_appSrc, m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).appSrc);
    EXPECT_TRUE(m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).hasDrm);
}

TEST_F(AttachSourceTest, shouldAttachVideoSourceWithStringCodecData)
{
    int width = 1920;
    int height = 1080;
    std::string codecDataStr{"TEST"};
    std::shared_ptr<firebolt::rialto::CodecData> codecData{std::make_shared<firebolt::rialto::CodecData>()};
    codecData->data = std::vector<std::uint8_t>{codecDataStr.begin(), codecDataStr.end()};
    codecData->type = firebolt::rialto::CodecDataType::STRING;
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264", true, width, height,
                                                                             firebolt::rialto::SegmentAlignment::AU,
                                                                             firebolt::rialto::StreamFormat::AVC,
                                                                             codecData);
    firebolt::rialto::server::tasks::generic::AttachSource task{m_context,     m_gstWrapper,
                                                                m_glibWrapper, m_rdkGstreamerUtilsWrapper,
                                                                m_gstPlayer,   source};
    EXPECT_CALL(*m_gstWrapper, gstCapsNewEmptySimple(StrEq("video/x-h264"))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("width"), G_TYPE_INT, width));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("height"), G_TYPE_INT, height));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("alignment"), _, StrEq("au")));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("codec_data"), G_TYPE_STRING,
                                                          StrEq(codecDataStr.c_str())));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("stream-format"), _, StrEq("avc")));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps1)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapper, gFree(&m_capsStr));
    EXPECT_CALL(*m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(m_vidName.c_str()))).WillOnce(Return(&m_appSrc));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_EQ(&m_appSrc, m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).appSrc);
    EXPECT_TRUE(m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).hasDrm);
}

TEST_F(AttachSourceTest, shouldAttachVideoSourceEmptyCodecData)
{
    int size = 0;
    gpointer memory = nullptr;
    GstBuffer buf;
    std::shared_ptr<firebolt::rialto::CodecData> codecData{std::make_shared<firebolt::rialto::CodecData>()};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264", true, size, size,
                                                                             firebolt::rialto::SegmentAlignment::AU,
                                                                             firebolt::rialto::StreamFormat::AVC,
                                                                             codecData);
    firebolt::rialto::server::tasks::generic::AttachSource task{m_context,     m_gstWrapper,
                                                                m_glibWrapper, m_rdkGstreamerUtilsWrapper,
                                                                m_gstPlayer,   source};
    EXPECT_CALL(*m_gstWrapper, gstCapsNewEmptySimple(StrEq("video/x-h264"))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("alignment"), _, StrEq("au")));
    EXPECT_CALL(*m_glibWrapper, gMemdup(arrayMatcher(codecData->data), codecData->data.size())).WillOnce(Return(memory));
    EXPECT_CALL(*m_gstWrapper, gstBufferNewWrapped(memory, codecData->data.size())).WillOnce(Return(&buf));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleBufferStub(&m_gstCaps1, StrEq("codec_data"), _, &buf));
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&buf));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("stream-format"), _, StrEq("avc")));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps1)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapper, gFree(&m_capsStr));
    EXPECT_CALL(*m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(m_vidName.c_str()))).WillOnce(Return(&m_appSrc));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_EQ(&m_appSrc, m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).appSrc);
}

TEST_F(AttachSourceTest, shouldAttachVideoDolbyVisionSource)
{
    gpointer memory = nullptr;
    int32_t dolbyVisionProfile = 5;
    int width = 1920;
    int height = 1080;
    GstBuffer buf;
    std::shared_ptr<firebolt::rialto::CodecData> codecData{std::make_shared<firebolt::rialto::CodecData>()};
    codecData->data = std::vector<std::uint8_t>{'T', 'E', 'S', 'T'};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source = std::make_unique<
        firebolt::rialto::IMediaPipeline::MediaSourceVideoDolbyVision>("video/h265", dolbyVisionProfile, true, width,
                                                                       height, firebolt::rialto::SegmentAlignment::AU,
                                                                       firebolt::rialto::StreamFormat::AVC, codecData);
    firebolt::rialto::server::tasks::generic::AttachSource task{m_context,     m_gstWrapper,
                                                                m_glibWrapper, m_rdkGstreamerUtilsWrapper,
                                                                m_gstPlayer,   source};
    EXPECT_CALL(*m_gstWrapper, gstCapsNewEmptySimple(StrEq("video/x-h265"))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("width"), G_TYPE_INT, width));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("height"), G_TYPE_INT, height));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("alignment"), _, StrEq("au")));
    EXPECT_CALL(*m_glibWrapper, gMemdup(arrayMatcher(codecData->data), codecData->data.size())).WillOnce(Return(memory));
    EXPECT_CALL(*m_gstWrapper, gstBufferNewWrapped(memory, codecData->data.size())).WillOnce(Return(&buf));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleBufferStub(&m_gstCaps1, StrEq("codec_data"), _, &buf));
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&buf));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("stream-format"), _, StrEq("avc")));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleBooleanStub(&m_gstCaps1, StrEq("dovi-stream"), _, true));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleUintStub(&m_gstCaps1, StrEq("dv_profile"), _, dolbyVisionProfile));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps1)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapper, gFree(&m_capsStr));
    EXPECT_CALL(*m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(m_vidName.c_str()))).WillOnce(Return(&m_appSrc));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    task.execute();
    EXPECT_EQ(1, m_context.streamInfo.size());
    EXPECT_NE(m_context.streamInfo.end(), m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_EQ(&m_appSrc, m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).appSrc);
}

TEST_F(AttachSourceTest, shouldSwitchAudioSource)
{
    constexpr gint64 kPosition{1234};
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &m_appSrc);
    m_context.audioSourceRemoved = true;
    gchar oldCapsStr[]{"audio/x-eac3"};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>("audio/aac");
    firebolt::rialto::server::tasks::generic::AttachSource task{m_context,     m_gstWrapper,
                                                                m_glibWrapper, m_rdkGstreamerUtilsWrapper,
                                                                m_gstPlayer,   source};
    EXPECT_CALL(*m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/mpeg"))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("mpegversion"), G_TYPE_INT, 4));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps1)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapper, gFree(&m_capsStr));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc))).WillOnce(Return(&m_gstCaps2));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps2)).WillOnce(Return(oldCapsStr));
    EXPECT_CALL(*m_glibWrapper, gFree(oldCapsStr));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps2));
    EXPECT_CALL(*m_gstWrapper, gstElementQueryPosition(_, GST_FORMAT_TIME, _))
        .WillOnce(Invoke(
            [this](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = kPosition;
                return TRUE;
            }));
    EXPECT_CALL(*m_rdkGstreamerUtilsWrapper,
                performAudioTrackCodecChannelSwitch(&m_context.playbackGroup, _, _, _, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(true));
    EXPECT_CALL(m_gstPlayer, notifyNeedMediaData(true, false));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps1));
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.audioSourceRemoved);
    EXPECT_EQ(m_context.lastAudioSampleTimestamps, kPosition);
}

TEST_F(AttachSourceTest, shouldNotSwitchAudioSourceWhenMimeTypeIsEmpty)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &m_appSrc);
    m_context.audioSourceRemoved = true;
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>("");
    firebolt::rialto::server::tasks::generic::AttachSource task{m_context,     m_gstWrapper,
                                                                m_glibWrapper, m_rdkGstreamerUtilsWrapper,
                                                                m_gstPlayer,   source};
    EXPECT_CALL(*m_gstWrapper, gstCapsNewEmpty()).WillOnce(Return(&m_gstCaps2));
    EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_gstCaps2)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapper, gFree(&m_capsStr));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_gstCaps2));
    task.execute();
}
