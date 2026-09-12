/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "YamlCppWrapper.h"
#include <AudioDecoderCapabilities.h>
#include <MediaCommon.h>
#include <VideoDecoderCapabilities.h>

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace firebolt::rialto::wrappers;
using firebolt::rialto::DecoderCapabilitiesStatus;

namespace
{
const std::string kAudioFilePath{"/product/hfp/config/hfp-audiodecoder.yaml"};
const std::string kVideoFilePath{"/product/hfp/config/hfp-videodecoder.yaml"};
const std::string kConfigDir{"/product/hfp/config"};

void writeYaml(const std::string &path, const std::string &content)
{
    std::ofstream f{path};
    f << content;
}
} // namespace

class YamlCppWrapperTest : public testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        std::error_code ec;
        std::filesystem::create_directories(kConfigDir, ec);
        s_canWrite = !ec;
    }

    void SetUp() override
    {
        std::error_code ec;
        std::filesystem::remove(kAudioFilePath, ec);
        std::filesystem::remove(kVideoFilePath, ec);
    }

    static bool s_canWrite;
    YamlCppWrapper m_sut;
};

bool YamlCppWrapperTest::s_canWrite{false};

// ---- File-not-found tests (always run) ----

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_FileNotFound_ReturnsSchemaValidationFailed)
{
    firebolt::rialto::AudioDecoderCapabilities caps;
    EXPECT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED);
}

TEST_F(YamlCppWrapperTest, GetVideoDecoderCapabilities_FileNotFound_ReturnsSchemaValidationFailed)
{
    firebolt::rialto::VideoDecoderCapabilities caps;
    EXPECT_EQ(m_sut.getVideoDecoderCapabilities(caps), DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED);
}

// ---- Version field tests ----

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_VersionFieldsParsed)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  interfaceVersion: "1.0"
  schemaVersion: "2.0"
  Capabilities: []
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    EXPECT_EQ(caps.interfaceVersion, "1.0");
    EXPECT_EQ(caps.schemaVersion, "2.0");
}

TEST_F(YamlCppWrapperTest, GetVideoDecoderCapabilities_VersionFieldsParsed)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kVideoFilePath, R"yaml(
videodecoder:
  interfaceVersion: "3.0"
  schemaVersion: "4.0"
  Capabilities: []
)yaml");
    firebolt::rialto::VideoDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getVideoDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    EXPECT_EQ(caps.interfaceVersion, "3.0");
    EXPECT_EQ(caps.schemaVersion, "4.0");
}

