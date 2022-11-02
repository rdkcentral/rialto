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

#include "MediaFrameWriterV2.h"
#include "metadata.pb.h"
#include <gtest/gtest.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::common;

namespace
{
constexpr uint32_t kMaxMetaBytes{6};
constexpr uint32_t kMaxBytes{250};
constexpr uint8_t kMediaData[]{0xD, 0xE, 0xA, 0xD, 0xB, 0xE, 0xE, 0xF};
constexpr uint32_t kMediaDataLength{8};
constexpr int32_t kSourceId{1};
constexpr int64_t kTimeStamp{1423435};
constexpr int64_t kDuration{12324};
constexpr int32_t kSampleRate{3536};
constexpr int32_t kNumberOfChannels{3};
constexpr int32_t kWidth{1024};
constexpr int32_t kHeight{768};
const std::vector<uint8_t> kExtraData{1, 2, 3, 4};
const int32_t kMksId{43};
const std::vector<uint8_t> kKeyId{9, 2, 6, 2, 0, 1};
const std::vector<uint8_t> kInitVector{34, 53, 54, 62, 56};
constexpr size_t kNumClearBytes{2};
constexpr size_t kNumEncryptedBytes{7};
constexpr uint32_t kInitWithLast15{1};

uint32_t readLEUint32(const uint8_t *buffer)
{
    uint32_t value = buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8 | buffer[0];
    return value;
}

std::unique_ptr<IMediaPipeline::MediaSegment> createAudioSegment()
{
    auto segment{std::make_unique<IMediaPipeline::MediaSegmentAudio>(kSourceId, kTimeStamp, kDuration, kSampleRate,
                                                                     kNumberOfChannels)};
    segment->setData(kMediaDataLength, kMediaData);
    return segment;
}

std::unique_ptr<IMediaPipeline::MediaSegment> createVideoSegment()
{
    auto segment{std::make_unique<IMediaPipeline::MediaSegmentVideo>(kSourceId, kTimeStamp, kDuration, kWidth, kHeight)};
    segment->setData(kMediaDataLength, kMediaData);
    return segment;
}

void addOptionalData(std::unique_ptr<IMediaPipeline::MediaSegment> &segment)
{
    segment->setSegmentAlignment(SegmentAlignment::NAL);
    segment->setExtraData(kExtraData);
}

void addEncryptionData(std::unique_ptr<IMediaPipeline::MediaSegment> &segment)
{
    segment->setEncrypted(true);
    segment->setMediaKeySessionId(kMksId);
    segment->setKeyId(kKeyId);
    segment->setInitVector(kInitVector);
    segment->addSubSample(kNumClearBytes, kNumEncryptedBytes);
    segment->setInitWithLast15(kInitWithLast15);
}

void checkMandatoryMetadata(const MediaSegmentMetadata &metadata)
{
    EXPECT_EQ(metadata.length(), kMediaDataLength);
    EXPECT_EQ(metadata.time_position(), kTimeStamp);
    EXPECT_EQ(metadata.sample_duration(), kDuration);
    EXPECT_EQ(metadata.stream_id(), kSourceId);
}

void checkAudioMetadata(const MediaSegmentMetadata &metadata)
{
    EXPECT_EQ(metadata.sample_rate(), kSampleRate);
    EXPECT_EQ(metadata.channels_num(), kNumberOfChannels);
    EXPECT_FALSE(metadata.has_width());
    EXPECT_FALSE(metadata.has_height());
}

void checkVideoMetadata(const MediaSegmentMetadata &metadata)
{
    EXPECT_FALSE(metadata.has_sample_rate());
    EXPECT_FALSE(metadata.has_channels_num());
    EXPECT_EQ(metadata.width(), kWidth);
    EXPECT_EQ(metadata.height(), kHeight);
}

void checkOptionalMetadataNotPresent(const MediaSegmentMetadata &metadata)
{
    EXPECT_FALSE(metadata.has_segment_alignment());
    EXPECT_FALSE(metadata.has_extra_data());
}

void checkOptionalMetadataPresent(const MediaSegmentMetadata &metadata)
{
    EXPECT_TRUE(metadata.has_segment_alignment());
    EXPECT_EQ(metadata.segment_alignment(), MediaSegmentMetadata_SegmentAlignment_ALIGNMENT_NAL);
    EXPECT_TRUE(metadata.has_extra_data());
    EXPECT_EQ(metadata.extra_data(), std::string(kExtraData.begin(), kExtraData.end()));
}

void checkEncryptionMetadataNotPresent(const MediaSegmentMetadata &metadata)
{
    EXPECT_FALSE(metadata.has_media_key_session_id());
    EXPECT_FALSE(metadata.has_key_id());
    EXPECT_FALSE(metadata.has_init_vector());
    EXPECT_FALSE(metadata.has_init_with_last_15());
    EXPECT_EQ(metadata.sub_sample_info_size(), 0);
}

void checkEncryptionMetadataPresent(const MediaSegmentMetadata &metadata)
{
    EXPECT_TRUE(metadata.has_media_key_session_id());
    EXPECT_TRUE(metadata.has_key_id());
    EXPECT_TRUE(metadata.has_init_vector());
    EXPECT_TRUE(metadata.has_init_with_last_15());
    EXPECT_EQ(metadata.sub_sample_info_size(), 1);
    EXPECT_EQ(metadata.media_key_session_id(), kMksId);
    EXPECT_EQ(metadata.key_id(), std::string(kKeyId.begin(), kKeyId.end()));
    EXPECT_EQ(metadata.init_vector(), std::string(kInitVector.begin(), kInitVector.end()));
    EXPECT_EQ(metadata.init_with_last_15(), kInitWithLast15);
    EXPECT_EQ(metadata.sub_sample_info(0).num_clear_bytes(), kNumClearBytes);
    EXPECT_EQ(metadata.sub_sample_info(0).num_encrypted_bytes(), kNumEncryptedBytes);
}

void checkMediaData(uint8_t *readPosition)
{
    EXPECT_EQ(0xD, *readPosition);
    ++readPosition;
    EXPECT_EQ(0xE, *readPosition);
    ++readPosition;
    EXPECT_EQ(0xA, *readPosition);
    ++readPosition;
    EXPECT_EQ(0xD, *readPosition);
    ++readPosition;
    EXPECT_EQ(0xB, *readPosition);
    ++readPosition;
    EXPECT_EQ(0xE, *readPosition);
    ++readPosition;
    EXPECT_EQ(0xE, *readPosition);
    ++readPosition;
    EXPECT_EQ(0xF, *readPosition);
}
} // namespace

