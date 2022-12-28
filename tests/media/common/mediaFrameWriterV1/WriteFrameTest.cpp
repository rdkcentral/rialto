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

#include "MediaFrameWriterV1.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::common;

const uint32_t MAX_MEDIA_BYTES = 100;
const uint32_t MAX_METADATA_BYTES = VERSION_SIZE_BYTES + 24 * METADATA_V1_SIZE_PER_FRAME_BYTES;

class RialtoPlayerCommonWriteFrameV1Test : public ::testing::Test
{
protected:
    std::unique_ptr<IMediaFrameWriter> m_mediaFrameWriter;

    // Create a buffer that can hold 24 frames of metadata amd 100bytes of media data
    uint8_t m_shmBuffer[MAX_METADATA_BYTES + MAX_MEDIA_BYTES] = {0};
    IMediaPipeline::MediaSegmentVector m_dataVec;
    int32_t m_sourceId = 1;
    std::shared_ptr<ShmInfo> m_shmInfo;

    virtual void SetUp()
    {
        // Init shm info
        m_shmInfo = std::make_shared<ShmInfo>();
        m_shmInfo->maxMetadataBytes = MAX_METADATA_BYTES;
        m_shmInfo->metadataOffset = 0;
        m_shmInfo->mediaDataOffset = MAX_METADATA_BYTES;
        m_shmInfo->maxMediaBytes = MAX_MEDIA_BYTES;
    }

    virtual void TearDown()
    {
        DeleteFrames();

        m_mediaFrameWriter.reset();
    }

    void AddAudioFrame(int64_t duration, int64_t timestamp, int32_t sampleRate, int32_t numberOfChannels,
                       uint32_t dataSize)
    {
        m_dataVec.push_back(std::make_unique<IMediaPipeline::MediaSegmentAudio>(m_sourceId, timestamp, duration,
                                                                                sampleRate, numberOfChannels));

        m_dataVec.back()->setData(dataSize, new uint8_t[dataSize]());

        m_dataVec.back()->setExtraData({1, 2});
    }

    void AddVideoFrame(int64_t duration, int64_t timestamp, int32_t width, int32_t height, uint32_t dataSize)
    {
        m_dataVec.push_back(
            std::make_unique<IMediaPipeline::MediaSegmentVideo>(m_sourceId, timestamp, duration, width, height));

        m_dataVec.back()->setData(dataSize, new uint8_t[dataSize]());

        m_dataVec.back()->setExtraData({1, 2});
    }

    void DeleteFrames()
    {
        for (auto it = m_dataVec.begin(); it != m_dataVec.end(); it++)
        {
            delete[](*it)->getData();
        }
        m_dataVec.clear();
    }

