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

#include "tasks/generic/AttachSamples.h"
#include "GenericPlayerContext.h"
#include "GstGenericPlayerPrivateMock.h"
#include "IMediaPipeline.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::_;
using testing::Return;
using testing::StrictMock;

namespace
{
constexpr auto audioSourceId{static_cast<std::int32_t>(firebolt::rialto::MediaSourceType::AUDIO)};
constexpr auto videoSourceId{static_cast<std::int32_t>(firebolt::rialto::MediaSourceType::VIDEO)};
constexpr gint64 itHappenedInThePast = 1238450934;
constexpr gint64 itWillHappenInTheFuture = 3823530248;
constexpr int64_t duration{9000000000};
constexpr int32_t sampleRate{13};
constexpr int32_t numberOfChannels{4};
constexpr int32_t width{1024};
constexpr int32_t height{768};
constexpr Fraction frameRate{15, 1};
const std::shared_ptr<std::vector<std::uint8_t>> emptyCodecData{};
const std::shared_ptr<std::vector<std::uint8_t>> codecData{
    std::make_shared<std::vector<std::uint8_t>>(std::vector<std::uint8_t>{1, 2, 3, 4})};

firebolt::rialto::IMediaPipeline::MediaSegmentVector buildAudioSamples()
{
    firebolt::rialto::IMediaPipeline::MediaSegmentVector dataVec;
    dataVec.emplace_back(
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentAudio>(audioSourceId, itHappenedInThePast,
                                                                              duration, sampleRate, numberOfChannels));
    dataVec.emplace_back(
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentAudio>(audioSourceId, itWillHappenInTheFuture,
                                                                              duration, sampleRate, numberOfChannels));
    dataVec.back()->setCodecData(codecData);
    return dataVec;
}

firebolt::rialto::IMediaPipeline::MediaSegmentVector buildVideoSamples()
{
    firebolt::rialto::IMediaPipeline::MediaSegmentVector dataVec;
    dataVec.emplace_back(std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentVideo>(videoSourceId,
                                                                                               itHappenedInThePast,
                                                                                               duration, width, height, frameRate));
    dataVec.emplace_back(std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentVideo>(videoSourceId,
                                                                                               itWillHappenInTheFuture,
                                                                                               duration, width, height, frameRate));
    dataVec.back()->setCodecData(codecData);
    return dataVec;
}
} // namespace

class AttachSamplesTest : public testing::Test
{
public:
    firebolt::rialto::server::GenericPlayerContext m_context;
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    GstBuffer m_gstBuffer{};
};

TEST_F(AttachSamplesTest, shouldAttachAllAudioSamples)
{
    auto samples = buildAudioSamples();
    EXPECT_CALL(m_gstPlayer, createBuffer(_)).Times(2).WillRepeatedly(Return(&m_gstBuffer));
    firebolt::rialto::server::tasks::generic::AttachSamples task{m_context, m_gstPlayer, samples};
    EXPECT_CALL(m_gstPlayer, updateAudioCaps(sampleRate, numberOfChannels, emptyCodecData));
    EXPECT_CALL(m_gstPlayer, updateAudioCaps(sampleRate, numberOfChannels, codecData));
    EXPECT_CALL(m_gstPlayer, attachAudioData()).Times(2);
    EXPECT_CALL(m_gstPlayer, notifyNeedMediaData(true, false));
    task.execute();
    EXPECT_EQ(m_context.audioBuffers.size(), 2);
}

TEST_F(AttachSamplesTest, shouldAttachAllVideoSamples)
{
    auto samples = buildVideoSamples();
    EXPECT_CALL(m_gstPlayer, createBuffer(_)).Times(2).WillRepeatedly(Return(&m_gstBuffer));
    firebolt::rialto::server::tasks::generic::AttachSamples task{m_context, m_gstPlayer, samples};
    EXPECT_CALL(m_gstPlayer, updateVideoCaps(width, height, frameRate, emptyCodecData));
    EXPECT_CALL(m_gstPlayer, updateVideoCaps(width, height, frameRate, codecData));
    EXPECT_CALL(m_gstPlayer, attachVideoData()).Times(2);
    EXPECT_CALL(m_gstPlayer, notifyNeedMediaData(false, true));
    task.execute();
    EXPECT_EQ(m_context.videoBuffers.size(), 2);
}
