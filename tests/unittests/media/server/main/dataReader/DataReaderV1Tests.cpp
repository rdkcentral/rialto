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

#include "DataReaderV1.h"
#include "IMediaFrameWriter.h"
#include "ISharedMemoryBuffer.h"
#include <cstdlib>
#include <gtest/gtest.h>

using firebolt::rialto::AddSegmentStatus;
using firebolt::rialto::IMediaPipeline;
using firebolt::rialto::MediaPlayerShmInfo;
using firebolt::rialto::common::IMediaFrameWriter;
using firebolt::rialto::common::IMediaFrameWriterFactory;
using firebolt::rialto::server::DataReaderV1;
using firebolt::rialto::server::ISharedMemoryBuffer;
using firebolt::rialto::server::ISharedMemoryBufferFactory;

namespace
{
constexpr int kNumOfPlaybacks{1};
constexpr int kNumOfWebAudioPlayers{2};
constexpr int kSessionId{0};
constexpr auto kVideoMediaSourceType{firebolt::rialto::MediaSourceType::VIDEO};
constexpr auto kVideoSourceId{static_cast<std::int32_t>(kVideoMediaSourceType)};
constexpr auto kAudioMediaSourceType{firebolt::rialto::MediaSourceType::AUDIO};
constexpr auto kAudioSourceId{static_cast<std::int32_t>(kAudioMediaSourceType)};
constexpr int64_t kTimeStamp{4135000000000};
constexpr int64_t kDuration{90000000000};
constexpr int32_t kWidth{1024};
constexpr int32_t kHeight{768};
constexpr int32_t kSampleRate{13};
constexpr int32_t kNumberOfChannels{4};
constexpr std::uint32_t kMaxMetadataBytes{2308};
std::vector<uint8_t> kVideoData{'T', 'E', 'S', 'T', '_', 'A', 'U', 'D', 'I', 'O'};
std::vector<uint8_t> kAudioData{'T', 'E', 'S', 'T', '_', 'V', 'I', 'D', 'E', 'O'};
constexpr std::uint32_t kNumFrames{1};
const std::unique_ptr<IMediaPipeline::MediaSegment> kVideoSegment{
    std::make_unique<IMediaPipeline::MediaSegmentVideo>(kVideoSourceId, kTimeStamp, kDuration, kWidth, kHeight)};
const std::unique_ptr<IMediaPipeline::MediaSegment> kAudioSegment{
    std::make_unique<IMediaPipeline::MediaSegmentAudio>(kAudioSourceId, kTimeStamp, kDuration, kSampleRate,
                                                        kNumberOfChannels)};
} // namespace

class DataReaderV1Tests : public testing::Test
{
public:
    DataReaderV1Tests()
        : m_shm{ISharedMemoryBufferFactory::createFactory()->createSharedMemoryBuffer(kNumOfPlaybacks,
                                                                                      kNumOfWebAudioPlayers)}
    {
    }

    virtual void SetUp()
    {
        setenv("RIALTO_METADATA_VERSION", "1", 1);
        kVideoSegment->setData(kVideoData.size(), kVideoData.data());
        kAudioSegment->setData(kAudioData.size(), kAudioData.data());
    }

    virtual void TearDown() { unsetenv("RIALTO_METADATA_VERSION"); }

    void writeVideoData()
    {
        ASSERT_TRUE(m_shm);
        EXPECT_TRUE(m_shm->mapPartition(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSessionId));
        std::uint32_t maxMediaBytes =
            m_shm->getMaxDataLen(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSessionId, kVideoMediaSourceType) -
            kMaxMetadataBytes;
        auto metadataOffset =
            m_shm->getDataOffset(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSessionId, kVideoMediaSourceType);
        auto mediadataOffset = metadataOffset + kMaxMetadataBytes;
        auto shmInfo = std::make_shared<MediaPlayerShmInfo>(
            MediaPlayerShmInfo{kMaxMetadataBytes, metadataOffset, mediadataOffset, maxMediaBytes});
        auto *shmBegin{
            m_shm->getDataPtr(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSessionId, kVideoMediaSourceType)};
        auto mediaFrameWriter = IMediaFrameWriterFactory::getFactory()->createFrameWriter(shmBegin, shmInfo);
        EXPECT_EQ(mediaFrameWriter->writeFrame(kVideoSegment), AddSegmentStatus::OK);
    }

