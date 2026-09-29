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

// NOTE ON INFERRED PROTO CONSTANTS:
// Most proto enum constant names used below appear literally in CapabilityDeserialiser.cpp
// (e.g. H264_PROFILE_MAIN, VP9_LEVEL_2_1, AV1_LEVEL_5_0) and are confirmed correct.
// A handful of "default-branch" constants are never referenced literally in the source
// (e.g. the proto constant for H264ProfileType::H264_BASELINE, since the code only compares
// against MAIN/HIGH and falls through to BASELINE by default). These are inferred from the
// otherwise-universal naming convention <COMMON_ENUM>_<VALUE> seen in every confirmed constant
// (e.g. common Mpeg2ProfileType::MPEG2_SIMPLE -> proto MPEG2_PROFILE_SIMPLE). They are marked
// with an "inferred" comment below - verify against the generated .pb.h if the build fails on
// just these lines.

#include "CapabilityConverters.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto::common;
using namespace firebolt::rialto::ipc::common;
using AudioCap = firebolt::rialto::AudioCapabilities;
using VideoCap = firebolt::rialto::VideoCapabilities;

// ---------------------------------------------------------------------------
// deserialiseAudioCapabilities - top level fields / empty input
// ---------------------------------------------------------------------------
class CapabilityDeserialiserAudioTest : public ::testing::Test
{
protected:
    AudioCap m_src;
};

TEST_F(CapabilityDeserialiserAudioTest, SetsVersionFieldsWithEmptyCapabilities)
{
    m_src.set_interface_version("1.0");
    m_src.set_schema_version("0.1.0");

    auto result = deserialiseAudioCapabilities(m_src);

    EXPECT_EQ(result.interfaceVersion, "1.0");
    EXPECT_EQ(result.schemaVersion, "0.1.0");
    EXPECT_TRUE(result.capabilities.empty());
}

// ---------------------------------------------------------------------------
// toAudioDecoderCapability - simple "base" codecs
// ---------------------------------------------------------------------------
TEST_F(CapabilityDeserialiserAudioTest, ParsesPcmBaseCapability)
{
    auto *cap = m_src.add_capabilities();
    auto *base = cap->mutable_pcm()->mutable_base();
    base->set_max_bitrate_in_bps(100001);
    base->set_max_channels(3);
    base->set_max_sample_rate_in_hz(48001);
    base->set_max_bit_depth(17);

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_EQ(result.capabilities.size(), 1u);
    ASSERT_TRUE(result.capabilities[0].pcm.has_value());
    EXPECT_EQ(result.capabilities[0].pcm->base.maxBitrateInBps, 100001u);
    EXPECT_EQ(result.capabilities[0].pcm->base.maxChannels, 3u);
    EXPECT_EQ(result.capabilities[0].pcm->base.maxSampleRateInHz, 48001u);
    EXPECT_EQ(result.capabilities[0].pcm->base.maxBitDepth, 17u);
}

TEST_F(CapabilityDeserialiserAudioTest, ParsesAllSimpleBaseCodecs)
{
    auto *cap = m_src.add_capabilities();
    cap->mutable_mp3()->mutable_base()->set_max_channels(4);
    cap->mutable_alac()->mutable_base()->set_max_channels(5);
    cap->mutable_sbc()->mutable_base()->set_max_channels(6);
    cap->mutable_dolby_ac4()->mutable_base()->set_max_channels(7);
    cap->mutable_dolby_truehd()->mutable_base()->set_max_channels(8);
    cap->mutable_flac()->mutable_base()->set_max_channels(9);
    cap->mutable_vorbis()->mutable_base()->set_max_channels(10);
    cap->mutable_opus()->mutable_base()->set_max_channels(11);

    auto result = deserialiseAudioCapabilities(m_src);

    const auto &out = result.capabilities[0];
    ASSERT_TRUE(out.mp3.has_value());
    EXPECT_EQ(out.mp3->base.maxChannels, 4u);
    ASSERT_TRUE(out.alac.has_value());
    EXPECT_EQ(out.alac->base.maxChannels, 5u);
    ASSERT_TRUE(out.sbc.has_value());
    EXPECT_EQ(out.sbc->base.maxChannels, 6u);
    ASSERT_TRUE(out.dolbyAc4.has_value());
    EXPECT_EQ(out.dolbyAc4->base.maxChannels, 7u);
    ASSERT_TRUE(out.dolbyTruehd.has_value());
    EXPECT_EQ(out.dolbyTruehd->base.maxChannels, 8u);
    ASSERT_TRUE(out.flac.has_value());
    EXPECT_EQ(out.flac->base.maxChannels, 9u);
    ASSERT_TRUE(out.vorbis.has_value());
    EXPECT_EQ(out.vorbis->base.maxChannels, 10u);
    ASSERT_TRUE(out.opus.has_value());
    EXPECT_EQ(out.opus->base.maxChannels, 11u);
}

