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

#include "DataReaderV2.h"
#include "IMediaFrameWriter.h"
#include <gtest/gtest.h>

using firebolt::rialto::AddSegmentStatus;
using firebolt::rialto::CipherMode;
using firebolt::rialto::IMediaPipeline;
using firebolt::rialto::MediaPlayerShmInfo;
using firebolt::rialto::SegmentAlignment;
using firebolt::rialto::common::IMediaFrameWriter;
using firebolt::rialto::common::IMediaFrameWriterFactory;
using firebolt::rialto::server::DataReaderV2;

namespace
{
constexpr size_t kMetaDataSize{10};
constexpr size_t kDataSize{246};
constexpr auto kVideoMediaSourceType{firebolt::rialto::MediaSourceType::VIDEO};
constexpr auto kVideoSourceId{static_cast<std::int32_t>(kVideoMediaSourceType)};
constexpr auto kAudioMediaSourceType{firebolt::rialto::MediaSourceType::AUDIO};
constexpr auto kAudioSourceId{static_cast<std::int32_t>(kAudioMediaSourceType)};
constexpr int64_t kTimeStamp{4135000000000};
constexpr int64_t kDuration{90000000000};
constexpr int32_t kWidth{1024};
constexpr int32_t kHeight{768};
constexpr firebolt::rialto::Fraction kFrameRate{15, 1};
constexpr int32_t kSampleRate{13};
constexpr int32_t kNumberOfChannels{4};
std::vector<uint8_t> kMediaData{'T', 'E', 'S', 'T', '_', 'M', 'E', 'D', 'I', 'A'};
std::uint32_t kNumFrames{1};
const std::vector<uint8_t> kExtraData{1, 2, 3, 4};
const firebolt::rialto::CodecData kCodecData{std::vector<std::uint8_t>(std::vector<std::uint8_t>{4, 3, 2, 1}),
                                             firebolt::rialto::CodecDataType::BUFFER};
const int32_t kMksId{43};
const std::vector<uint8_t> kKeyId{9, 2, 6, 2, 0, 1};
const std::vector<uint8_t> kInitVector{34, 53, 54, 62, 56};
constexpr size_t kNumClearBytes{2};
constexpr size_t kNumEncryptedBytes{7};
constexpr uint32_t kInitWithLast15{1};
constexpr SegmentAlignment kSegmentAlignment{SegmentAlignment::AU};

constexpr uint32_t kCryptBlocks{131};
constexpr uint32_t kSkipBlocks{242};

class Check
{
public:
    explicit Check(std::unique_ptr<IMediaPipeline::MediaSegment> &segment) : m_segment{segment}
    {
        EXPECT_TRUE(segment);
    }

    Check &mandatoryDataPresent()
    {
        EXPECT_EQ(m_segment->getTimeStamp(), kTimeStamp);
        EXPECT_EQ(m_segment->getDuration(), kDuration);
        EXPECT_EQ(m_segment->getDataLength(), kMediaData.size());
        std::vector<uint8_t> resultData{m_segment->getData(), m_segment->getData() + m_segment->getDataLength()};
        EXPECT_EQ(resultData, kMediaData);
        return *this;
    }

    Check &audioDataPresent()
    {
        IMediaPipeline::MediaSegmentAudio *resultSegment =
            dynamic_cast<IMediaPipeline::MediaSegmentAudio *>(m_segment.get());
        EXPECT_NE(nullptr, resultSegment);
        EXPECT_EQ(resultSegment->getType(), kAudioMediaSourceType);
        EXPECT_EQ(resultSegment->getSampleRate(), kSampleRate);
        EXPECT_EQ(resultSegment->getNumberOfChannels(), kNumberOfChannels);
        return *this;
    }

    Check &videoDataPresent()
    {
        IMediaPipeline::MediaSegmentVideo *resultSegment =
            dynamic_cast<IMediaPipeline::MediaSegmentVideo *>(m_segment.get());
        EXPECT_NE(nullptr, resultSegment);
        EXPECT_EQ(resultSegment->getType(), kVideoMediaSourceType);
        EXPECT_EQ(resultSegment->getWidth(), kWidth);
        EXPECT_EQ(resultSegment->getHeight(), kHeight);
        EXPECT_EQ(resultSegment->getFrameRate(), kFrameRate);
        return *this;
    }

