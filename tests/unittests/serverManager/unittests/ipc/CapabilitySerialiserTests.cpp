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

#include "CapabilityConverters.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto::common;
using namespace firebolt::rialto::ipc::common;

namespace
{
AudioProfileCapability makeAudioProfile(uint32_t seed)
{
    AudioProfileCapability cap;
    cap.maxBitrateInBps = 100000 + seed;
    cap.maxChannels = 2 + seed;
    cap.maxSampleRateInHz = 48000 + seed;
    cap.maxBitDepth = 16 + seed;
    return cap;
}
} // namespace

// ---------------------------------------------------------------------------
// serialiseAudioCapabilities - top level fields / empty input
// ---------------------------------------------------------------------------
class CapabilitySerialiserAudioTest : public ::testing::Test
{
protected:
    ::firebolt::rialto::AudioCapabilities m_dst;
};

TEST_F(CapabilitySerialiserAudioTest, SetsVersionFieldsWithEmptyCapabilities)
{
    AudioDecoderCapabilities src;
    src.interfaceVersion = "1.0";
    src.schemaVersion = "0.1.0";

    serialiseAudioCapabilities(src, &m_dst);

    EXPECT_EQ(m_dst.interface_version(), "1.0");
    EXPECT_EQ(m_dst.schema_version(), "0.1.0");
    EXPECT_EQ(m_dst.capabilities_size(), 0);
}

// ---------------------------------------------------------------------------
// fillAudioDecoderCapability - simple "base" codecs, each its own struct type.
// Covers the `if (src.X) fillBase(...)` branch + fillProfileCap body for each.
// ---------------------------------------------------------------------------
TEST_F(CapabilitySerialiserAudioTest, FillsPcmBaseCapability)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    cap.pcm = PcmCapability{makeAudioProfile(1)};
    src.capabilities.push_back(cap);

    serialiseAudioCapabilities(src, &m_dst);

    ASSERT_EQ(m_dst.capabilities_size(), 1);
    const auto &pcm = m_dst.capabilities(0).pcm().base();
    EXPECT_EQ(pcm.max_bitrate_in_bps(), 100001u);
    EXPECT_EQ(pcm.max_channels(), 3u);
    EXPECT_EQ(pcm.max_sample_rate_in_hz(), 48001u);
    EXPECT_EQ(pcm.max_bit_depth(), 17u);
}

TEST_F(CapabilitySerialiserAudioTest, FillsAllSimpleBaseCodecs)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    cap.mp3 = Mp3Capability{makeAudioProfile(2)};
    cap.alac = AlacCapability{makeAudioProfile(3)};
    cap.sbc = SbcCapability{makeAudioProfile(4)};
    cap.dolbyAc4 = DolbyAc4Capability{makeAudioProfile(5)};
    cap.dolbyTruehd = DolbyTruehdCapability{makeAudioProfile(6)};
    cap.flac = FlacCapability{makeAudioProfile(7)};
    cap.vorbis = VorbisCapability{makeAudioProfile(8)};
    cap.opus = OpusCapability{makeAudioProfile(9)};
    src.capabilities.push_back(cap);

    serialiseAudioCapabilities(src, &m_dst);

    ASSERT_EQ(m_dst.capabilities_size(), 1);
    const auto &out = m_dst.capabilities(0);
    EXPECT_EQ(out.mp3().base().max_channels(), 4u);
    EXPECT_EQ(out.alac().base().max_channels(), 5u);
    EXPECT_EQ(out.sbc().base().max_channels(), 6u);
    EXPECT_EQ(out.dolby_ac4().base().max_channels(), 7u);
    EXPECT_EQ(out.dolby_truehd().base().max_channels(), 8u);
    EXPECT_EQ(out.flac().base().max_channels(), 9u);
    EXPECT_EQ(out.vorbis().base().max_channels(), 10u);
    EXPECT_EQ(out.opus().base().max_channels(), 11u);
}