TEST_F(CapabilityDeserialiserAudioTest, LeavesUnsetCodecsAsNullopt)
{
    // Covers the "false" side of every `if (src.has_X())` branch in one go.
    m_src.add_capabilities();

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_EQ(result.capabilities.size(), 1u);
    EXPECT_FALSE(result.capabilities[0].pcm.has_value());
    EXPECT_FALSE(result.capabilities[0].aac.has_value());
    EXPECT_FALSE(result.capabilities[0].dolbyAc3.has_value());
}

// ---------------------------------------------------------------------------
// dolbyAc3 - special-cased loop: fixed to DolbyAc3Profile::STANDARD, and only
// checks entry.has_capability() (NOT entry.has_profile(), unlike every other
// named-profile codec below). These tests pin that actual behaviour.
// ---------------------------------------------------------------------------
TEST_F(CapabilityDeserialiserAudioTest, DolbyAc3EntryWithCapabilityIsParsedRegardlessOfProfileField)
{
    auto *cap = m_src.add_capabilities();
    auto *entry = cap->mutable_dolby_ac3()->add_profiles();
    // Deliberately NOT calling entry->set_profile(...) here - current code doesn't check has_profile().
    entry->mutable_capability()->set_max_channels(6);

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_TRUE(result.capabilities[0].dolbyAc3.has_value());
    ASSERT_EQ(result.capabilities[0].dolbyAc3->profiles.count(DolbyAc3Profile::STANDARD), 1u);
    EXPECT_EQ(result.capabilities[0].dolbyAc3->profiles.at(DolbyAc3Profile::STANDARD).maxChannels, 6u);
}

TEST_F(CapabilityDeserialiserAudioTest, DolbyAc3EntryWithoutCapabilityIsSkipped)
{
    auto *cap = m_src.add_capabilities();
    cap->mutable_dolby_ac3()->add_profiles(); // no capability set

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_TRUE(result.capabilities[0].dolbyAc3.has_value());
    EXPECT_TRUE(result.capabilities[0].dolbyAc3->profiles.empty());
}

// ---------------------------------------------------------------------------
// fillProfileMap + fromProto(...) overloads for named-profile codecs.
// Unlike dolbyAc3, these DO require both has_profile() AND has_capability().
// ---------------------------------------------------------------------------

// --- AAC: HE_V1/HE_V2/ELD/X_HE explicit; LC has no explicit case -> default -
namespace
{
// Anonymous namespace gives this file's own AacProfileParam internal linkage, so it
// doesn't collide with the identically-named struct in CapabilitySerialiserTests.cpp
// (fixes cppcheck's ctuOneDefinitionRuleViolation across the two test files).
struct AacProfileParam
{
    AudioCap::AacProfile input;
    AacProfile expected;
};
} // namespace

class AacFromProtoTest : public ::testing::TestWithParam<AacProfileParam>
{
};

TEST_P(AacFromProtoTest, MapsToExpectedCommonValue)
{
    AudioCap src;
    auto *cap = src.add_capabilities();
    auto *entry = cap->mutable_aac()->add_profiles();
    entry->set_profile(GetParam().input);
    entry->mutable_capability(); // must be present for fillProfileMap to accept the entry

    auto result = deserialiseAudioCapabilities(src);

    ASSERT_TRUE(result.capabilities[0].aac.has_value());
    ASSERT_EQ(result.capabilities[0].aac->profiles.count(GetParam().expected), 1u);
}