    void CheckSharedBuffer(MediaSourceType sourceType, const std::shared_ptr<ShmInfo> &shmInfo)
    {
        uint8_t *metadataOffsetPtr = m_shmBuffer + shmInfo->metadataOffset;
        uint32_t dataOffset = shmInfo->mediaDataOffset;

        // Ignore version bytes
        metadataOffsetPtr += VERSION_SIZE_BYTES;

        for (auto it = m_dataVec.begin(); it != m_dataVec.end(); it++)
        {
            // Generic metadata
            EXPECT_EQ(readLEUint32(metadataOffsetPtr), dataOffset);
            metadataOffsetPtr += 4U;
            EXPECT_EQ(readLEUint32(metadataOffsetPtr), (*it)->getDataLength());
            metadataOffsetPtr += 4U;
            EXPECT_EQ(readLEInt64(metadataOffsetPtr), (*it)->getTimeStamp());
            metadataOffsetPtr += 8U;
            EXPECT_EQ(readLEInt64(metadataOffsetPtr), (*it)->getDuration());
            metadataOffsetPtr += 8U;
            EXPECT_EQ(readLEUint32(metadataOffsetPtr), static_cast<uint32_t>((*it)->getId()));
            metadataOffsetPtr += 4U;
            EXPECT_EQ(readLEUint32(metadataOffsetPtr), static_cast<uint32_t>((*it)->getExtraData().size()));
            metadataOffsetPtr += 4U;

            // Extra data
            EXPECT_EQ(memcmp(const_cast<uint8_t *>(&((*it)->getExtraData()[0])), metadataOffsetPtr,
                             (*it)->getExtraData().size()),
                      0);
            metadataOffsetPtr += (*it)->getExtraData().size();
            uint8_t zeroedMem[MAX_EXTRA_DATA_SIZE] = {0};
            EXPECT_EQ(memcmp(zeroedMem, metadataOffsetPtr, MAX_EXTRA_DATA_SIZE - (*it)->getExtraData().size()), 0);
            metadataOffsetPtr += MAX_EXTRA_DATA_SIZE - (*it)->getExtraData().size();

            // Encrypted generic metadata
            metadataOffsetPtr += MediaFrameWriterV1::m_kEncryptionMetadataSizeBytes;

            if (MediaSourceType::AUDIO == sourceType)
            {
                // Audio metadata
                IMediaPipeline::MediaSegmentAudio &audioSegment =
                    dynamic_cast<IMediaPipeline::MediaSegmentAudio &>(*(*it));
                EXPECT_EQ(readLEUint32(metadataOffsetPtr), static_cast<uint32_t>(audioSegment.getSampleRate()));
                metadataOffsetPtr += 4U;
                EXPECT_EQ(readLEUint32(metadataOffsetPtr), static_cast<uint32_t>(audioSegment.getNumberOfChannels()));
                metadataOffsetPtr += 4U;
            }
            else
            {
                // Video metadata
                IMediaPipeline::MediaSegmentVideo &videoSegment =
                    dynamic_cast<IMediaPipeline::MediaSegmentVideo &>(*(*it));
                EXPECT_EQ(readLEUint32(metadataOffsetPtr), static_cast<uint32_t>(videoSegment.getWidth()));
                metadataOffsetPtr += 4U;
                EXPECT_EQ(readLEUint32(metadataOffsetPtr), static_cast<uint32_t>(videoSegment.getHeight()));
                metadataOffsetPtr += 4U;
            }

            // Data
            EXPECT_EQ(memcmp((*it)->getData(), m_shmBuffer + dataOffset, (*it)->getDataLength()), 0);
            dataOffset += (*it)->getDataLength();
        }
    }

    uint32_t readLEUint32(const uint8_t *buffer)
    {
        uint32_t value = buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8 | buffer[0];
        return value;
    }

    int64_t readLEInt64(const uint8_t *buffer)
    {
        int64_t value = static_cast<int64_t>(buffer[7]) << 56 | static_cast<int64_t>(buffer[6]) << 48 |
                        static_cast<int64_t>(buffer[5]) << 40 | static_cast<int64_t>(buffer[4]) << 32 |
                        static_cast<int64_t>(buffer[3]) << 24 | static_cast<int64_t>(buffer[2]) << 16 |
                        static_cast<int64_t>(buffer[1]) << 8 | static_cast<int64_t>(buffer[0]);
        return value;
    }
};

/**
 * Test that an MediaFrameWriterV1 object can write a frame to the shared buffer.
 */
TEST_F(RialtoPlayerCommonWriteFrameV1Test, WriteAudioFrame)
{
    m_mediaFrameWriter = std::make_unique<MediaFrameWriterV1>(m_shmBuffer, m_shmInfo);

    AddAudioFrame(1000000000, 0, 24, 2, 4);

    EXPECT_EQ(m_mediaFrameWriter->writeFrame(m_dataVec[0]), AddSegmentStatus::OK);

    CheckSharedBuffer(MediaSourceType::AUDIO, m_shmInfo);
}

/**
 * Test that an MediaFrameWriterV1 object can write a frame to the shared buffer.
 */