TEST_F(CapabilitySerialiserAudioTest, LeavesUnsetCodecsAbsent)
{
    // Covers the "false" side of every `if (src.X)` branch in one go.
    AudioDecoderCapabilities src;
    src.capabilities.push_back(AudioDecoderCapability{});

    serialiseAudioCapabilities(src, &m_dst);

    ASSERT_EQ(m_dst.capabilities_size(), 1);
    EXPECT_FALSE(m_dst.capabilities(0).has_pcm());
    EXPECT_FALSE(m_dst.capabilities(0).has_aac());
    EXPECT_FALSE(m_dst.capabilities(0).has_dolby_ac3());
}

// ---------------------------------------------------------------------------
// dolbyAc3 - inline profile-map loop, fixed to DOLBY_AC3_PROFILE_STANDARD
// (only enum value that exists for DolbyAc3Profile).
// ---------------------------------------------------------------------------
TEST_F(CapabilitySerialiserAudioTest, FillsDolbyAc3Profiles)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    DolbyAc3Capability ac3;
    ac3.profiles[DolbyAc3Profile::STANDARD] = makeAudioProfile(1);
    cap.dolbyAc3 = ac3;
    src.capabilities.push_back(cap);

    serialiseAudioCapabilities(src, &m_dst);

    ASSERT_EQ(m_dst.capabilities(0).dolby_ac3().profiles_size(), 1);
    EXPECT_EQ(m_dst.capabilities(0).dolby_ac3().profiles(0).profile(),
              ::firebolt::rialto::AudioCapabilities::DOLBY_AC3_PROFILE_STANDARD);
}

// ---------------------------------------------------------------------------
// fillNamedProfiles + toProto(...) overloads - parameterised so every enum
// value (including each `default:`) is exercised at least once.
// ---------------------------------------------------------------------------

// --- AAC: LC/HE_V1/HE_V2/ELD/X_HE - LC has no explicit case -> hits default -
struct AacProfileParam
{
    AacProfile input;
    ::firebolt::rialto::AudioCapabilities::AacProfile expected;
};

class AacProfileTest : public ::testing::TestWithParam<AacProfileParam>
{
};