    void readVideoData()
    {
        ASSERT_TRUE(m_shm);
        std::uint8_t *buffer = m_shm->getBuffer();
        m_sut = std::make_unique<DataReaderV1>(kVideoMediaSourceType, buffer, 4, kNumFrames);
        auto result = m_sut->readData();
        ASSERT_EQ(1, result.size());
        IMediaPipeline::MediaSegmentVideo *resultSegment =
            dynamic_cast<IMediaPipeline::MediaSegmentVideo *>(result.front().get());
        ASSERT_NE(nullptr, resultSegment);
        EXPECT_EQ(resultSegment->getType(), kVideoMediaSourceType);
        EXPECT_EQ(resultSegment->getTimeStamp(), kTimeStamp);
        EXPECT_EQ(resultSegment->getDuration(), kDuration);
        EXPECT_EQ(resultSegment->getWidth(), kWidth);
        EXPECT_EQ(resultSegment->getHeight(), kHeight);
        EXPECT_EQ(resultSegment->getDataLength(), kVideoData.size());
        std::vector<uint8_t> resultData{resultSegment->getData(),
                                        resultSegment->getData() + resultSegment->getDataLength()};
        EXPECT_EQ(resultData, kVideoData);
    }

    void writeAudioData()
    {
        ASSERT_TRUE(m_shm);
        EXPECT_TRUE(m_shm->mapPartition(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSessionId));
        std::uint32_t maxMediaBytes =
            m_shm->getMaxDataLen(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSessionId, kAudioMediaSourceType) -
            kMaxMetadataBytes;
        auto metadataOffset =
            m_shm->getDataOffset(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSessionId, kAudioMediaSourceType);
        auto mediadataOffset = metadataOffset + kMaxMetadataBytes;
        auto shmInfo = std::make_shared<MediaPlayerShmInfo>(
            MediaPlayerShmInfo{kMaxMetadataBytes, metadataOffset, mediadataOffset, maxMediaBytes});
        auto *shmBegin{
            m_shm->getDataPtr(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSessionId, kVideoMediaSourceType)};
        auto mediaFrameWriter = IMediaFrameWriterFactory::getFactory()->createFrameWriter(shmBegin, shmInfo);
        ASSERT_TRUE(mediaFrameWriter);
        EXPECT_EQ(mediaFrameWriter->writeFrame(kAudioSegment), AddSegmentStatus::OK);
    }

    void readAudioData()
    {
        ASSERT_TRUE(m_shm);
        std::uint8_t *buffer = m_shm->getBuffer();
        std::uint32_t regionOffset =
            m_shm->getDataOffset(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSessionId, kAudioMediaSourceType);
        m_sut = std::make_unique<DataReaderV1>(kAudioMediaSourceType, buffer, regionOffset + 4, kNumFrames);
        auto result = m_sut->readData();
        ASSERT_EQ(1, result.size());
        IMediaPipeline::MediaSegmentAudio *resultSegment =
            dynamic_cast<IMediaPipeline::MediaSegmentAudio *>(result.front().get());
        ASSERT_NE(nullptr, resultSegment);
        EXPECT_EQ(resultSegment->getType(), kAudioMediaSourceType);
        EXPECT_EQ(resultSegment->getTimeStamp(), kTimeStamp);
        EXPECT_EQ(resultSegment->getDuration(), kDuration);
        EXPECT_EQ(resultSegment->getSampleRate(), kSampleRate);
        EXPECT_EQ(resultSegment->getNumberOfChannels(), kNumberOfChannels);
        EXPECT_EQ(resultSegment->getDataLength(), kAudioData.size());
        std::vector<uint8_t> resultData{resultSegment->getData(),
                                        resultSegment->getData() + resultSegment->getDataLength()};
        EXPECT_EQ(resultData, kAudioData);
    }

private:
    std::shared_ptr<ISharedMemoryBuffer> m_shm;
    std::unique_ptr<DataReaderV1> m_sut;
};

TEST_F(DataReaderV1Tests, shouldReadVideoData)
{
    writeVideoData();
    readVideoData();
}

TEST_F(DataReaderV1Tests, shouldReadAudioData)
{
    writeAudioData();
    readAudioData();
}