// ---- Audio codec happy-path tests ----

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_PcmCodec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - PCM:
              profiles:
                - BASE:
                    maxBitrateInBps: 1536000
                    maxChannels: 8
                    maxSampleRateInHz: 192000
                    maxBitDepth: 32
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_EQ(caps.capabilities.size(), 1u);
    ASSERT_TRUE(caps.capabilities[0].pcm.has_value());
    const auto &base = caps.capabilities[0].pcm->base;
    EXPECT_EQ(base.maxBitrateInBps, 1536000u);
    EXPECT_EQ(base.maxChannels, 8u);
    EXPECT_EQ(base.maxSampleRateInHz, 192000u);
    EXPECT_EQ(base.maxBitDepth, 32u);
}

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_AacCodec_MultipleProfilesParsed)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - AAC:
              profiles:
                - LC:
                    maxBitrateInBps: 256000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
                - HE_V1:
                    maxBitrateInBps: 128000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
                - HE_V2:
                    maxBitrateInBps: 64000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
                - ELD:
                    maxBitrateInBps: 192000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
                - X_HE:
                    maxBitrateInBps: 512000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].aac.has_value());
    const auto &profiles = caps.capabilities[0].aac->profiles;
    ASSERT_EQ(profiles.size(), 5u);
    EXPECT_EQ(profiles.at(firebolt::rialto::AacProfile::LC).maxBitrateInBps, 256000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::AacProfile::HE_V1).maxBitrateInBps, 128000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::AacProfile::HE_V2).maxBitrateInBps, 64000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::AacProfile::ELD).maxBitrateInBps, 192000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::AacProfile::X_HE).maxBitrateInBps, 512000u);
}

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_MpegAudioCodec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - MPEG_AUDIO:
              profiles:
                - LAYER_1:
                    maxBitrateInBps: 448000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
                - LAYER_2:
                    maxBitrateInBps: 384000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].mpegAudio.has_value());
    const auto &profiles = caps.capabilities[0].mpegAudio->profiles;
    ASSERT_EQ(profiles.size(), 2u);
    EXPECT_EQ(profiles.at(firebolt::rialto::MpegAudioProfile::LAYER_1).maxBitrateInBps, 448000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::MpegAudioProfile::LAYER_2).maxBitrateInBps, 384000u);
}

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_Mp3Codec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - MP3:
              profiles:
                - BASE:
                    maxBitrateInBps: 320000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].mp3.has_value());
    EXPECT_EQ(caps.capabilities[0].mp3->base.maxBitrateInBps, 320000u);
    EXPECT_EQ(caps.capabilities[0].mp3->base.maxChannels, 2u);
}

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_DolbyAc3Codec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - DOLBY_AC3:
              profiles:
                - STANDARD:
                    maxBitrateInBps: 640000
                    maxChannels: 6
                    maxSampleRateInHz: 48000
                    maxBitDepth: 24
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].dolbyAc3.has_value());
    const auto &profiles = caps.capabilities[0].dolbyAc3->profiles;
    ASSERT_EQ(profiles.size(), 1u);
    EXPECT_EQ(profiles.at(firebolt::rialto::DolbyAc3Profile::STANDARD).maxBitrateInBps, 640000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::DolbyAc3Profile::STANDARD).maxChannels, 6u);
}

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_DolbyEac3Codec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - DOLBY_EAC3:
              profiles:
                - PLUS:
                    maxBitrateInBps: 6144000
                    maxChannels: 8
                    maxSampleRateInHz: 48000
                    maxBitDepth: 24
                - PLUS_JOC:
                    maxBitrateInBps: 3072000
                    maxChannels: 8
                    maxSampleRateInHz: 48000
                    maxBitDepth: 24
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].dolbyEac3.has_value());
    const auto &profiles = caps.capabilities[0].dolbyEac3->profiles;
    ASSERT_EQ(profiles.size(), 2u);
    EXPECT_EQ(profiles.at(firebolt::rialto::DolbyEac3Profile::PLUS).maxBitrateInBps, 6144000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::DolbyEac3Profile::PLUS_JOC).maxBitrateInBps, 3072000u);
}

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_RealAudioCodec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - REALAUDIO:
              profiles:
                - RA8:
                    maxBitrateInBps: 320000
                    maxChannels: 2
                    maxSampleRateInHz: 44100
                    maxBitDepth: 16
                - RA10:
                    maxBitrateInBps: 640000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].realAudio.has_value());
    const auto &profiles = caps.capabilities[0].realAudio->profiles;
    ASSERT_EQ(profiles.size(), 2u);
    EXPECT_EQ(profiles.at(firebolt::rialto::RealAudioProfile::RA8).maxBitrateInBps, 320000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::RealAudioProfile::RA10).maxBitrateInBps, 640000u);
}

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_UsacCodec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - USAC:
              profiles:
                - BASELINE:
                    maxBitrateInBps: 256000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
                - EXTENDED_HE_AAC:
                    maxBitrateInBps: 512000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].usac.has_value());
    const auto &profiles = caps.capabilities[0].usac->profiles;
    ASSERT_EQ(profiles.size(), 2u);
    EXPECT_EQ(profiles.at(firebolt::rialto::UsacProfile::BASELINE).maxBitrateInBps, 256000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::UsacProfile::EXTENDED_HE_AAC).maxBitrateInBps, 512000u);
}

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_DtsCodec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - DTS:
              profiles:
                - CORE:
                    maxBitrateInBps: 1509750
                    maxChannels: 6
                    maxSampleRateInHz: 48000
                    maxBitDepth: 24
                - HD_HRA:
                    maxBitrateInBps: 6000000
                    maxChannels: 8
                    maxSampleRateInHz: 96000
                    maxBitDepth: 24
                - HD_MA:
                    maxBitrateInBps: 24500000
                    maxChannels: 8
                    maxSampleRateInHz: 192000
                    maxBitDepth: 24
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].dts.has_value());
    const auto &profiles = caps.capabilities[0].dts->profiles;
    ASSERT_EQ(profiles.size(), 3u);
    EXPECT_EQ(profiles.at(firebolt::rialto::DtsProfile::CORE).maxBitrateInBps, 1509750u);
    EXPECT_EQ(profiles.at(firebolt::rialto::DtsProfile::HD_HRA).maxBitrateInBps, 6000000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::DtsProfile::HD_MA).maxBitrateInBps, 24500000u);
}

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_AvsCodec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - AVS:
              profiles:
                - AVS1_PART2:
                    maxBitrateInBps: 192000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
                - AVS2:
                    maxBitrateInBps: 384000
                    maxChannels: 6
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
                - AVS3:
                    maxBitrateInBps: 768000
                    maxChannels: 8
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].avs.has_value());
    const auto &profiles = caps.capabilities[0].avs->profiles;
    ASSERT_EQ(profiles.size(), 3u);
    EXPECT_EQ(profiles.at(firebolt::rialto::AvsProfile::AVS1_PART2).maxBitrateInBps, 192000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::AvsProfile::AVS2).maxBitrateInBps, 384000u);
    EXPECT_EQ(profiles.at(firebolt::rialto::AvsProfile::AVS3).maxBitrateInBps, 768000u);
}

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_UnknownCodecIgnored)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - UNKNOWN_CODEC:
              profiles:
                - PROFILE_X:
                    maxBitrateInBps: 1000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_EQ(caps.capabilities.size(), 1u);
    EXPECT_FALSE(caps.capabilities[0].pcm.has_value());
    EXPECT_FALSE(caps.capabilities[0].aac.has_value());
    EXPECT_FALSE(caps.capabilities[0].dts.has_value());
}