TEST_P(AacProfileTest, MapsToExpectedProtoValue)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    AacCapability aac;
    aac.profiles[GetParam().input] = makeAudioProfile(1);
    cap.aac = aac;
    src.capabilities.push_back(cap);

    ::firebolt::rialto::AudioCapabilities dst;
    serialiseAudioCapabilities(src, &dst);

    ASSERT_EQ(dst.capabilities(0).aac().profiles_size(), 1);
    EXPECT_EQ(dst.capabilities(0).aac().profiles(0).profile(), GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(
    AllValues, AacProfileTest,
    ::testing::Values(AacProfileParam{AacProfile::HE_V1, ::firebolt::rialto::AudioCapabilities::AAC_PROFILE_HE_V1},
                      AacProfileParam{AacProfile::HE_V2, ::firebolt::rialto::AudioCapabilities::AAC_PROFILE_HE_V2},
                      AacProfileParam{AacProfile::ELD, ::firebolt::rialto::AudioCapabilities::AAC_PROFILE_ELD},
                      AacProfileParam{AacProfile::X_HE, ::firebolt::rialto::AudioCapabilities::AAC_PROFILE_X_HE},
                      // LC has no explicit `case` in the switch -> exercises `default:`.
                      AacProfileParam{AacProfile::LC, ::firebolt::rialto::AudioCapabilities::AAC_PROFILE_LC}));

// --- DTS: HD_HRA/HD_MA explicit; CORE has no explicit case -> default -------
struct DtsProfileParam
{
    DtsProfile input;
    ::firebolt::rialto::AudioCapabilities::DtsProfile expected;
};

class DtsProfileTest : public ::testing::TestWithParam<DtsProfileParam>
{
};

TEST_P(DtsProfileTest, MapsToExpectedProtoValue)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    DtsCapability dts;
    dts.profiles[GetParam().input] = makeAudioProfile(1);
    cap.dts = dts;
    src.capabilities.push_back(cap);

    ::firebolt::rialto::AudioCapabilities dst;
    serialiseAudioCapabilities(src, &dst);

    ASSERT_EQ(dst.capabilities(0).dts().profiles_size(), 1);
    EXPECT_EQ(dst.capabilities(0).dts().profiles(0).profile(), GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(
    AllValues, DtsProfileTest,
    ::testing::Values(DtsProfileParam{DtsProfile::HD_HRA, ::firebolt::rialto::AudioCapabilities::DTS_PROFILE_HD_HRA},
                      DtsProfileParam{DtsProfile::HD_MA, ::firebolt::rialto::AudioCapabilities::DTS_PROFILE_HD_MA},
                      // CORE has no explicit case -> exercises `default:`.
                      DtsProfileParam{DtsProfile::CORE, ::firebolt::rialto::AudioCapabilities::DTS_PROFILE_CORE}));

// --- AVS: AVS2/AVS3 explicit; AVS1_PART2 has no explicit case -> default ----
struct AvsProfileParam
{
    AvsProfile input;
    ::firebolt::rialto::AudioCapabilities::AvsProfile expected;
};

class AvsProfileTest : public ::testing::TestWithParam<AvsProfileParam>
{
};

TEST_P(AvsProfileTest, MapsToExpectedProtoValue)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    AvsCapability avs;
    avs.profiles[GetParam().input] = makeAudioProfile(1);
    cap.avs = avs;
    src.capabilities.push_back(cap);

    ::firebolt::rialto::AudioCapabilities dst;
    serialiseAudioCapabilities(src, &dst);

    ASSERT_EQ(dst.capabilities(0).avs().profiles_size(), 1);
    EXPECT_EQ(dst.capabilities(0).avs().profiles(0).profile(), GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(
    AllValues, AvsProfileTest,
    ::testing::Values(AvsProfileParam{AvsProfile::AVS2, ::firebolt::rialto::AudioCapabilities::AVS_PROFILE_AVS2},
                      AvsProfileParam{AvsProfile::AVS3, ::firebolt::rialto::AudioCapabilities::AVS_PROFILE_AVS3},
                      // AVS1_PART2 has no explicit case -> exercises `default:`.
                      AvsProfileParam{AvsProfile::AVS1_PART2,
                                      ::firebolt::rialto::AudioCapabilities::AVS_PROFILE_AVS1_PART2}));

// --- Ternary-based mappers: DolbyEac3, MpegAudio, RealAudio, Usac ------------
// Each needs both branches of its ternary exercised.

TEST_F(CapabilitySerialiserAudioTest, DolbyEac3ProfilePlusJoc)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    DolbyEac3Capability e;
    e.profiles[DolbyEac3Profile::PLUS_JOC] = makeAudioProfile(1);
    cap.dolbyEac3 = e;
    src.capabilities.push_back(cap);

    serialiseAudioCapabilities(src, &m_dst);

    EXPECT_EQ(m_dst.capabilities(0).dolby_eac3().profiles(0).profile(),
              ::firebolt::rialto::AudioCapabilities::DOLBY_EAC3_PROFILE_PLUS_JOC);
}

TEST_F(CapabilitySerialiserAudioTest, DolbyEac3ProfilePlus)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    DolbyEac3Capability e;
    e.profiles[DolbyEac3Profile::PLUS] = makeAudioProfile(1);
    cap.dolbyEac3 = e;
    src.capabilities.push_back(cap);

    serialiseAudioCapabilities(src, &m_dst);

    EXPECT_EQ(m_dst.capabilities(0).dolby_eac3().profiles(0).profile(),
              ::firebolt::rialto::AudioCapabilities::DOLBY_EAC3_PROFILE_PLUS);
}

TEST_F(CapabilitySerialiserAudioTest, MpegAudioProfileLayer2)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    MpegAudioCapability m;
    m.profiles[MpegAudioProfile::LAYER_2] = makeAudioProfile(1);
    cap.mpegAudio = m;
    src.capabilities.push_back(cap);

    serialiseAudioCapabilities(src, &m_dst);

    EXPECT_EQ(m_dst.capabilities(0).mpeg_audio().profiles(0).profile(),
              ::firebolt::rialto::AudioCapabilities::MPEG_AUDIO_PROFILE_LAYER_2);
}