INSTANTIATE_TEST_SUITE_P(AllValues, AacFromProtoTest,
                         ::testing::Values(AacProfileParam{AudioCap::AAC_PROFILE_HE_V1, AacProfile::HE_V1},
                                           AacProfileParam{AudioCap::AAC_PROFILE_HE_V2, AacProfile::HE_V2},
                                           AacProfileParam{AudioCap::AAC_PROFILE_ELD, AacProfile::ELD},
                                           AacProfileParam{AudioCap::AAC_PROFILE_X_HE, AacProfile::X_HE},
                                           // LC has no explicit case -> exercises `default:`.
                                           AacProfileParam{AudioCap::AAC_PROFILE_LC, AacProfile::LC}));

// --- DTS: HD_HRA/HD_MA explicit; CORE has no explicit case -> default -------
namespace
{
// See note above AacProfileParam: anonymous namespace avoids the ODR collision.
struct DtsProfileParam
{
    AudioCap::DtsProfile input;
    DtsProfile expected;
};
} // namespace

class DtsFromProtoTest : public ::testing::TestWithParam<DtsProfileParam>
{
};

TEST_P(DtsFromProtoTest, MapsToExpectedCommonValue)
{
    AudioCap src;
    auto *cap = src.add_capabilities();
    auto *entry = cap->mutable_dts()->add_profiles();
    entry->set_profile(GetParam().input);
    entry->mutable_capability();

    auto result = deserialiseAudioCapabilities(src);

    ASSERT_TRUE(result.capabilities[0].dts.has_value());
    ASSERT_EQ(result.capabilities[0].dts->profiles.count(GetParam().expected), 1u);
}

INSTANTIATE_TEST_SUITE_P(AllValues, DtsFromProtoTest,
                         ::testing::Values(DtsProfileParam{AudioCap::DTS_PROFILE_HD_HRA, DtsProfile::HD_HRA},
                                           DtsProfileParam{AudioCap::DTS_PROFILE_HD_MA, DtsProfile::HD_MA},
                                           // CORE has no explicit case -> exercises `default:`.
                                           DtsProfileParam{AudioCap::DTS_PROFILE_CORE, DtsProfile::CORE}));

// --- AVS: AVS2/AVS3 explicit; AVS1_PART2 has no explicit case -> default ----
namespace
{
// See note above AacProfileParam: anonymous namespace avoids the ODR collision.
struct AvsProfileParam
{
    AudioCap::AvsProfile input;
    AvsProfile expected;
};
} // namespace

class AvsFromProtoTest : public ::testing::TestWithParam<AvsProfileParam>
{
};

TEST_P(AvsFromProtoTest, MapsToExpectedCommonValue)
{
    AudioCap src;
    auto *cap = src.add_capabilities();
    auto *entry = cap->mutable_avs()->add_profiles();
    entry->set_profile(GetParam().input);
    entry->mutable_capability();

    auto result = deserialiseAudioCapabilities(src);

    ASSERT_TRUE(result.capabilities[0].avs.has_value());
    ASSERT_EQ(result.capabilities[0].avs->profiles.count(GetParam().expected), 1u);
}

INSTANTIATE_TEST_SUITE_P(AllValues, AvsFromProtoTest,
                         ::testing::Values(AvsProfileParam{AudioCap::AVS_PROFILE_AVS2, AvsProfile::AVS2},
                                           AvsProfileParam{AudioCap::AVS_PROFILE_AVS3, AvsProfile::AVS3},
                                           // AVS1_PART2 has no explicit case -> exercises `default:`.
                                           AvsProfileParam{AudioCap::AVS_PROFILE_AVS1_PART2, AvsProfile::AVS1_PART2}));

// --- Ternary-based mappers: DolbyEac3, MpegAudio, RealAudio, Usac ------------
TEST_F(CapabilityDeserialiserAudioTest, DolbyEac3ProfilePlusJoc)
{
    auto *cap = m_src.add_capabilities();
    auto *entry = cap->mutable_dolby_eac3()->add_profiles();
    entry->set_profile(AudioCap::DOLBY_EAC3_PROFILE_PLUS_JOC);
    entry->mutable_capability();

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_EQ(result.capabilities[0].dolbyEac3->profiles.count(DolbyEac3Profile::PLUS_JOC), 1u);
}