class RialtoPlayerCommonWriteFrameV2Test : public ::testing::Test
{
protected:
    uint8_t m_shmBuffer[kMaxBytes] = {0};
    std::shared_ptr<ShmInfo> m_shmInfo;

    virtual void SetUp()
    {
        // init shm info
        m_shmInfo = std::make_shared<ShmInfo>();
        m_shmInfo->maxMetadataBytes = kMaxMetaBytes;
        m_shmInfo->metadataOffset = 0;
        m_shmInfo->mediaDataOffset = kMaxMetaBytes;
        m_shmInfo->maxMediaBytes = kMaxBytes;
    }

    MediaSegmentMetadata readSegment()
    {
        uint8_t *readPosition{m_shmBuffer};
        // Version should be set to 2
        EXPECT_EQ(readLEUint32(readPosition), 2U);
        readPosition += kMaxMetaBytes;

        uint32_t metadataSize{readLEUint32(readPosition)};
        readPosition += sizeof(metadataSize);

        MediaSegmentMetadata metadata;
        metadata.ParseFromArray(readPosition, metadataSize);

        checkMandatoryMetadata(metadata);

        readPosition += metadataSize;

        checkMediaData(readPosition);

        return metadata;
    }
};

/**
 * Test that an MediaFrameWriterV2 can write unencrypted audio without optional params
 */
TEST_F(RialtoPlayerCommonWriteFrameV2Test, WriteUnencryptedAudioWithoutOptionalParams)
{
    auto segment = createAudioSegment();
    MediaFrameWriterV2 mediaFrameWriter{m_shmBuffer, m_shmInfo};
    EXPECT_EQ(AddSegmentStatus::OK, mediaFrameWriter.writeFrame(segment));
    EXPECT_EQ(1, mediaFrameWriter.getNumFrames());
    auto metadata = readSegment();
    checkAudioMetadata(metadata);
    checkOptionalMetadataNotPresent(metadata);
    checkEncryptionMetadataNotPresent(metadata);
}

/**
 * Test that an MediaFrameWriterV2 can write unencrypted audio with optional params
 */
TEST_F(RialtoPlayerCommonWriteFrameV2Test, WriteUnencryptedAudioWithOptionalParams)
{
    auto segment = createAudioSegment();
    addOptionalData(segment);
    MediaFrameWriterV2 mediaFrameWriter{m_shmBuffer, m_shmInfo};
    EXPECT_EQ(AddSegmentStatus::OK, mediaFrameWriter.writeFrame(segment));
    EXPECT_EQ(1, mediaFrameWriter.getNumFrames());
    auto metadata = readSegment();
    checkAudioMetadata(metadata);
    checkOptionalMetadataPresent(metadata);
    checkEncryptionMetadataNotPresent(metadata);
}