TEST_F(CapabilitySerialiserAudioTest, MpegAudioProfileLayer1)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    MpegAudioCapability m;
    m.profiles[MpegAudioProfile::LAYER_1] = makeAudioProfile(1);
    cap.mpegAudio = m;
    src.capabilities.push_back(cap);

    serialiseAudioCapabilities(src, &m_dst);

    EXPECT_EQ(m_dst.capabilities(0).mpeg_audio().profiles(0).profile(),
              ::firebolt::rialto::AudioCapabilities::MPEG_AUDIO_PROFILE_LAYER_1);
}

TEST_F(CapabilitySerialiserAudioTest, RealAudioProfileRa10)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    RealAudioCapability r;
    r.profiles[RealAudioProfile::RA10] = makeAudioProfile(1);
    cap.realAudio = r;
    src.capabilities.push_back(cap);

    serialiseAudioCapabilities(src, &m_dst);

    EXPECT_EQ(m_dst.capabilities(0).real_audio().profiles(0).profile(),
              ::firebolt::rialto::AudioCapabilities::REALAUDIO_PROFILE_RA10);
}

TEST_F(CapabilitySerialiserAudioTest, RealAudioProfileRa8)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    RealAudioCapability r;
    r.profiles[RealAudioProfile::RA8] = makeAudioProfile(1);
    cap.realAudio = r;
    src.capabilities.push_back(cap);

    serialiseAudioCapabilities(src, &m_dst);

    EXPECT_EQ(m_dst.capabilities(0).real_audio().profiles(0).profile(),
              ::firebolt::rialto::AudioCapabilities::REALAUDIO_PROFILE_RA8);
}

TEST_F(CapabilitySerialiserAudioTest, UsacProfileExtendedHeAac)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    UsacCapability u;
    u.profiles[UsacProfile::EXTENDED_HE_AAC] = makeAudioProfile(1);
    cap.usac = u;
    src.capabilities.push_back(cap);

    serialiseAudioCapabilities(src, &m_dst);

    EXPECT_EQ(m_dst.capabilities(0).usac().profiles(0).profile(),
              ::firebolt::rialto::AudioCapabilities::USAC_PROFILE_EXTENDED_HE_AAC);
}

TEST_F(CapabilitySerialiserAudioTest, UsacProfileBaseline)
{
    AudioDecoderCapabilities src;
    AudioDecoderCapability cap;
    UsacCapability u;
    u.profiles[UsacProfile::BASELINE] = makeAudioProfile(1);
    cap.usac = u;
    src.capabilities.push_back(cap);

    serialiseAudioCapabilities(src, &m_dst);

    EXPECT_EQ(m_dst.capabilities(0).usac().profiles(0).profile(),
              ::firebolt::rialto::AudioCapabilities::USAC_PROFILE_BASELINE);
}

// ---------------------------------------------------------------------------
// serialiseVideoCapabilities
// ---------------------------------------------------------------------------
class CapabilitySerialiserVideoTest : public ::testing::Test
{
protected:
    ::firebolt::rialto::VideoCapabilities m_dst;
};

TEST_F(CapabilitySerialiserVideoTest, SetsVersionFieldsWithEmptyCapabilities)
{
    VideoDecoderCapabilities src;
    src.interfaceVersion = "1.0";
    src.schemaVersion = "0.1.0";

    serialiseVideoCapabilities(src, &m_dst);

    EXPECT_EQ(m_dst.interface_version(), "1.0");
    EXPECT_EQ(m_dst.schema_version(), "0.1.0");
    EXPECT_EQ(m_dst.capabilities_size(), 0);
}

TEST_F(CapabilitySerialiserVideoTest, LeavesUnsetCodecsAbsent)
{
    VideoDecoderCapabilities src;
    src.capabilities.push_back(VideoDecoderCapability{});

    serialiseVideoCapabilities(src, &m_dst);

    ASSERT_EQ(m_dst.capabilities_size(), 1);
    EXPECT_FALSE(m_dst.capabilities(0).codec_capabilities().has_mpeg2());
    EXPECT_FALSE(m_dst.capabilities(0).codec_capabilities().has_h264());
    EXPECT_FALSE(m_dst.capabilities(0).codec_capabilities().has_h265());
    EXPECT_FALSE(m_dst.capabilities(0).codec_capabilities().has_vp9());
    EXPECT_FALSE(m_dst.capabilities(0).codec_capabilities().has_av1());
}

