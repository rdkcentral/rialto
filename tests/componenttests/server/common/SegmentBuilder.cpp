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

#include "SegmentBuilder.h"
#include "Constants.h"

namespace
{
constexpr int64_t kTimeStamp{4135000000000};
constexpr int64_t kDuration{90000000000};
constexpr firebolt::rialto::Fraction kFrameRate{15, 1};
std::vector<uint8_t> kMediaData{'T', 'E', 'S', 'T', '_', 'M', 'E', 'D', 'I', 'A'};
const std::vector<uint8_t> kExtraData{1, 2, 3, 4};
const firebolt::rialto::CodecData kCodecData{std::vector<std::uint8_t>(std::vector<std::uint8_t>{4, 3, 2, 1}),
                                             firebolt::rialto::CodecDataType::BUFFER};
const int32_t kMksId{43};
const std::vector<uint8_t> kKeyId{9, 2, 6, 2, 0, 1};
const std::vector<uint8_t> kInitVector{34, 53, 54, 62, 56};
constexpr size_t kNumClearBytes{2};
constexpr size_t kNumEncryptedBytes{7};
constexpr uint32_t kInitWithLast15{1};
constexpr firebolt::rialto::SegmentAlignment kSegmentAlignment{firebolt::rialto::SegmentAlignment::NAL};

constexpr uint32_t kCryptBlocks{131};
constexpr uint32_t kSkipBlocks{242};
} // namespace

namespace firebolt::rialto::server::ct
{
SegmentBuilder &SegmentBuilder::basicVideoSegment(int sourceId)
{
    m_segment = std::make_unique<IMediaPipeline::MediaSegmentVideo>(sourceId, kTimeStamp, kDuration, kWidth, kHeight,
                                                                    kFrameRate);
    m_segment->setData(kMediaData.size(), kMediaData.data());
    return *this;
}

SegmentBuilder &SegmentBuilder::basicAudioSegment(int sourceId)
{
    m_segment = std::make_unique<IMediaPipeline::MediaSegmentAudio>(sourceId, kTimeStamp, kDuration, kSampleRate,
                                                                    kNumOfChannels);
    m_segment->setData(kMediaData.size(), kMediaData.data());
    return *this;
}

SegmentBuilder &SegmentBuilder::withOptionalData()
{
    m_segment->setExtraData(kExtraData);
    m_segment->setSegmentAlignment(kSegmentAlignment);
    m_segment->setCodecData(std::make_shared<firebolt::rialto::CodecData>(kCodecData));
    return *this;
}

SegmentBuilder &SegmentBuilder::withEncryptionData()
{
    m_segment->setEncrypted(true);
    m_segment->setMediaKeySessionId(kMksId);
    m_segment->setKeyId(kKeyId);
    m_segment->setInitVector(kInitVector);
    m_segment->addSubSample(kNumClearBytes, kNumEncryptedBytes);
    m_segment->setInitWithLast15(kInitWithLast15);

    return *this;
}

SegmentBuilder &SegmentBuilder::withCBCSCipherMode()
{
    m_segment->setCipherMode(CipherMode::CBCS);
    m_segment->setEncryptionPattern(kCryptBlocks, kSkipBlocks);

    return *this;
}

SegmentBuilder &SegmentBuilder::withCENCCipherMode()
{
    m_segment->setCipherMode(CipherMode::CENC);
    return *this;
}

SegmentBuilder &SegmentBuilder::withCENSCipherMode()
{
    m_segment->setCipherMode(CipherMode::CENS);
    m_segment->setEncryptionPattern(kCryptBlocks, kSkipBlocks);
    return *this;
}

SegmentBuilder &SegmentBuilder::withCBC1CipherMode()
{
    m_segment->setCipherMode(CipherMode::CBC1);
    return *this;
}

std::unique_ptr<IMediaPipeline::MediaSegment> SegmentBuilder::operator()()
{
    return std::move(m_segment);
}
} // namespace firebolt::rialto::server::ct
