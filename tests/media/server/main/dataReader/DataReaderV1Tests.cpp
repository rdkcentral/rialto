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
#include <iostream>

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
constexpr int numOfPlaybacks{1};
constexpr int sessionId{0};
constexpr auto videoMediaSourceType{firebolt::rialto::MediaSourceType::VIDEO};
constexpr auto videoSourceId{static_cast<std::int32_t>(videoMediaSourceType)};
constexpr auto audioMediaSourceType{firebolt::rialto::MediaSourceType::AUDIO};
constexpr auto audioSourceId{static_cast<std::int32_t>(audioMediaSourceType)};
constexpr int64_t timeStamp{4135000000000};
constexpr int64_t duration{90000000000};
constexpr int32_t width{1024};
constexpr int32_t height{768};
constexpr int32_t sampleRate{13};
constexpr int32_t numberOfChannels{4};
constexpr std::uint32_t maxMetadataBytes{2308};
std::vector<uint8_t> videoData{'T', 'E', 'S', 'T', '_', 'A', 'U', 'D', 'I', 'O'};
std::vector<uint8_t> audioData{'T', 'E', 'S', 'T', '_', 'V', 'I', 'D', 'E', 'O'};
std::uint32_t numFrames{1};
const std::unique_ptr<IMediaPipeline::MediaSegment> videoSegment{
    std::make_unique<IMediaPipeline::MediaSegmentVideo>(videoSourceId, timeStamp, duration, width, height)};
const std::unique_ptr<IMediaPipeline::MediaSegment> audioSegment{
    std::make_unique<IMediaPipeline::MediaSegmentAudio>(audioSourceId, timeStamp, duration, sampleRate, numberOfChannels)};
} // namespace

class DataReaderV1Tests : public testing::Test
{
public:
    DataReaderV1Tests() : m_shm{ISharedMemoryBufferFactory::createFactory()->createSharedMemoryBuffer(numOfPlaybacks)}
    {
    }

    virtual void SetUp()
    {
        setenv("RIALTO_METADATA_VERSION", "1", 1);
        videoSegment->setData(videoData.size(), videoData.data());
        audioSegment->setData(audioData.size(), audioData.data());
    }

    virtual void TearDown() { unsetenv("RIALTO_METADATA_VERSION"); }

    void writeVideoData()
    {
        ASSERT_TRUE(m_shm);
        EXPECT_TRUE(m_shm->mapPartition(sessionId));
        std::uint32_t maxMediaBytes = m_shm->getMaxDataLen(sessionId, videoMediaSourceType) - maxMetadataBytes;
        auto metadataOffset = m_shm->getDataOffset(sessionId, videoMediaSourceType);
        auto mediadataOffset = metadataOffset + maxMetadataBytes;
        auto shmInfo = std::make_shared<MediaPlayerShmInfo>(
            MediaPlayerShmInfo{maxMetadataBytes, metadataOffset, mediadataOffset, maxMediaBytes});
        auto *shmBegin{m_shm->getDataPtr(sessionId, videoMediaSourceType)};
        auto mediaFrameWriter = IMediaFrameWriterFactory::getFactory()->createFrameWriter(shmBegin, shmInfo);
        EXPECT_EQ(mediaFrameWriter->writeFrame(videoSegment), AddSegmentStatus::OK);
    }

    void readVideoData()
    {
        ASSERT_TRUE(m_shm);
        std::uint8_t *buffer = m_shm->getBuffer();
        m_sut = std::make_unique<DataReaderV1>(videoMediaSourceType, buffer, 4, numFrames);
        auto result = m_sut->readData();
        ASSERT_EQ(1, result.size());
        IMediaPipeline::MediaSegmentVideo *resultSegment =
            dynamic_cast<IMediaPipeline::MediaSegmentVideo *>(result.front().get());
        ASSERT_NE(nullptr, resultSegment);
        EXPECT_EQ(resultSegment->getType(), videoMediaSourceType);
        EXPECT_EQ(resultSegment->getTimeStamp(), timeStamp);
        EXPECT_EQ(resultSegment->getDuration(), duration);
        EXPECT_EQ(resultSegment->getWidth(), width);
        EXPECT_EQ(resultSegment->getHeight(), height);
        EXPECT_EQ(resultSegment->getDataLength(), videoData.size());
        std::vector<uint8_t> resultData{resultSegment->getData(),
                                        resultSegment->getData() + resultSegment->getDataLength()};
        EXPECT_EQ(resultData, videoData);
    }

    void writeAudioData()
    {
        ASSERT_TRUE(m_shm);
        EXPECT_TRUE(m_shm->mapPartition(sessionId));
        std::uint32_t maxMediaBytes = m_shm->getMaxDataLen(sessionId, audioMediaSourceType) - maxMetadataBytes;
        auto metadataOffset = m_shm->getDataOffset(sessionId, audioMediaSourceType);
        auto mediadataOffset = metadataOffset + maxMetadataBytes;
        auto shmInfo = std::make_shared<MediaPlayerShmInfo>(
            MediaPlayerShmInfo{maxMetadataBytes, metadataOffset, mediadataOffset, maxMediaBytes});
        auto *shmBegin{m_shm->getDataPtr(sessionId, videoMediaSourceType)};
        auto mediaFrameWriter = IMediaFrameWriterFactory::getFactory()->createFrameWriter(shmBegin, shmInfo);
        ASSERT_TRUE(mediaFrameWriter);
        EXPECT_EQ(mediaFrameWriter->writeFrame(audioSegment), AddSegmentStatus::OK);
    }

    void readAudioData()
    {
        ASSERT_TRUE(m_shm);
        std::uint8_t *buffer = m_shm->getBuffer();
        std::uint32_t regionOffset = m_shm->getDataOffset(sessionId, audioMediaSourceType);
        m_sut = std::make_unique<DataReaderV1>(audioMediaSourceType, buffer, regionOffset + 4, numFrames);
        auto result = m_sut->readData();
        ASSERT_EQ(1, result.size());
        IMediaPipeline::MediaSegmentAudio *resultSegment =
            dynamic_cast<IMediaPipeline::MediaSegmentAudio *>(result.front().get());
        ASSERT_NE(nullptr, resultSegment);
        EXPECT_EQ(resultSegment->getType(), audioMediaSourceType);
        EXPECT_EQ(resultSegment->getTimeStamp(), timeStamp);
        EXPECT_EQ(resultSegment->getDuration(), duration);
        EXPECT_EQ(resultSegment->getSampleRate(), sampleRate);
        EXPECT_EQ(resultSegment->getNumberOfChannels(), numberOfChannels);
        EXPECT_EQ(resultSegment->getDataLength(), audioData.size());
        std::vector<uint8_t> resultData{resultSegment->getData(),
                                        resultSegment->getData() + resultSegment->getDataLength()};
        EXPECT_EQ(resultData, audioData);
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