// --- Mpeg2 ------------------------------------------------------------
// The proto's exact enum constant names for profile/level types aren't
// knowable from CapabilitySerialiser.cpp alone (unlike DynamicRange, whose
// constants appear verbatim there), so correctness is verified by casting
// the proto value back to the common-namespace enum rather than guessing
// proto constant identifiers.
TEST_F(CapabilitySerialiserVideoTest, FillsMpeg2ProfilesAndDynamicRanges)
{
    VideoDecoderCapabilities src;
    VideoDecoderCapability cap;
    Mpeg2CodecCapability mpeg2;
    mpeg2.profiles.push_back(Mpeg2Profile{Mpeg2ProfileType::MPEG2_MAIN, Mpeg2Level::MPEG2_LEVEL_HIGH, 5000000});
    mpeg2.dynamicRanges = {DynamicRange::HLG, DynamicRange::HDR10, DynamicRange::HDR10PLUS, DynamicRange::DOLBY_VISION,
                           DynamicRange::SDR};
    cap.codecCapabilities.mpeg2 = mpeg2;
    src.capabilities.push_back(cap);

    serialiseVideoCapabilities(src, &m_dst);

    const auto &out = m_dst.capabilities(0).codec_capabilities().mpeg2();
    ASSERT_EQ(out.profiles_size(), 1);
    EXPECT_EQ(static_cast<Mpeg2ProfileType>(out.profiles(0).type()), Mpeg2ProfileType::MPEG2_MAIN);
    EXPECT_EQ(static_cast<Mpeg2Level>(out.profiles(0).max_level()), Mpeg2Level::MPEG2_LEVEL_HIGH);
    EXPECT_EQ(out.profiles(0).max_bitrate_in_bps(), 5000000u);

    ASSERT_EQ(out.dynamic_ranges_size(), 5);
    EXPECT_EQ(out.dynamic_ranges(0), ::firebolt::rialto::VideoCapabilities::DYNAMIC_RANGE_HLG);
    EXPECT_EQ(out.dynamic_ranges(1), ::firebolt::rialto::VideoCapabilities::DYNAMIC_RANGE_HDR10);
    EXPECT_EQ(out.dynamic_ranges(2), ::firebolt::rialto::VideoCapabilities::DYNAMIC_RANGE_HDR10PLUS);
    EXPECT_EQ(out.dynamic_ranges(3), ::firebolt::rialto::VideoCapabilities::DYNAMIC_RANGE_DOLBY_VISION);
    // SDR has no explicit case in toDR() -> exercises `default:`.
    EXPECT_EQ(out.dynamic_ranges(4), ::firebolt::rialto::VideoCapabilities::DYNAMIC_RANGE_SDR);
}

// --- H264 -----------------------------------------------------------------
TEST_F(CapabilitySerialiserVideoTest, FillsH264ProfilesAndDynamicRanges)
{
    VideoDecoderCapabilities src;
    VideoDecoderCapability cap;
    H264CodecCapability h264;
    h264.profiles.push_back(H264Profile{H264ProfileType::H264_HIGH, H264Level::H264_LEVEL_5_1, 8000000});
    h264.dynamicRanges = {DynamicRange::HLG};
    cap.codecCapabilities.h264 = h264;
    src.capabilities.push_back(cap);

    serialiseVideoCapabilities(src, &m_dst);

    const auto &out = m_dst.capabilities(0).codec_capabilities().h264();
    ASSERT_EQ(out.profiles_size(), 1);
    EXPECT_EQ(static_cast<H264ProfileType>(out.profiles(0).type()), H264ProfileType::H264_HIGH);
    EXPECT_EQ(static_cast<H264Level>(out.profiles(0).max_level()), H264Level::H264_LEVEL_5_1);
    ASSERT_EQ(out.dynamic_ranges_size(), 1);
}