    Check &optionalDataPresent()
    {
        EXPECT_EQ(m_segment->getExtraData(), kExtraData);
        EXPECT_EQ(m_segment->getSegmentAlignment(), kSegmentAlignment);
        EXPECT_TRUE(m_segment->getCodecData());
        EXPECT_EQ(m_segment->getCodecData()->data, kCodecData.data);
        EXPECT_EQ(m_segment->getCodecData()->type, kCodecData.type);
        return *this;
    }

    Check &optionalDataNotPresent()
    {
        EXPECT_TRUE(m_segment->getExtraData().empty());
        EXPECT_EQ(m_segment->getSegmentAlignment(), firebolt::rialto::SegmentAlignment::UNDEFINED);
        EXPECT_FALSE(m_segment->getCodecData());
        return *this;
    }

    Check &encryptionDataPresent()
    {
        EXPECT_TRUE(m_segment->isEncrypted());
        EXPECT_EQ(m_segment->getMediaKeySessionId(), kMksId);
        EXPECT_EQ(m_segment->getKeyId(), kKeyId);
        EXPECT_EQ(m_segment->getInitVector(), kInitVector);
        EXPECT_EQ(m_segment->getSubSamples().size(), 1);
        EXPECT_EQ(m_segment->getSubSamples().front().numClearBytes, kNumClearBytes);
        EXPECT_EQ(m_segment->getSubSamples().front().numEncryptedBytes, kNumEncryptedBytes);
        EXPECT_EQ(m_segment->getInitWithLast15(), kInitWithLast15);
        return *this;
    }

    Check &cipherModeCBCSPresent()
    {
        EXPECT_EQ(m_segment->getCipherMode(), CipherMode::CBCS);
        uint32_t crypt{0};
        uint32_t skip{0};
        EXPECT_TRUE(m_segment->getEncryptionPattern(crypt, skip));
        EXPECT_EQ(crypt, kCryptBlocks);
        EXPECT_EQ(skip, kSkipBlocks);
        return *this;
    }

    Check &cipherModeCENCPresent()
    {
        EXPECT_EQ(m_segment->getCipherMode(), CipherMode::CENC);
        uint32_t crypt{0};
        uint32_t skip{0};
        EXPECT_FALSE(m_segment->getEncryptionPattern(crypt, skip));
        return *this;
    }
    Check &cipherModeCENSPresent()
    {
        EXPECT_EQ(m_segment->getCipherMode(), CipherMode::CENS);
        uint32_t crypt{0};
        uint32_t skip{0};
        EXPECT_TRUE(m_segment->getEncryptionPattern(crypt, skip));
        EXPECT_EQ(crypt, kCryptBlocks);
        EXPECT_EQ(skip, kSkipBlocks);
        return *this;
    }

    Check &cipherModeCBC1Present()
    {
        EXPECT_EQ(m_segment->getCipherMode(), CipherMode::CBC1);
        uint32_t crypt{0};
        uint32_t skip{0};
        EXPECT_FALSE(m_segment->getEncryptionPattern(crypt, skip));
        return *this;
    }

    Check &encryptionDataNotPresent()
    {
        EXPECT_FALSE(m_segment->isEncrypted());
        EXPECT_EQ(m_segment->getMediaKeySessionId(), 0);
        EXPECT_TRUE(m_segment->getKeyId().empty());
        EXPECT_TRUE(m_segment->getInitVector().empty());
        EXPECT_TRUE(m_segment->getSubSamples().empty());
        EXPECT_EQ(m_segment->getInitWithLast15(), 0);
        EXPECT_EQ(m_segment->getCipherMode(), CipherMode::UNKNOWN);
        uint32_t crypt{0};
        uint32_t skip{0};
        EXPECT_FALSE(m_segment->getEncryptionPattern(crypt, skip));
        return *this;
    }

private:
    std::unique_ptr<IMediaPipeline::MediaSegment> &m_segment;
};

class Build
{
public:
    Build &basicVideoSegment()
    {
        m_segment = std::make_unique<IMediaPipeline::MediaSegmentVideo>(kVideoSourceId, kTimeStamp, kDuration, kWidth,
                                                                        kHeight, kFrameRate);
        m_segment->setData(kMediaData.size(), kMediaData.data());
        return *this;
    }