/**
 * Test that an MediaFrameWriterV2 can write encrypted audio
 */
TEST_F(RialtoPlayerCommonWriteFrameV2Test, WriteEncryptedAudio)
{
    auto segment = createAudioSegment();
    addEncryptionData(segment);
    MediaFrameWriterV2 mediaFrameWriter{m_shmBuffer, m_shmInfo};
    EXPECT_EQ(AddSegmentStatus::OK, mediaFrameWriter.writeFrame(segment));
    EXPECT_EQ(1, mediaFrameWriter.getNumFrames());
    auto metadata = readSegment();
    checkAudioMetadata(metadata);
    checkOptionalMetadataNotPresent(metadata);
    checkEncryptionMetadataPresent(metadata);
}

/**
 * Test that an MediaFrameWriterV2 can write unencrypted video without optional params
 */
TEST_F(RialtoPlayerCommonWriteFrameV2Test, WriteUnencryptedVideoWithoutOptionalParams)
{
    auto segment = createVideoSegment();
    MediaFrameWriterV2 mediaFrameWriter{m_shmBuffer, m_shmInfo};
    EXPECT_EQ(AddSegmentStatus::OK, mediaFrameWriter.writeFrame(segment));
    EXPECT_EQ(1, mediaFrameWriter.getNumFrames());
    auto metadata = readSegment();
    checkVideoMetadata(metadata);
    checkOptionalMetadataNotPresent(metadata);
    checkEncryptionMetadataNotPresent(metadata);
}

/**
 * Test that an MediaFrameWriterV2 can write unencrypted video with optional params
 */
TEST_F(RialtoPlayerCommonWriteFrameV2Test, WriteUnencryptedVideoWithOptionalParams)
{
    auto segment = createVideoSegment();
    addOptionalData(segment);
    MediaFrameWriterV2 mediaFrameWriter{m_shmBuffer, m_shmInfo};
    EXPECT_EQ(AddSegmentStatus::OK, mediaFrameWriter.writeFrame(segment));
    EXPECT_EQ(1, mediaFrameWriter.getNumFrames());
    auto metadata = readSegment();
    checkVideoMetadata(metadata);
    checkOptionalMetadataPresent(metadata);
    checkEncryptionMetadataNotPresent(metadata);
}

/**
 * Test that an MediaFrameWriterV2 can write encrypted video
 */
TEST_F(RialtoPlayerCommonWriteFrameV2Test, WriteEncryptedVideo)
{
    auto segment = createVideoSegment();
    addEncryptionData(segment);
    MediaFrameWriterV2 mediaFrameWriter{m_shmBuffer, m_shmInfo};
    EXPECT_EQ(AddSegmentStatus::OK, mediaFrameWriter.writeFrame(segment));
    EXPECT_EQ(1, mediaFrameWriter.getNumFrames());
    auto metadata = readSegment();
    checkVideoMetadata(metadata);
    checkOptionalMetadataNotPresent(metadata);
    checkEncryptionMetadataPresent(metadata);
}

/**
 * Test that an MediaFrameWriterV2 will return NO_SPACE when we don't have enough memory
 */
TEST_F(RialtoPlayerCommonWriteFrameV2Test, SkipWritingDueToNoSpaceAvailable)
{
    m_shmInfo->maxMediaBytes = 10;
    auto segment = createVideoSegment();
    MediaFrameWriterV2 mediaFrameWriter{m_shmBuffer, m_shmInfo};
    EXPECT_EQ(AddSegmentStatus::NO_SPACE, mediaFrameWriter.writeFrame(segment));
    EXPECT_EQ(0, mediaFrameWriter.getNumFrames());
}

/**
 * Test that an MediaFrameWriterV2 will return ERROR when MediaSegment has unknown media type
 */
TEST_F(RialtoPlayerCommonWriteFrameV2Test, SkipWritingDueToUnknownDataType)
{
    auto segment = std::make_unique<IMediaPipeline::MediaSegment>();
    MediaFrameWriterV2 mediaFrameWriter{m_shmBuffer, m_shmInfo};
    EXPECT_EQ(AddSegmentStatus::ERROR, mediaFrameWriter.writeFrame(segment));
    EXPECT_EQ(0, mediaFrameWriter.getNumFrames());
}
