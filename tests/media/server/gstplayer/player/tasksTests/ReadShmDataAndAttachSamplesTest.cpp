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

#include "tasks/ReadShmDataAndAttachSamples.h"
#include "DataReaderMock.h"
#include "GstPlayerPrivateMock.h"
#include "IMediaPipeline.h"
#include "PlayerContext.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::_;
using testing::ByMove;
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

firebolt::rialto::IMediaPipeline::MediaSegmentVector buildAudioSamples()
{
    firebolt::rialto::IMediaPipeline::MediaSegmentVector dataVec;
    dataVec.emplace_back(
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentAudio>(audioSourceId, itHappenedInThePast,
                                                                              duration, sampleRate, numberOfChannels));
    dataVec.emplace_back(
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentAudio>(audioSourceId, itWillHappenInTheFuture,
                                                                              duration, sampleRate, numberOfChannels));
    return dataVec;
}

firebolt::rialto::IMediaPipeline::MediaSegmentVector buildVideoSamples()
{
    firebolt::rialto::IMediaPipeline::MediaSegmentVector dataVec;
    dataVec.emplace_back(std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentVideo>(videoSourceId,
                                                                                               itHappenedInThePast,
                                                                                               duration, width, height));
    dataVec.emplace_back(std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentVideo>(videoSourceId,
                                                                                               itWillHappenInTheFuture,
                                                                                               duration, width, height));
    return dataVec;
}
} // namespace

class ReadShmDataAndAttachSamplesTest : public testing::Test
{
protected:
    firebolt::rialto::server::PlayerContext m_context{};
    StrictMock<firebolt::rialto::server::GstPlayerPrivateMock> m_gstPlayer;
    std::shared_ptr<StrictMock<firebolt::rialto::server::DataReaderMock>> m_dataReader{
        std::make_shared<StrictMock<firebolt::rialto::server::DataReaderMock>>()};
    GstBuffer m_gstBuffer{};
};

TEST_F(ReadShmDataAndAttachSamplesTest, shouldAttachAllAudioSamples)
{
    firebolt::rialto::IMediaPipeline::MediaSegmentVector dataVec = buildAudioSamples();
    EXPECT_CALL(*m_dataReader, readData()).WillOnce(Return(ByMove(std::move(dataVec))));
    EXPECT_CALL(m_gstPlayer, createDecryptedBuffer(_)).Times(2).WillRepeatedly(Return(&m_gstBuffer));
    EXPECT_CALL(m_gstPlayer, updateAudioCaps(sampleRate, numberOfChannels)).Times(2);
    EXPECT_CALL(m_gstPlayer, attachAudioData()).Times(2);
    EXPECT_CALL(m_gstPlayer, notifyNeedMediaData(true, false));
    firebolt::rialto::server::ReadShmDataAndAttachSamples task{m_context, m_gstPlayer, m_dataReader};
    task.execute();
    EXPECT_EQ(m_context.audioBuffers.size(), 2);
}

TEST_F(ReadShmDataAndAttachSamplesTest, shouldAttachAllVideoSamples)
{
    firebolt::rialto::IMediaPipeline::MediaSegmentVector dataVec = buildVideoSamples();
    EXPECT_CALL(*m_dataReader, readData()).WillOnce(Return(ByMove(std::move(dataVec))));
    EXPECT_CALL(m_gstPlayer, createDecryptedBuffer(_)).Times(2).WillRepeatedly(Return(&m_gstBuffer));
    EXPECT_CALL(m_gstPlayer, updateVideoCaps(width, height)).Times(2);
    EXPECT_CALL(m_gstPlayer, attachVideoData()).Times(2);
    EXPECT_CALL(m_gstPlayer, notifyNeedMediaData(false, true));
    firebolt::rialto::server::ReadShmDataAndAttachSamples task{m_context, m_gstPlayer, m_dataReader};
    task.execute();
    EXPECT_EQ(m_context.videoBuffers.size(), 2);
}