    Build &basicAudioSegment()
    {
        m_segment = std::make_unique<IMediaPipeline::MediaSegmentAudio>(kAudioSourceId, kTimeStamp, kDuration,
                                                                        kSampleRate, kNumberOfChannels);
        m_segment->setData(kMediaData.size(), kMediaData.data());
        return *this;
    }

    Build &withOptionalData()
    {
        m_segment->setExtraData(kExtraData);
        m_segment->setSegmentAlignment(kSegmentAlignment);
        m_segment->setCodecData(std::make_shared<firebolt::rialto::CodecData>(kCodecData));
        return *this;
    }

    Build &withEncryptionData()
    {
        m_segment->setEncrypted(true);
        m_segment->setMediaKeySessionId(kMksId);
        m_segment->setKeyId(kKeyId);
        m_segment->setInitVector(kInitVector);
        m_segment->addSubSample(kNumClearBytes, kNumEncryptedBytes);
        m_segment->setInitWithLast15(kInitWithLast15);

        return *this;
    }

    Build &withCBCSCipherMode()
    {
        m_segment->setCipherMode(CipherMode::CBCS);
        m_segment->setEncryptionPattern(kCryptBlocks, kSkipBlocks);

        return *this;
    }

    Build &withCENCCipherMode()
    {
        m_segment->setCipherMode(CipherMode::CENC);
        return *this;
    }

    Build &withCENSCipherMode()
    {
        m_segment->setCipherMode(CipherMode::CENS);
        m_segment->setEncryptionPattern(kCryptBlocks, kSkipBlocks);
        return *this;
    }

    Build &withCBC1CipherMode()
    {
        m_segment->setCipherMode(CipherMode::CBC1);
        return *this;
    }

    std::unique_ptr<IMediaPipeline::MediaSegment> operator()() { return std::move(m_segment); }

private:
    std::unique_ptr<IMediaPipeline::MediaSegment> m_segment;
};
} // namespace

namespace firebolt::rialto
{
bool operator==(const Fraction &lhs, const Fraction &rhs)
{
    return lhs.numerator == rhs.numerator && lhs.denominator == rhs.denominator;
}
} // namespace firebolt::rialto
class DataReaderV2Tests : public testing::Test
{
protected:
    DataReaderV2Tests() = default;

    std::unique_ptr<IMediaPipeline::MediaSegment> readData(const firebolt::rialto::MediaSourceType &sourceType)
    {
        m_sut = std::make_unique<DataReaderV2>(sourceType, m_shm, kMetaDataSize, kNumFrames);
        auto result = m_sut->readData();
        if (result.size() != 1)
            return nullptr;
        return std::unique_ptr< IMediaPipeline::MediaSegment>(std::move(result.front()));
    }

    void writeBuffer(const std::unique_ptr<IMediaPipeline::MediaSegment> &segment)
    {
        auto shmInfo =
            std::make_shared<MediaPlayerShmInfo>(MediaPlayerShmInfo{kMetaDataSize, 0, kMetaDataSize, kDataSize});
        auto mediaFrameWriter = IMediaFrameWriterFactory::getFactory()->createFrameWriter(m_shm, shmInfo);
        EXPECT_EQ(mediaFrameWriter->writeFrame(segment), AddSegmentStatus::OK);
    }

    void doSomeMessInMemory()
    {
        m_shm[12] = 'S';
        m_shm[13] = 'U';
        m_shm[14] = 'R';
        m_shm[15] = 'P';
        m_shm[16] = 'R';
        m_shm[17] = 'I';
        m_shm[18] = 'S';
        m_shm[19] = 'E';
    }

private:
    uint8_t m_shm[kMetaDataSize + kDataSize];
    std::unique_ptr<DataReaderV2> m_sut;
};

TEST_F(DataReaderV2Tests, shouldReadBasicVideoData)
{
    auto inputSegment = Build().basicVideoSegment()();
    writeBuffer(inputSegment);
    auto resultSegment = readData(kVideoMediaSourceType);
    Check(resultSegment).mandatoryDataPresent().videoDataPresent().optionalDataNotPresent().encryptionDataNotPresent();
}