TEST_F(CapabilityDeserialiserAudioTest, DolbyEac3ProfilePlus)
{
    auto *cap = m_src.add_capabilities();
    auto *entry = cap->mutable_dolby_eac3()->add_profiles();
    entry->set_profile(AudioCap::DOLBY_EAC3_PROFILE_PLUS);
    entry->mutable_capability();

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_EQ(result.capabilities[0].dolbyEac3->profiles.count(DolbyEac3Profile::PLUS), 1u);
}

TEST_F(CapabilityDeserialiserAudioTest, MpegAudioProfileLayer2)
{
    auto *cap = m_src.add_capabilities();
    auto *entry = cap->mutable_mpeg_audio()->add_profiles();
    entry->set_profile(AudioCap::MPEG_AUDIO_PROFILE_LAYER_2);
    entry->mutable_capability();

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_EQ(result.capabilities[0].mpegAudio->profiles.count(MpegAudioProfile::LAYER_2), 1u);
}

TEST_F(CapabilityDeserialiserAudioTest, MpegAudioProfileLayer1)
{
    auto *cap = m_src.add_capabilities();
    auto *entry = cap->mutable_mpeg_audio()->add_profiles();
    entry->set_profile(AudioCap::MPEG_AUDIO_PROFILE_LAYER_1);
    entry->mutable_capability();

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_EQ(result.capabilities[0].mpegAudio->profiles.count(MpegAudioProfile::LAYER_1), 1u);
}

TEST_F(CapabilityDeserialiserAudioTest, RealAudioProfileRa10)
{
    auto *cap = m_src.add_capabilities();
    auto *entry = cap->mutable_real_audio()->add_profiles();
    entry->set_profile(AudioCap::REALAUDIO_PROFILE_RA10);
    entry->mutable_capability();

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_EQ(result.capabilities[0].realAudio->profiles.count(RealAudioProfile::RA10), 1u);
}

TEST_F(CapabilityDeserialiserAudioTest, RealAudioProfileRa8)
{
    auto *cap = m_src.add_capabilities();
    auto *entry = cap->mutable_real_audio()->add_profiles();
    entry->set_profile(AudioCap::REALAUDIO_PROFILE_RA8);
    entry->mutable_capability();

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_EQ(result.capabilities[0].realAudio->profiles.count(RealAudioProfile::RA8), 1u);
}

TEST_F(CapabilityDeserialiserAudioTest, UsacProfileExtendedHeAac)
{
    auto *cap = m_src.add_capabilities();
    auto *entry = cap->mutable_usac()->add_profiles();
    entry->set_profile(AudioCap::USAC_PROFILE_EXTENDED_HE_AAC);
    entry->mutable_capability();

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_EQ(result.capabilities[0].usac->profiles.count(UsacProfile::EXTENDED_HE_AAC), 1u);
}

TEST_F(CapabilityDeserialiserAudioTest, UsacProfileBaseline)
{
    auto *cap = m_src.add_capabilities();
    auto *entry = cap->mutable_usac()->add_profiles();
    entry->set_profile(AudioCap::USAC_PROFILE_BASELINE);
    entry->mutable_capability();

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_EQ(result.capabilities[0].usac->profiles.count(UsacProfile::BASELINE), 1u);
}

// --- fillProfileMap filtering: entries missing profile/capability are skipped
TEST_F(CapabilityDeserialiserAudioTest, SkipsEntryMissingProfileField)
{
    auto *cap = m_src.add_capabilities();
    auto *entry = cap->mutable_aac()->add_profiles();
    entry->mutable_capability(); // capability set, profile NOT set

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_TRUE(result.capabilities[0].aac.has_value());
    EXPECT_TRUE(result.capabilities[0].aac->profiles.empty());
}

TEST_F(CapabilityDeserialiserAudioTest, SkipsEntryMissingCapabilityField)
{
    auto *cap = m_src.add_capabilities();
    auto *entry = cap->mutable_aac()->add_profiles();
    entry->set_profile(AudioCap::AAC_PROFILE_HE_V1); // profile set, capability NOT set

    auto result = deserialiseAudioCapabilities(m_src);

    ASSERT_TRUE(result.capabilities[0].aac.has_value());
    EXPECT_TRUE(result.capabilities[0].aac->profiles.empty());
}