TEST_F(RialtoPlayerCommonWriteFrameV1Test, WriteVideoFrame)
{
    m_mediaFrameWriter = std::make_unique<MediaFrameWriterV1>(m_shmBuffer, m_shmInfo);

    AddVideoFrame(1000000000, 0, 8, 9, 4);

    EXPECT_EQ(m_mediaFrameWriter->writeFrame(m_dataVec[0]), AddSegmentStatus::OK);

    CheckSharedBuffer(MediaSourceType::VIDEO, m_shmInfo);
}

/**
 * Test that an FrameWriter object returns failure if the maximum metadata bytes to
 * write has been reached.
 */
TEST_F(RialtoPlayerCommonWriteFrameV1Test, WriteFrameMaxMetadataFailure)
{
    // Enough to write the version number, but not for a a single frame of metadata
    m_shmInfo->maxMetadataBytes = 10;

    m_mediaFrameWriter = std::make_unique<MediaFrameWriterV1>(m_shmBuffer, m_shmInfo);

    AddVideoFrame(1000000000, 0, 8, 9, 4);

    EXPECT_EQ(m_mediaFrameWriter->writeFrame(m_dataVec[0]), AddSegmentStatus::NO_SPACE);
}

/**
 * Test that an FrameWriter object returns failure if the maximum media data bytes to
 * write has been reached.
 */
TEST_F(RialtoPlayerCommonWriteFrameV1Test, WriteFrameMaxMediaDataFailure)
{
    // Set a small max bytes in the frame writer
    m_shmInfo->maxMediaBytes = 1;

    m_mediaFrameWriter = std::make_unique<MediaFrameWriterV1>(m_shmBuffer, m_shmInfo);

    // Create a frame that is 1 byte bigger than the max
    AddVideoFrame(1000000000, 0, 8, 9, 2);

    EXPECT_EQ(m_mediaFrameWriter->writeFrame(m_dataVec[0]), AddSegmentStatus::NO_SPACE);
}

/**
 * Test that an FrameWriter object can write multiple frames to the shared buffer.
 */
TEST_F(RialtoPlayerCommonWriteFrameV1Test, WriteMultipleFrames)
{
    m_mediaFrameWriter = std::make_unique<MediaFrameWriterV1>(m_shmBuffer, m_shmInfo);

    AddAudioFrame(1000000000, 0, 24, 2, 4);
    AddAudioFrame(2000000000, 2000000000, 12, 4, 6);
    AddAudioFrame(1000000000, 3000000000, 24, 3, 2);

    EXPECT_EQ(m_mediaFrameWriter->writeFrame(m_dataVec[0]), AddSegmentStatus::OK);
    EXPECT_EQ(m_mediaFrameWriter->writeFrame(m_dataVec[1]), AddSegmentStatus::OK);
    EXPECT_EQ(m_mediaFrameWriter->writeFrame(m_dataVec[2]), AddSegmentStatus::OK);

    CheckSharedBuffer(MediaSourceType::AUDIO, m_shmInfo);
}

/**
 * Test that an FrameWriter object can write multiple frames to the shared buffer with a offset.
 */
TEST_F(RialtoPlayerCommonWriteFrameV1Test, WriteMultipleFramesOffset)
{
    m_shmInfo->metadataOffset += 10;
    m_shmInfo->mediaDataOffset += 10;

    m_mediaFrameWriter = std::make_unique<MediaFrameWriterV1>(m_shmBuffer, m_shmInfo);

    AddVideoFrame(1000000000, 0, 24, 2, 4);
    AddVideoFrame(2000000000, 2000000000, 12, 4, 6);
    AddVideoFrame(1000000000, 3000000000, 24, 3, 2);

    EXPECT_EQ(m_mediaFrameWriter->writeFrame(m_dataVec[0]), AddSegmentStatus::OK);
    EXPECT_EQ(m_mediaFrameWriter->writeFrame(m_dataVec[1]), AddSegmentStatus::OK);
    EXPECT_EQ(m_mediaFrameWriter->writeFrame(m_dataVec[2]), AddSegmentStatus::OK);

    CheckSharedBuffer(MediaSourceType::VIDEO, m_shmInfo);
}