TEST_F(DataReaderV2Tests, shouldReadBasicAudioData)
{
    auto inputSegment = Build().basicAudioSegment()();
    writeBuffer(inputSegment);
    auto resultSegment = readData(kAudioMediaSourceType);
    Check(resultSegment).mandatoryDataPresent().audioDataPresent().optionalDataNotPresent().encryptionDataNotPresent();
}

TEST_F(DataReaderV2Tests, shouldReadVideoDataWithOptionalParams)
{
    auto inputSegment = Build().basicVideoSegment().withOptionalData()();
    writeBuffer(inputSegment);
    auto resultSegment = readData(kVideoMediaSourceType);
    Check(resultSegment).mandatoryDataPresent().videoDataPresent().optionalDataPresent().encryptionDataNotPresent();
}

TEST_F(DataReaderV2Tests, shouldReadAudioDataWithOptionalParams)
{
    auto inputSegment = Build().basicAudioSegment().withOptionalData()();
    writeBuffer(inputSegment);
    auto resultSegment = readData(kAudioMediaSourceType);
    Check(resultSegment).mandatoryDataPresent().audioDataPresent().optionalDataPresent().encryptionDataNotPresent();
}

TEST_F(DataReaderV2Tests, shouldReadCBCSEncryptedVideoData)
{
    auto inputSegment = Build().basicVideoSegment().withEncryptionData().withCBCSCipherMode()();
    writeBuffer(inputSegment);
    auto resultSegment = readData(kVideoMediaSourceType);
    Check(resultSegment)
        .mandatoryDataPresent()
        .videoDataPresent()
        .optionalDataNotPresent()
        .encryptionDataPresent()
        .cipherModeCBCSPresent();
}

TEST_F(DataReaderV2Tests, shouldReadCENCEncryptedAudioData)
{
    auto inputSegment = Build().basicAudioSegment().withEncryptionData().withCENCCipherMode()();
    writeBuffer(inputSegment);
    auto resultSegment = readData(kAudioMediaSourceType);
    Check(resultSegment)
        .mandatoryDataPresent()
        .audioDataPresent()
        .optionalDataNotPresent()
        .encryptionDataPresent()
        .cipherModeCENCPresent();
}

TEST_F(DataReaderV2Tests, shouldReadCENSEncryptedVideoData)
{
    auto inputSegment = Build().basicVideoSegment().withEncryptionData().withCENSCipherMode()();
    writeBuffer(inputSegment);
    auto resultSegment = readData(kVideoMediaSourceType);
    Check(resultSegment)
        .mandatoryDataPresent()
        .videoDataPresent()
        .optionalDataNotPresent()
        .encryptionDataPresent()
        .cipherModeCENSPresent();
}

TEST_F(DataReaderV2Tests, shouldReadCBC1EncryptedAudioData)
{
    auto inputSegment = Build().basicAudioSegment().withEncryptionData().withCBC1CipherMode()();
    writeBuffer(inputSegment);
    auto resultSegment = readData(kAudioMediaSourceType);
    Check(resultSegment)
        .mandatoryDataPresent()
        .audioDataPresent()
        .optionalDataNotPresent()
        .encryptionDataPresent()
        .cipherModeCBC1Present();
}

TEST_F(DataReaderV2Tests, shouldReturnEmptyVectorWhenVideoSourceTypeIsSelectedForAudioData)
{
    auto inputSegment = Build().basicAudioSegment()();
    writeBuffer(inputSegment);
    auto resultSegment = readData(kVideoMediaSourceType);
    EXPECT_FALSE(resultSegment);
}

TEST_F(DataReaderV2Tests, shouldReturnEmptyVectorWhenAudioSourceTypeIsSelectedForVideoData)
{
    auto inputSegment = Build().basicVideoSegment()();
    writeBuffer(inputSegment);
    auto resultSegment = readData(kAudioMediaSourceType);
    EXPECT_FALSE(resultSegment);
}

TEST_F(DataReaderV2Tests, shouldReturnEmptyVectorWhenMetadataParsingFails)
{
    auto inputSegment = Build().basicAudioSegment()();
    writeBuffer(inputSegment);
    doSomeMessInMemory();
    auto resultSegment = readData(kAudioMediaSourceType);
    EXPECT_FALSE(resultSegment);
}