// ---- Audio error-path tests ----

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_MissingRequiredCapField_ReturnsSchemaValidationFailed)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    // maxBitDepth is absent — parseAudioProfileCapability throws
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - PCM:
              profiles:
                - BASE:
                    maxBitrateInBps: 1000000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    EXPECT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED);
}

TEST_F(YamlCppWrapperTest, GetAudioDecoderCapabilities_WrongBaseProfileKey_ReturnsSchemaValidationFailed)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    // parseBaseProfileCapability throws when the key is not "BASE"
    writeYaml(kAudioFilePath, R"yaml(
audiodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - PCM:
              profiles:
                - WRONG_KEY:
                    maxBitrateInBps: 1000000
                    maxChannels: 2
                    maxSampleRateInHz: 48000
                    maxBitDepth: 16
)yaml");
    firebolt::rialto::AudioDecoderCapabilities caps;
    EXPECT_EQ(m_sut.getAudioDecoderCapabilities(caps), DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED);
}

// ---- Video codec happy-path tests ----

TEST_F(YamlCppWrapperTest, GetVideoDecoderCapabilities_H264Codec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kVideoFilePath, R"yaml(
videodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - H264_AVC:
              profiles:
                - H264_BASELINE:
                    maxBitrateInBps: 20000000
                    maxLevel: H264_LEVEL_4
                - H264_MAIN:
                    maxBitrateInBps: 30000000
                    maxLevel: H264_LEVEL_4_1
                - H264_HIGH:
                    maxBitrateInBps: 50000000
                    maxLevel: H264_LEVEL_5_1
              dynamicRange:
                - SDR
                - HDR10
)yaml");
    firebolt::rialto::VideoDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getVideoDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_EQ(caps.capabilities.size(), 1u);
    ASSERT_TRUE(caps.capabilities[0].codecCapabilities.h264.has_value());
    const auto &h264 = *caps.capabilities[0].codecCapabilities.h264;
    ASSERT_EQ(h264.profiles.size(), 3u);
    EXPECT_EQ(h264.profiles[0].type, firebolt::rialto::H264ProfileType::H264_BASELINE);
    EXPECT_EQ(h264.profiles[0].maxLevel, firebolt::rialto::H264Level::H264_LEVEL_4);
    EXPECT_EQ(h264.profiles[1].type, firebolt::rialto::H264ProfileType::H264_MAIN);
    EXPECT_EQ(h264.profiles[1].maxLevel, firebolt::rialto::H264Level::H264_LEVEL_4_1);
    EXPECT_EQ(h264.profiles[2].type, firebolt::rialto::H264ProfileType::H264_HIGH);
    EXPECT_EQ(h264.profiles[2].maxLevel, firebolt::rialto::H264Level::H264_LEVEL_5_1);
    EXPECT_EQ(h264.profiles[2].maxBitrateInBps, 50000000u);
    ASSERT_EQ(h264.dynamicRanges.size(), 2u);
    EXPECT_EQ(h264.dynamicRanges[0], firebolt::rialto::DynamicRange::SDR);
    EXPECT_EQ(h264.dynamicRanges[1], firebolt::rialto::DynamicRange::HDR10);
}