// ---------------------------------------------------------------------------
// deserialiseVideoCapabilities
// ---------------------------------------------------------------------------
class CapabilityDeserialiserVideoTest : public ::testing::Test
{
protected:
    VideoCap m_src;
};

TEST_F(CapabilityDeserialiserVideoTest, SetsVersionFieldsWithEmptyCapabilities)
{
    m_src.set_interface_version("1.0");
    m_src.set_schema_version("0.1.0");

    auto result = deserialiseVideoCapabilities(m_src);

    EXPECT_EQ(result.interfaceVersion, "1.0");
    EXPECT_EQ(result.schemaVersion, "0.1.0");
    EXPECT_TRUE(result.capabilities.empty());
}

TEST_F(CapabilityDeserialiserVideoTest, ReturnsEmptyCodecCapabilitiesWhenNotSet)
{
    // Covers the early `if (!src.has_codec_capabilities()) return dst;` branch.
    m_src.add_capabilities();

    auto result = deserialiseVideoCapabilities(m_src);

    ASSERT_EQ(result.capabilities.size(), 1u);
    EXPECT_FALSE(result.capabilities[0].codecCapabilities.mpeg2.has_value());
    EXPECT_FALSE(result.capabilities[0].codecCapabilities.h264.has_value());
}

TEST_F(CapabilityDeserialiserVideoTest, LeavesUnsetCodecsAsNulloptWhenCodecCapabilitiesPresent)
{
    auto *cap = m_src.add_capabilities();
    cap->mutable_codec_capabilities(); // present but no codec fields set

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &cc = result.capabilities[0].codecCapabilities;
    EXPECT_FALSE(cc.mpeg2.has_value());
    EXPECT_FALSE(cc.h264.has_value());
    EXPECT_FALSE(cc.h265.has_value());
    EXPECT_FALSE(cc.vp9.has_value());
    EXPECT_FALSE(cc.av1.has_value());
}

// --- fromDR: explicit cases + default (SDR) --------------------------------
TEST_F(CapabilityDeserialiserVideoTest, ParsesAllDynamicRangeValues)
{
    auto *cap = m_src.add_capabilities();
    auto *mpeg2 = cap->mutable_codec_capabilities()->mutable_mpeg2();
    mpeg2->add_dynamic_ranges(VideoCap::DYNAMIC_RANGE_HLG);
    mpeg2->add_dynamic_ranges(VideoCap::DYNAMIC_RANGE_HDR10);
    mpeg2->add_dynamic_ranges(VideoCap::DYNAMIC_RANGE_HDR10PLUS);
    mpeg2->add_dynamic_ranges(VideoCap::DYNAMIC_RANGE_DOLBY_VISION);
    mpeg2->add_dynamic_ranges(VideoCap::DYNAMIC_RANGE_SDR); // exercises `default:` in fromDR

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &ranges = result.capabilities[0].codecCapabilities.mpeg2->dynamicRanges;
    ASSERT_EQ(ranges.size(), 5u);
    EXPECT_EQ(ranges[0], DynamicRange::HLG);
    EXPECT_EQ(ranges[1], DynamicRange::HDR10);
    EXPECT_EQ(ranges[2], DynamicRange::HDR10PLUS);
    EXPECT_EQ(ranges[3], DynamicRange::DOLBY_VISION);
    EXPECT_EQ(ranges[4], DynamicRange::SDR);
}

// --- Mpeg2: profile ternary + level switch ---------------------------------
TEST_F(CapabilityDeserialiserVideoTest, ParsesMpeg2ProfileAndLevel)
{
    auto *cap = m_src.add_capabilities();
    auto *mpeg2 = cap->mutable_codec_capabilities()->mutable_mpeg2();
    auto *p = mpeg2->add_profiles();
    p->set_type(VideoCap::MPEG2_PROFILE_SIMPLE);
    p->set_max_level(VideoCap::MPEG2_LEVEL_HIGH);
    p->set_max_bitrate_in_bps(5000000);

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &out = result.capabilities[0].codecCapabilities.mpeg2->profiles;
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, Mpeg2ProfileType::MPEG2_SIMPLE);
    EXPECT_EQ(out[0].maxLevel, Mpeg2Level::MPEG2_LEVEL_HIGH);
    EXPECT_EQ(out[0].maxBitrateInBps, 5000000u);
}

