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
#include <random>

namespace
{
const std::string kSegments[] = {"lloefewhrwohruwhfr9eee8833hbcjka",
                                 "hyrejhye y6u2255y6858geqszz",
                                 "nvid9",
                                 "#mcuruiw83udbhc",
                                 "yr6w4hgsbhjk674kuur",
                                 "hrjkl8p09[jtrwhstry5twyyuhhgdhtyww455y]",
                                 "cnvbvfbuhrew8383hbvbckzoaopalwlelhvrwvc",
                                 "yu764thgkio9p9trw545jjhf",
                                 "R080E",
                                 "nvid9",
                                 "yyt",
                                 "greabfdbnmur7y64wnguyoyrar55shshjw",
                                 "viru80202-idendhjx cefebfihgrwwwwcdcwd",
                                 "shtrnkkil ik;oiy,etq 525425",
                                 "nvid9",
                                 "ncie83uuwonvbbvaoaodoekjvbv  vyreifur779y",
                                 "yu764thgkio9p9trw545jjhf",
                                 "greabfdbnmur7y64wnguyoyrar55shshjw",
                                 "yr6w4hgsbhjk674kuur",
                                 "viru80202-idendhjx cefebfihgrwwwwcdcwd",
                                 "ncie83uuwonvbbvaoaodoekjvbv  vyreifur779y",
                                 "pdfigjiudfhgoidfhgiudfhgiudfhg",
                                 "t98ygurvschv98egjow9eufwrpg",
                                 "do9 gynq9rfy9weuf8eroigh9u"};
const firebolt::rialto::CodecData kCodecData{std::vector<std::uint8_t>(std::vector<std::uint8_t>{4, 3, 2, 1}),
                                             firebolt::rialto::CodecDataType::BUFFER};
constexpr uint32_t kInitWithLast15{1};
constexpr firebolt::rialto::SegmentAlignment kSegmentAlignment{firebolt::rialto::SegmentAlignment::NAL};

template <typename T> T generate(T max = std::numeric_limits<T>::max())
{
    std::random_device rd;
    std::default_random_engine gen(rd());
    std::uniform_int_distribution<T> distrib(0, max);
    return distrib(gen);
}

std::vector<uint8_t> generateBytes()
{
    constexpr size_t kMaxSize{10};
    const std::size_t bytesLen{generate<std::size_t>(kMaxSize)};
    std::vector<uint8_t> bytes(bytesLen);
    std::generate(bytes.begin(), bytes.end(), []() { return generate<uint8_t>(); });
    return bytes;
}
} // namespace

namespace firebolt::rialto::server::ct
{
SegmentBuilder &SegmentBuilder::basicVideoSegment(int sourceId)
{
    m_segment = std::make_unique<IMediaPipeline::MediaSegmentVideo>(sourceId, generate<int64_t>(), generate<int64_t>(),
                                                                    kWidth, kHeight, kFrameRate);
    const size_t kIdx{generate<std::size_t>(std::size(kSegments) - 1)};
    m_segment->setData(kSegments[kIdx].size(), reinterpret_cast<const uint8_t *>(kSegments[kIdx].data()));
    return *this;
}

SegmentBuilder &SegmentBuilder::basicAudioSegment(int sourceId)
{
    m_segment = std::make_unique<IMediaPipeline::MediaSegmentAudio>(sourceId, generate<int64_t>(), generate<int64_t>(),
                                                                    kSampleRate, kNumOfChannels);
    const size_t kIdx{generate<std::size_t>(std::size(kSegments) - 1)};
    m_segment->setData(kSegments[kIdx].size(), reinterpret_cast<const uint8_t *>(kSegments[kIdx].data()));
    return *this;
}

SegmentBuilder &SegmentBuilder::withOptionalData()
{
    m_segment->setExtraData(generateBytes());
    m_segment->setSegmentAlignment(kSegmentAlignment);
    m_segment->setCodecData(std::make_shared<firebolt::rialto::CodecData>(kCodecData));
    return *this;
}

SegmentBuilder &SegmentBuilder::withEncryptionData()
{
    m_segment->setEncrypted(true);
    m_segment->setMediaKeySessionId(generate<int32_t>());
    m_segment->setKeyId(generateBytes());
    m_segment->setInitVector(generateBytes());
    m_segment->addSubSample(generate<std::size_t>(), generate<std::size_t>());
    m_segment->setInitWithLast15(kInitWithLast15);

    return *this;
}

SegmentBuilder &SegmentBuilder::withCBCSCipherMode()
{
    m_segment->setCipherMode(CipherMode::CBCS);
    m_segment->setEncryptionPattern(generate<uint32_t>(), generate<uint32_t>());

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
    m_segment->setEncryptionPattern(generate<uint32_t>(), generate<uint32_t>());
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