TEST_F(YamlCppWrapperTest, GetVideoDecoderCapabilities_H265Codec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kVideoFilePath, R"yaml(
videodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - H265_HEVC:
              profiles:
                - H265_MAIN:
                    maxBitrateInBps: 40000000
                    maxLevel: H265_LEVEL_5
                - H265_MAIN_10:
                    maxBitrateInBps: 60000000
                    maxLevel: H265_LEVEL_5_1
                - H265_MAIN_10_HDR10:
                    maxBitrateInBps: 80000000
                    maxLevel: H265_LEVEL_5_2
              dynamicRange:
                - SDR
                - HLG
                - HDR10
                - HDR10PLUS
                - DOLBY_VISION
)yaml");
    firebolt::rialto::VideoDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getVideoDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].codecCapabilities.h265.has_value());
    const auto &h265 = *caps.capabilities[0].codecCapabilities.h265;
    ASSERT_EQ(h265.profiles.size(), 3u);
    EXPECT_EQ(h265.profiles[0].type, firebolt::rialto::H265ProfileType::H265_MAIN);
    EXPECT_EQ(h265.profiles[0].maxLevel, firebolt::rialto::H265Level::H265_LEVEL_5);
    EXPECT_EQ(h265.profiles[1].type, firebolt::rialto::H265ProfileType::H265_MAIN_10);
    EXPECT_EQ(h265.profiles[2].type, firebolt::rialto::H265ProfileType::H265_MAIN_10_HDR10);
    EXPECT_EQ(h265.profiles[2].maxLevel, firebolt::rialto::H265Level::H265_LEVEL_5_2);
    ASSERT_EQ(h265.dynamicRanges.size(), 5u);
    EXPECT_EQ(h265.dynamicRanges[0], firebolt::rialto::DynamicRange::SDR);
    EXPECT_EQ(h265.dynamicRanges[1], firebolt::rialto::DynamicRange::HLG);
    EXPECT_EQ(h265.dynamicRanges[2], firebolt::rialto::DynamicRange::HDR10);
    EXPECT_EQ(h265.dynamicRanges[3], firebolt::rialto::DynamicRange::HDR10PLUS);
    EXPECT_EQ(h265.dynamicRanges[4], firebolt::rialto::DynamicRange::DOLBY_VISION);
}

TEST_F(YamlCppWrapperTest, GetVideoDecoderCapabilities_Vp9Codec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kVideoFilePath, R"yaml(
videodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - VP9:
              profiles:
                - VP9_PROFILE_0:
                    maxBitrateInBps: 20000000
                    maxLevel: VP9_LEVEL_5
                - VP9_PROFILE_2:
                    maxBitrateInBps: 40000000
                    maxLevel: VP9_LEVEL_5_1
              dynamicRange:
                - SDR
                - HDR10
)yaml");
    firebolt::rialto::VideoDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getVideoDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].codecCapabilities.vp9.has_value());
    const auto &vp9 = *caps.capabilities[0].codecCapabilities.vp9;
    ASSERT_EQ(vp9.profiles.size(), 2u);
    EXPECT_EQ(vp9.profiles[0].type, firebolt::rialto::Vp9ProfileType::VP9_PROFILE_0);
    EXPECT_EQ(vp9.profiles[0].maxLevel, firebolt::rialto::Vp9Level::VP9_LEVEL_5);
    EXPECT_EQ(vp9.profiles[1].type, firebolt::rialto::Vp9ProfileType::VP9_PROFILE_2);
    EXPECT_EQ(vp9.profiles[1].maxLevel, firebolt::rialto::Vp9Level::VP9_LEVEL_5_1);
    ASSERT_EQ(vp9.dynamicRanges.size(), 2u);
    EXPECT_EQ(vp9.dynamicRanges[0], firebolt::rialto::DynamicRange::SDR);
    EXPECT_EQ(vp9.dynamicRanges[1], firebolt::rialto::DynamicRange::HDR10);
}