TEST_F(CapabilityDeserialiserVideoTest, Mpeg2ProfileAndLevelDefaults)
{
    auto *cap = m_src.add_capabilities();
    auto *mpeg2 = cap->mutable_codec_capabilities()->mutable_mpeg2();
    auto *p = mpeg2->add_profiles();
    p->set_type(VideoCap::MPEG2_PROFILE_MAIN);   // inferred proto constant name (see file header note)
    p->set_max_level(VideoCap::MPEG2_LEVEL_LOW); // inferred proto constant name (see file header note)

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &out = result.capabilities[0].codecCapabilities.mpeg2->profiles;
    EXPECT_EQ(out[0].type, Mpeg2ProfileType::MPEG2_MAIN);
    EXPECT_EQ(out[0].maxLevel, Mpeg2Level::MPEG2_LEVEL_LOW);
}

// --- H264: profile switch (2 explicit + default) + level switch (6 explicit + default)
TEST_F(CapabilityDeserialiserVideoTest, ParsesH264ProfileAndLevel)
{
    auto *cap = m_src.add_capabilities();
    auto *h264 = cap->mutable_codec_capabilities()->mutable_h264();
    auto *p = h264->add_profiles();
    p->set_type(VideoCap::H264_PROFILE_HIGH);
    p->set_max_level(VideoCap::H264_LEVEL_5_1);

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &out = result.capabilities[0].codecCapabilities.h264->profiles;
    EXPECT_EQ(out[0].type, H264ProfileType::H264_HIGH);
    EXPECT_EQ(out[0].maxLevel, H264Level::H264_LEVEL_5_1);
}

TEST_F(CapabilityDeserialiserVideoTest, H264ProfileAndLevelDefaults)
{
    auto *cap = m_src.add_capabilities();
    auto *h264 = cap->mutable_codec_capabilities()->mutable_h264();
    auto *p = h264->add_profiles();
    p->set_type(VideoCap::H264_PROFILE_BASELINE); // inferred proto constant name
    p->set_max_level(VideoCap::H264_LEVEL_3);     // inferred proto constant name

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &out = result.capabilities[0].codecCapabilities.h264->profiles;
    EXPECT_EQ(out[0].type, H264ProfileType::H264_BASELINE);
    EXPECT_EQ(out[0].maxLevel, H264Level::H264_LEVEL_3);
}

// --- H265 ------------------------------------------------------------------
TEST_F(CapabilityDeserialiserVideoTest, ParsesH265ProfileAndLevel)
{
    auto *cap = m_src.add_capabilities();
    auto *h265 = cap->mutable_codec_capabilities()->mutable_h265();
    auto *p = h265->add_profiles();
    p->set_type(VideoCap::H265_PROFILE_MAIN_10_HDR10);
    p->set_max_level(VideoCap::H265_LEVEL_6);

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &out = result.capabilities[0].codecCapabilities.h265->profiles;
    EXPECT_EQ(out[0].type, H265ProfileType::H265_MAIN_10_HDR10);
    EXPECT_EQ(out[0].maxLevel, H265Level::H265_LEVEL_6);
}

TEST_F(CapabilityDeserialiserVideoTest, H265ProfileAndLevelDefaults)
{
    auto *cap = m_src.add_capabilities();
    auto *h265 = cap->mutable_codec_capabilities()->mutable_h265();
    auto *p = h265->add_profiles();
    p->set_type(VideoCap::H265_PROFILE_MAIN); // inferred proto constant name
    p->set_max_level(VideoCap::H265_LEVEL_4); // inferred proto constant name

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &out = result.capabilities[0].codecCapabilities.h265->profiles;
    EXPECT_EQ(out[0].type, H265ProfileType::H265_MAIN);
    EXPECT_EQ(out[0].maxLevel, H265Level::H265_LEVEL_4);
}