// --- H265 -----------------------------------------------------------------
TEST_F(CapabilitySerialiserVideoTest, FillsH265ProfilesAndDynamicRanges)
{
    VideoDecoderCapabilities src;
    VideoDecoderCapability cap;
    H265CodecCapability h265;
    h265.profiles.push_back(H265Profile{H265ProfileType::H265_MAIN_10_HDR10, H265Level::H265_LEVEL_6, 12000000});
    h265.dynamicRanges = {DynamicRange::HDR10};
    cap.codecCapabilities.h265 = h265;
    src.capabilities.push_back(cap);

    serialiseVideoCapabilities(src, &m_dst);

    const auto &out = m_dst.capabilities(0).codec_capabilities().h265();
    ASSERT_EQ(out.profiles_size(), 1);
    EXPECT_EQ(static_cast<H265ProfileType>(out.profiles(0).type()), H265ProfileType::H265_MAIN_10_HDR10);
    ASSERT_EQ(out.dynamic_ranges_size(), 1);
}

// --- Vp9 ------------------------------------------------------------------
TEST_F(CapabilitySerialiserVideoTest, FillsVp9ProfilesAndDynamicRanges)
{
    VideoDecoderCapabilities src;
    VideoDecoderCapability cap;
    Vp9CodecCapability vp9;
    vp9.profiles.push_back(Vp9Profile{Vp9ProfileType::VP9_PROFILE_2, Vp9Level::VP9_LEVEL_4, 4000000});
    vp9.dynamicRanges = {DynamicRange::HDR10PLUS};
    cap.codecCapabilities.vp9 = vp9;
    src.capabilities.push_back(cap);

    serialiseVideoCapabilities(src, &m_dst);

    const auto &out = m_dst.capabilities(0).codec_capabilities().vp9();
    ASSERT_EQ(out.profiles_size(), 1);
    EXPECT_EQ(static_cast<Vp9ProfileType>(out.profiles(0).type()), Vp9ProfileType::VP9_PROFILE_2);
    ASSERT_EQ(out.dynamic_ranges_size(), 1);
}

// --- Av1 ------------------------------------------------------------------
TEST_F(CapabilitySerialiserVideoTest, FillsAv1ProfilesAndDynamicRanges)
{
    VideoDecoderCapabilities src;
    VideoDecoderCapability cap;
    Av1CodecCapability av1;
    av1.profiles.push_back(Av1Profile{Av1ProfileType::AV1_HIGH, Av1Level::AV1_LEVEL_5_0, 20000000});
    av1.dynamicRanges = {DynamicRange::DOLBY_VISION};
    cap.codecCapabilities.av1 = av1;
    src.capabilities.push_back(cap);

    serialiseVideoCapabilities(src, &m_dst);

    const auto &out = m_dst.capabilities(0).codec_capabilities().av1();
    ASSERT_EQ(out.profiles_size(), 1);
    EXPECT_EQ(static_cast<Av1ProfileType>(out.profiles(0).type()), Av1ProfileType::AV1_HIGH);
    ASSERT_EQ(out.dynamic_ranges_size(), 1);
}

// --- All codecs present together (loop-with-multiple-entries case) --------
TEST_F(CapabilitySerialiserVideoTest, FillsMultipleCapabilitiesEntries)
{
    VideoDecoderCapabilities src;

    VideoDecoderCapability cap1;
    cap1.codecCapabilities.mpeg2 =
        Mpeg2CodecCapability{{Mpeg2Profile{Mpeg2ProfileType::MPEG2_SIMPLE, Mpeg2Level::MPEG2_LEVEL_LOW, 1000000}},
                             {DynamicRange::SDR}};
    src.capabilities.push_back(cap1);

    VideoDecoderCapability cap2;
    cap2.codecCapabilities.av1 =
        Av1CodecCapability{{Av1Profile{Av1ProfileType::AV1_MAIN, Av1Level::AV1_LEVEL_4_0, 2000000}},
                           {DynamicRange::HDR10}};
    src.capabilities.push_back(cap2);

    serialiseVideoCapabilities(src, &m_dst);

    ASSERT_EQ(m_dst.capabilities_size(), 2);
    EXPECT_TRUE(m_dst.capabilities(0).codec_capabilities().has_mpeg2());
    EXPECT_TRUE(m_dst.capabilities(1).codec_capabilities().has_av1());
}