TEST_F(YamlCppWrapperTest, GetVideoDecoderCapabilities_Av1Codec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kVideoFilePath, R"yaml(
videodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - AV1:
              profiles:
                - AV1_MAIN:
                    maxBitrateInBps: 100000000
                    maxLevel: AV1_LEVEL_6_0
                - AV1_HIGH:
                    maxBitrateInBps: 200000000
                    maxLevel: AV1_LEVEL_6_2
              dynamicRange:
                - SDR
                - HDR10
                - HDR10PLUS
)yaml");
    firebolt::rialto::VideoDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getVideoDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].codecCapabilities.av1.has_value());
    const auto &av1 = *caps.capabilities[0].codecCapabilities.av1;
    ASSERT_EQ(av1.profiles.size(), 2u);
    EXPECT_EQ(av1.profiles[0].type, firebolt::rialto::Av1ProfileType::AV1_MAIN);
    EXPECT_EQ(av1.profiles[0].maxLevel, firebolt::rialto::Av1Level::AV1_LEVEL_6_0);
    EXPECT_EQ(av1.profiles[0].maxBitrateInBps, 100000000u);
    EXPECT_EQ(av1.profiles[1].type, firebolt::rialto::Av1ProfileType::AV1_HIGH);
    EXPECT_EQ(av1.profiles[1].maxLevel, firebolt::rialto::Av1Level::AV1_LEVEL_6_2);
    ASSERT_EQ(av1.dynamicRanges.size(), 3u);
    EXPECT_EQ(av1.dynamicRanges[2], firebolt::rialto::DynamicRange::HDR10PLUS);
}

TEST_F(YamlCppWrapperTest, GetVideoDecoderCapabilities_Mpeg2Codec_ParsedCorrectly)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kVideoFilePath, R"yaml(
videodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - MPEG2_VIDEO:
              profiles:
                - MPEG2_MAIN:
                    maxBitrateInBps: 80000000
                    maxLevel: MPEG2_LEVEL_HIGH
                - MPEG2_SIMPLE:
                    maxBitrateInBps: 15000000
                    maxLevel: MPEG2_LEVEL_MAIN
              dynamicRange:
                - SDR
)yaml");
    firebolt::rialto::VideoDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getVideoDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_TRUE(caps.capabilities[0].codecCapabilities.mpeg2.has_value());
    const auto &mpeg2 = *caps.capabilities[0].codecCapabilities.mpeg2;
    ASSERT_EQ(mpeg2.profiles.size(), 2u);
    EXPECT_EQ(mpeg2.profiles[0].type, firebolt::rialto::Mpeg2ProfileType::MPEG2_MAIN);
    EXPECT_EQ(mpeg2.profiles[0].maxLevel, firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_HIGH);
    EXPECT_EQ(mpeg2.profiles[0].maxBitrateInBps, 80000000u);
    EXPECT_EQ(mpeg2.profiles[1].type, firebolt::rialto::Mpeg2ProfileType::MPEG2_SIMPLE);
    EXPECT_EQ(mpeg2.profiles[1].maxLevel, firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_MAIN);
    ASSERT_EQ(mpeg2.dynamicRanges.size(), 1u);
    EXPECT_EQ(mpeg2.dynamicRanges[0], firebolt::rialto::DynamicRange::SDR);
}

TEST_F(YamlCppWrapperTest, GetVideoDecoderCapabilities_UnknownCodecIgnored)
{
    if (!s_canWrite)
        GTEST_SKIP() << "Cannot create " << kConfigDir;
    writeYaml(kVideoFilePath, R"yaml(
videodecoder:
  Capabilities:
    - decoder1:
        codecCapabilities:
          - UNKNOWN_VIDEO_CODEC:
              profiles:
                - PROFILE_X:
                    maxBitrateInBps: 1000000
                    maxLevel: SOME_LEVEL
              dynamicRange:
                - SDR
)yaml");
    firebolt::rialto::VideoDecoderCapabilities caps;
    ASSERT_EQ(m_sut.getVideoDecoderCapabilities(caps), DecoderCapabilitiesStatus::OK);
    ASSERT_EQ(caps.capabilities.size(), 1u);
    EXPECT_FALSE(caps.capabilities[0].codecCapabilities.h264.has_value());
    EXPECT_FALSE(caps.capabilities[0].codecCapabilities.h265.has_value());
    EXPECT_FALSE(caps.capabilities[0].codecCapabilities.mpeg2.has_value());
    EXPECT_FALSE(caps.capabilities[0].codecCapabilities.vp9.has_value());
    EXPECT_FALSE(caps.capabilities[0].codecCapabilities.av1.has_value());
}