// --- Vp9 ---------------------------------------------------------------
TEST_F(CapabilityDeserialiserVideoTest, ParsesVp9ProfileAndLevel)
{
    auto *cap = m_src.add_capabilities();
    auto *vp9 = cap->mutable_codec_capabilities()->mutable_vp9();
    auto *p = vp9->add_profiles();
    p->set_type(VideoCap::VP9_PROFILE_2);
    p->set_max_level(VideoCap::VP9_LEVEL_4);

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &out = result.capabilities[0].codecCapabilities.vp9->profiles;
    EXPECT_EQ(out[0].type, Vp9ProfileType::VP9_PROFILE_2);
    EXPECT_EQ(out[0].maxLevel, Vp9Level::VP9_LEVEL_4);
}

TEST_F(CapabilityDeserialiserVideoTest, Vp9ProfileAndLevelDefaults)
{
    auto *cap = m_src.add_capabilities();
    auto *vp9 = cap->mutable_codec_capabilities()->mutable_vp9();
    auto *p = vp9->add_profiles();
    p->set_type(VideoCap::VP9_PROFILE_0);    // inferred proto constant name
    p->set_max_level(VideoCap::VP9_LEVEL_1); // inferred proto constant name

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &out = result.capabilities[0].codecCapabilities.vp9->profiles;
    EXPECT_EQ(out[0].type, Vp9ProfileType::VP9_PROFILE_0);
    EXPECT_EQ(out[0].maxLevel, Vp9Level::VP9_LEVEL_1);
}

// --- Av1: profile ternary + level switch -----------------------------------
TEST_F(CapabilityDeserialiserVideoTest, ParsesAv1ProfileHighAndLevel)
{
    auto *cap = m_src.add_capabilities();
    auto *av1 = cap->mutable_codec_capabilities()->mutable_av1();
    auto *p = av1->add_profiles();
    p->set_type(VideoCap::AV1_PROFILE_HIGH);
    p->set_max_level(VideoCap::AV1_LEVEL_5_0);

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &out = result.capabilities[0].codecCapabilities.av1->profiles;
    EXPECT_EQ(out[0].type, Av1ProfileType::AV1_HIGH);
    EXPECT_EQ(out[0].maxLevel, Av1Level::AV1_LEVEL_5_0);
}

TEST_F(CapabilityDeserialiserVideoTest, Av1ProfileMainAndLevelDefault)
{
    auto *cap = m_src.add_capabilities();
    auto *av1 = cap->mutable_codec_capabilities()->mutable_av1();
    auto *p = av1->add_profiles();
    p->set_type(VideoCap::AV1_PROFILE_MAIN);   // inferred proto constant name
    p->set_max_level(VideoCap::AV1_LEVEL_4_0); // inferred proto constant name

    auto result = deserialiseVideoCapabilities(m_src);

    const auto &out = result.capabilities[0].codecCapabilities.av1->profiles;
    EXPECT_EQ(out[0].type, Av1ProfileType::AV1_MAIN);
    EXPECT_EQ(out[0].maxLevel, Av1Level::AV1_LEVEL_4_0);
}

// --- Multiple profile/dynamic-range entries + multiple capabilities entries
TEST_F(CapabilityDeserialiserVideoTest, ParsesMultipleProfilesAndCapabilitiesEntries)
{
    auto *cap1 = m_src.add_capabilities();
    auto *mpeg2 = cap1->mutable_codec_capabilities()->mutable_mpeg2();
    auto *p1 = mpeg2->add_profiles();
    p1->set_type(VideoCap::MPEG2_PROFILE_SIMPLE);
    p1->set_max_level(VideoCap::MPEG2_LEVEL_MAIN);
    auto *p2 = mpeg2->add_profiles();
    p2->set_type(VideoCap::MPEG2_PROFILE_SIMPLE);
    p2->set_max_level(VideoCap::MPEG2_LEVEL_HIGH);

    auto *cap2 = m_src.add_capabilities();
    cap2->mutable_codec_capabilities()->mutable_av1()->add_profiles()->set_type(VideoCap::AV1_PROFILE_HIGH);

    auto result = deserialiseVideoCapabilities(m_src);

    ASSERT_EQ(result.capabilities.size(), 2u);
    EXPECT_EQ(result.capabilities[0].codecCapabilities.mpeg2->profiles.size(), 2u);
    ASSERT_TRUE(result.capabilities[1].codecCapabilities.av1.has_value());
}
