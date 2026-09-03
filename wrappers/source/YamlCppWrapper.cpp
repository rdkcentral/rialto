/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace
{
using firebolt::rialto::common::AacCapability;
using firebolt::rialto::common::AacProfile;
using firebolt::rialto::common::AlacCapability;
using firebolt::rialto::common::AudioDecoderCapabilities;
using firebolt::rialto::common::AudioDecoderCapability;
using firebolt::rialto::common::AudioProfileCapability;
using firebolt::rialto::common::Av1CodecCapability;
using firebolt::rialto::common::Av1Level;
using firebolt::rialto::common::Av1Profile;
using firebolt::rialto::common::Av1ProfileType;
using firebolt::rialto::common::AvsCapability;
using firebolt::rialto::common::AvsProfile;
using firebolt::rialto::common::DolbyAc3Capability;
using firebolt::rialto::common::DolbyAc3Profile;
using firebolt::rialto::common::DolbyAc4Capability;
using firebolt::rialto::common::DolbyEac3Capability;
using firebolt::rialto::common::DolbyEac3Profile;
using firebolt::rialto::common::DolbyTruehdCapability;
using firebolt::rialto::common::DtsCapability;
using firebolt::rialto::common::DtsProfile;
using firebolt::rialto::common::DynamicRange;
using firebolt::rialto::common::FlacCapability;
using firebolt::rialto::common::H264CodecCapability;
using firebolt::rialto::common::H264Level;
using firebolt::rialto::common::H264Profile;
using firebolt::rialto::common::H264ProfileType;
using firebolt::rialto::common::H265CodecCapability;
using firebolt::rialto::common::H265Level;
using firebolt::rialto::common::H265Profile;
using firebolt::rialto::common::H265ProfileType;
using firebolt::rialto::common::Mp3Capability;
using firebolt::rialto::common::Mpeg2CodecCapability;
using firebolt::rialto::common::Mpeg2Level;
using firebolt::rialto::common::Mpeg2Profile;
using firebolt::rialto::common::Mpeg2ProfileType;
using firebolt::rialto::common::MpegAudioCapability;
using firebolt::rialto::common::MpegAudioProfile;
using firebolt::rialto::common::OpusCapability;
using firebolt::rialto::common::PcmCapability;
using firebolt::rialto::common::RealAudioCapability;
using firebolt::rialto::common::RealAudioProfile;
using firebolt::rialto::common::SbcCapability;
using firebolt::rialto::common::UsacCapability;
using firebolt::rialto::common::UsacProfile;
using firebolt::rialto::common::VideoDecoderCapabilities;
using firebolt::rialto::common::VideoDecoderCapability;
using firebolt::rialto::common::VorbisCapability;
using firebolt::rialto::common::Vp9CodecCapability;
using firebolt::rialto::common::Vp9Level;
using firebolt::rialto::common::Vp9Profile;
using firebolt::rialto::common::Vp9ProfileType;

const std::string kAudioCapabilitiesFilePath{"/product/hfp/config/hfp-audiodecoder.yaml"};
const std::string kVideoCapabilitiesFilePath{"/product/hfp/config/hfp-videodecoder.yaml"};

AudioProfileCapability parseAudioProfileCapability(const YAML::Node &node)
{
    if (!node["maxBitrateInBps"] || !node["maxChannels"] || !node["maxSampleRateInHz"] || !node["maxBitDepth"])
    {
        throw std::runtime_error("AudioProfileCapability: missing required field(s) "
                                 "(maxBitrateInBps, maxChannels, maxSampleRateInHz, maxBitDepth)");
    }
    AudioProfileCapability cap{};
    cap.maxBitrateInBps = node["maxBitrateInBps"].as<uint64_t>();
    cap.maxChannels = node["maxChannels"].as<uint32_t>();
    cap.maxSampleRateInHz = node["maxSampleRateInHz"].as<uint32_t>();
    cap.maxBitDepth = node["maxBitDepth"].as<uint32_t>();
    return cap;
}

/**
 * @brief Parse the single BASE profile capability from a codec node.
 *
 * YAML structure: { profiles: [{BASE: {maxBitrateInBps:..., ...}}] }
 */
AudioProfileCapability parseBaseProfileCapability(const YAML::Node &codecData)
{
    if (codecData["profiles"] && codecData["profiles"].IsSequence())
    {
        for (const auto &entry : codecData["profiles"])
        {
            YAML::const_iterator it = entry.begin();
            if (it != entry.end())
            {
                if (it->first.as<std::string>() != "BASE")
                    throw std::runtime_error("parseBaseProfileCapability: expected key 'BASE', got '" +
                                             it->first.as<std::string>() + "'");
                return parseAudioProfileCapability(it->second);
            }
        }
    }
    return {};
}

/**
 * @brief Parse a named-profile map from a codec node.
 *
 * YAML structure: { profiles: [{LC: {caps}}, {HE_V1: {caps}}, ...] }
 */
template <typename MapType, typename ProfileConverter>
MapType parseNamedProfileMap(const YAML::Node &codecData, ProfileConverter convertProfile)
{
    MapType result;
    if (!codecData["profiles"] || !codecData["profiles"].IsSequence())
        return result;
    for (const auto &profileEntry : codecData["profiles"])
    {
        for (YAML::const_iterator it = profileEntry.begin(); it != profileEntry.end(); ++it)
        {
            const std::string kProfileName = it->first.as<std::string>();
            auto profile = convertProfile(kProfileName);
            if (profile)
                result.emplace(*profile, parseAudioProfileCapability(it->second));
        }
    }
    return result;
}

std::optional<AacProfile> convertAacProfileName(const std::string &name)
{
    if ("LC" == name)
        return AacProfile::LC;
    if ("HE_V1" == name)
        return AacProfile::HE_V1;
    if ("HE_V2" == name)
        return AacProfile::HE_V2;
    if ("ELD" == name)
        return AacProfile::ELD;
    if ("X_HE" == name)
        return AacProfile::X_HE;
    return std::nullopt;
}

std::optional<DolbyAc3Profile> convertDolbyAc3ProfileName(const std::string &name)
{
    if ("STANDARD" == name)
        return DolbyAc3Profile::STANDARD;
    return std::nullopt;
}

std::optional<DolbyEac3Profile> convertDolbyEac3ProfileName(const std::string &name)
{
    if ("PLUS" == name)
        return DolbyEac3Profile::PLUS;
    if ("PLUS_JOC" == name)
        return DolbyEac3Profile::PLUS_JOC;
    return std::nullopt;
}

std::optional<MpegAudioProfile> convertMpegAudioProfileName(const std::string &name)
{
    if ("LAYER_1" == name)
        return MpegAudioProfile::LAYER_1;
    if ("LAYER_2" == name)
        return MpegAudioProfile::LAYER_2;
    return std::nullopt;
}

std::optional<RealAudioProfile> convertRealAudioProfileName(const std::string &name)
{
    if ("RA8" == name)
        return RealAudioProfile::RA8;
    if ("RA10" == name)
        return RealAudioProfile::RA10;
    return std::nullopt;
}

std::optional<UsacProfile> convertUsacProfileName(const std::string &name)
{
    if ("BASELINE" == name)
        return UsacProfile::BASELINE;
    if ("EXTENDED_HE_AAC" == name)
        return UsacProfile::EXTENDED_HE_AAC;
    return std::nullopt;
}

std::optional<DtsProfile> convertDtsProfileName(const std::string &name)
{
    if ("CORE" == name)
        return DtsProfile::CORE;
    if ("HD_HRA" == name)
        return DtsProfile::HD_HRA;
    if ("HD_MA" == name)
        return DtsProfile::HD_MA;
    return std::nullopt;
}

std::optional<AvsProfile> convertAvsProfileName(const std::string &name)
{
    if ("AVS1_PART2" == name)
        return AvsProfile::AVS1_PART2;
    if ("AVS2" == name)
        return AvsProfile::AVS2;
    if ("AVS3" == name)
        return AvsProfile::AVS3;
    return std::nullopt;
}

firebolt::rialto::common::AudioDecoderCapability buildAudioDecoderCapability(const YAML::Node &capability)
{
    firebolt::rialto::common::AudioDecoderCapability result;
    for (YAML::const_iterator capabilitiesIt = capability.begin(); capabilitiesIt != capability.end(); ++capabilitiesIt)
    {
        if (capabilitiesIt->second["codecCapabilities"])
        {
            for (const auto &codecCapability : capabilitiesIt->second["codecCapabilities"])
            {
                for (YAML::const_iterator codecIt = codecCapability.begin(); codecIt != codecCapability.end(); ++codecIt)
                {
                    const std::string kCodecName{codecIt->first.as<std::string>()};
                    const auto &kCodecData{codecIt->second};
                    if ("PCM" == kCodecName)
                    {
                        result.pcm = PcmCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("AAC" == kCodecName)
                    {
                        result.aac = AacCapability{parseNamedProfileMap<
                            std::map<AacProfile, firebolt::rialto::common::AudioProfileCapability>>(kCodecData,
                                                                                                    convertAacProfileName)};
                    }
                    else if ("MPEG_AUDIO" == kCodecName)
                    {
                        result.mpegAudio = MpegAudioCapability{parseNamedProfileMap<std::map<
                            MpegAudioProfile, firebolt::rialto::common::AudioProfileCapability>>(kCodecData,
                                                                                                 convertMpegAudioProfileName)};
                    }
                    else if ("MP3" == kCodecName)
                    {
                        result.mp3 = Mp3Capability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("ALAC" == kCodecName)
                    {
                        result.alac = AlacCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("SBC" == kCodecName)
                    {
                        result.sbc = SbcCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("DOLBY_AC3" == kCodecName)
                    {
                        result.dolbyAc3 = DolbyAc3Capability{parseNamedProfileMap<std::map<
                            DolbyAc3Profile, firebolt::rialto::common::AudioProfileCapability>>(kCodecData,
                                                                                                convertDolbyAc3ProfileName)};
                    }
                    else if ("DOLBY_AC4" == kCodecName)
                    {
                        result.dolbyAc4 = DolbyAc4Capability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("DOLBY_EAC3" == kCodecName)
                    {
                        result.dolbyEac3 = DolbyEac3Capability{parseNamedProfileMap<std::map<
                            DolbyEac3Profile, firebolt::rialto::common::AudioProfileCapability>>(kCodecData,
                                                                                                 convertDolbyEac3ProfileName)};
                    }
                    else if ("DOLBY_TRUEHD" == kCodecName)
                    {
                        result.dolbyTruehd = DolbyTruehdCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("FLAC" == kCodecName)
                    {
                        result.flac = FlacCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("VORBIS" == kCodecName)
                    {
                        result.vorbis = VorbisCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("OPUS" == kCodecName)
                    {
                        result.opus = OpusCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("REALAUDIO" == kCodecName)
                    {
                        result.realAudio = RealAudioCapability{parseNamedProfileMap<std::map<
                            RealAudioProfile, firebolt::rialto::common::AudioProfileCapability>>(kCodecData,
                                                                                                 convertRealAudioProfileName)};
                    }
                    else if ("USAC" == kCodecName)
                    {
                        result.usac = UsacCapability{parseNamedProfileMap<
                            std::map<UsacProfile, firebolt::rialto::common::AudioProfileCapability>>(kCodecData,
                                                                                                     convertUsacProfileName)};
                    }
                    else if ("DTS" == kCodecName)
                    {
                        result.dts = DtsCapability{parseNamedProfileMap<
                            std::map<DtsProfile, firebolt::rialto::common::AudioProfileCapability>>(kCodecData,
                                                                                                    convertDtsProfileName)};
                    }
                    else if ("AVS" == kCodecName)
                    {
                        result.avs = AvsCapability{parseNamedProfileMap<
                            std::map<AvsProfile, firebolt::rialto::common::AudioProfileCapability>>(kCodecData,
                                                                                                    convertAvsProfileName)};
                    }
                }
            }
        }
    }
    return result;
}

std::vector<DynamicRange> getDynamicRanges(const YAML::Node &ranges)
{
    std::vector<DynamicRange> result;
    if (!ranges || !ranges.IsSequence())
        return result;
    for (const auto &r : ranges)
    {
        const std::string kName = r.as<std::string>();
        if ("SDR" == kName)
            result.push_back(DynamicRange::SDR);
        else if ("HLG" == kName)
            result.push_back(DynamicRange::HLG);
        else if ("HDR10" == kName)
            result.push_back(DynamicRange::HDR10);
        else if ("HDR10PLUS" == kName)
            result.push_back(DynamicRange::HDR10PLUS);
        else if ("DOLBY_VISION" == kName)
            result.push_back(DynamicRange::DOLBY_VISION);
    }
    return result;
}

// ---------- Video profile builders ----------
// Each profiles node is a YAML sequence of single-key maps:
//   [{PROFILE_NAME: {maxLevel: ..., maxBitrateInBps: ...}}, ...]

std::vector<Mpeg2Profile> buildMpeg2Profiles(const YAML::Node &profilesNode)
{
    std::vector<Mpeg2Profile> result;
    if (!profilesNode.IsSequence())
        return result;
    for (const auto &entry : profilesNode)
    {
        for (YAML::const_iterator it = entry.begin(); it != entry.end(); ++it)
        {
            const std::string kName = it->first.as<std::string>();
            Mpeg2Profile p{};
            if ("MPEG2_SIMPLE" == kName)
                p.type = Mpeg2ProfileType::MPEG2_SIMPLE;
            else if ("MPEG2_MAIN" == kName)
                p.type = Mpeg2ProfileType::MPEG2_MAIN;
            else
                continue;
            const auto &v = it->second;
            if (v["maxBitrateInBps"])
                p.maxBitrateInBps = v["maxBitrateInBps"].as<uint64_t>();
            if (v["maxLevel"])
            {
                const std::string kLevel = v["maxLevel"].as<std::string>();
                if ("MPEG2_LEVEL_LOW" == kLevel)
                    p.maxLevel = Mpeg2Level::MPEG2_LEVEL_LOW;
                else if ("MPEG2_LEVEL_MAIN" == kLevel)
                    p.maxLevel = Mpeg2Level::MPEG2_LEVEL_MAIN;
                else if ("MPEG2_LEVEL_HIGH" == kLevel)
                    p.maxLevel = Mpeg2Level::MPEG2_LEVEL_HIGH;
            }
            result.push_back(p);
        }
    }
    return result;
}

std::vector<H264Profile> buildH264Profiles(const YAML::Node &profilesNode)
{
    std::vector<H264Profile> result;
    if (!profilesNode.IsSequence())
        return result;
    for (const auto &entry : profilesNode)
    {
        for (YAML::const_iterator it = entry.begin(); it != entry.end(); ++it)
        {
            const std::string kName = it->first.as<std::string>();
            H264Profile p{};
            if ("H264_BASELINE" == kName)
                p.type = H264ProfileType::H264_BASELINE;
            else if ("H264_MAIN" == kName)
                p.type = H264ProfileType::H264_MAIN;
            else if ("H264_HIGH" == kName)
                p.type = H264ProfileType::H264_HIGH;
            else
                continue;
            const auto &v = it->second;
            if (v["maxBitrateInBps"])
                p.maxBitrateInBps = v["maxBitrateInBps"].as<uint64_t>();
            if (v["maxLevel"])
            {
                const std::string kLevel = v["maxLevel"].as<std::string>();
                if ("H264_LEVEL_3" == kLevel)
                    p.maxLevel = H264Level::H264_LEVEL_3;
                else if ("H264_LEVEL_3_1" == kLevel)
                    p.maxLevel = H264Level::H264_LEVEL_3_1;
                else if ("H264_LEVEL_4" == kLevel)
                    p.maxLevel = H264Level::H264_LEVEL_4;
                else if ("H264_LEVEL_4_1" == kLevel)
                    p.maxLevel = H264Level::H264_LEVEL_4_1;
                else if ("H264_LEVEL_5" == kLevel)
                    p.maxLevel = H264Level::H264_LEVEL_5;
                else if ("H264_LEVEL_5_1" == kLevel)
                    p.maxLevel = H264Level::H264_LEVEL_5_1;
                else if ("H264_LEVEL_5_2" == kLevel)
                    p.maxLevel = H264Level::H264_LEVEL_5_2;
            }
            result.push_back(p);
        }
    }
    return result;
}

std::vector<H265Profile> buildH265Profiles(const YAML::Node &profilesNode)
{
    std::vector<H265Profile> result;
    if (!profilesNode.IsSequence())
        return result;
    for (const auto &entry : profilesNode)
    {
        for (YAML::const_iterator it = entry.begin(); it != entry.end(); ++it)
        {
            const std::string kName = it->first.as<std::string>();
            H265Profile p{};
            if ("H265_MAIN" == kName)
                p.type = H265ProfileType::H265_MAIN;
            else if ("H265_MAIN_10" == kName)
                p.type = H265ProfileType::H265_MAIN_10;
            else if ("H265_MAIN_10_HDR10" == kName)
                p.type = H265ProfileType::H265_MAIN_10_HDR10;
            else
                continue;
            const auto &v = it->second;
            if (v["maxBitrateInBps"])
                p.maxBitrateInBps = v["maxBitrateInBps"].as<uint64_t>();
            if (v["maxLevel"])
            {
                const std::string kLevel = v["maxLevel"].as<std::string>();
                if ("H265_LEVEL_4" == kLevel)
                    p.maxLevel = H265Level::H265_LEVEL_4;
                else if ("H265_LEVEL_4_1" == kLevel)
                    p.maxLevel = H265Level::H265_LEVEL_4_1;
                else if ("H265_LEVEL_5" == kLevel)
                    p.maxLevel = H265Level::H265_LEVEL_5;
                else if ("H265_LEVEL_5_1" == kLevel)
                    p.maxLevel = H265Level::H265_LEVEL_5_1;
                else if ("H265_LEVEL_5_2" == kLevel)
                    p.maxLevel = H265Level::H265_LEVEL_5_2;
                else if ("H265_LEVEL_6" == kLevel)
                    p.maxLevel = H265Level::H265_LEVEL_6;
                else if ("H265_LEVEL_6_1" == kLevel)
                    p.maxLevel = H265Level::H265_LEVEL_6_1;
                else if ("H265_LEVEL_6_2" == kLevel)
                    p.maxLevel = H265Level::H265_LEVEL_6_2;
            }
            result.push_back(p);
        }
    }
    return result;
}

std::vector<Vp9Profile> buildVp9Profiles(const YAML::Node &profilesNode)
{
    std::vector<Vp9Profile> result;
    if (!profilesNode.IsSequence())
        return result;
    for (const auto &entry : profilesNode)
    {
        for (YAML::const_iterator it = entry.begin(); it != entry.end(); ++it)
        {
            const std::string kName = it->first.as<std::string>();
            Vp9Profile p{};
            if ("VP9_PROFILE_0" == kName)
                p.type = Vp9ProfileType::VP9_PROFILE_0;
            else if ("VP9_PROFILE_1" == kName)
                p.type = Vp9ProfileType::VP9_PROFILE_1;
            else if ("VP9_PROFILE_2" == kName)
                p.type = Vp9ProfileType::VP9_PROFILE_2;
            else if ("VP9_PROFILE_3" == kName)
                p.type = Vp9ProfileType::VP9_PROFILE_3;
            else
                continue;
            const auto &v = it->second;
            if (v["maxBitrateInBps"])
                p.maxBitrateInBps = v["maxBitrateInBps"].as<uint64_t>();
            if (v["maxLevel"])
            {
                const std::string kLevel = v["maxLevel"].as<std::string>();
                if ("VP9_LEVEL_1" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_1;
                else if ("VP9_LEVEL_1_1" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_1_1;
                else if ("VP9_LEVEL_2" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_2;
                else if ("VP9_LEVEL_2_1" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_2_1;
                else if ("VP9_LEVEL_3" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_3;
                else if ("VP9_LEVEL_3_1" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_3_1;
                else if ("VP9_LEVEL_4" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_4;
                else if ("VP9_LEVEL_4_1" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_4_1;
                else if ("VP9_LEVEL_5" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_5;
                else if ("VP9_LEVEL_5_1" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_5_1;
                else if ("VP9_LEVEL_5_2" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_5_2;
                else if ("VP9_LEVEL_6" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_6;
                else if ("VP9_LEVEL_6_1" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_6_1;
                else if ("VP9_LEVEL_6_2" == kLevel)
                    p.maxLevel = Vp9Level::VP9_LEVEL_6_2;
            }
            result.push_back(p);
        }
    }
    return result;
}

std::vector<Av1Profile> buildAv1Profiles(const YAML::Node &profilesNode)
{
    std::vector<Av1Profile> result;
    if (!profilesNode.IsSequence())
        return result;
    for (const auto &entry : profilesNode)
    {
        for (YAML::const_iterator it = entry.begin(); it != entry.end(); ++it)
        {
            const std::string kName = it->first.as<std::string>();
            Av1Profile p{};
            if ("AV1_MAIN" == kName)
                p.type = Av1ProfileType::AV1_MAIN;
            else if ("AV1_HIGH" == kName)
                p.type = Av1ProfileType::AV1_HIGH;
            else
                continue;
            const auto &v = it->second;
            if (v["maxBitrateInBps"])
                p.maxBitrateInBps = v["maxBitrateInBps"].as<uint64_t>();
            if (v["maxLevel"])
            {
                const std::string kLevel = v["maxLevel"].as<std::string>();
                if ("AV1_LEVEL_4_0" == kLevel)
                    p.maxLevel = Av1Level::AV1_LEVEL_4_0;
                else if ("AV1_LEVEL_4_1" == kLevel)
                    p.maxLevel = Av1Level::AV1_LEVEL_4_1;
                else if ("AV1_LEVEL_5_0" == kLevel)
                    p.maxLevel = Av1Level::AV1_LEVEL_5_0;
                else if ("AV1_LEVEL_5_1" == kLevel)
                    p.maxLevel = Av1Level::AV1_LEVEL_5_1;
                else if ("AV1_LEVEL_5_2" == kLevel)
                    p.maxLevel = Av1Level::AV1_LEVEL_5_2;
                else if ("AV1_LEVEL_6_0" == kLevel)
                    p.maxLevel = Av1Level::AV1_LEVEL_6_0;
                else if ("AV1_LEVEL_6_1" == kLevel)
                    p.maxLevel = Av1Level::AV1_LEVEL_6_1;
                else if ("AV1_LEVEL_6_2" == kLevel)
                    p.maxLevel = Av1Level::AV1_LEVEL_6_2;
            }
            result.push_back(p);
        }
    }
    return result;
}

firebolt::rialto::common::VideoDecoderCapability buildVideoDecoderCapability(const YAML::Node &capability)
{
    firebolt::rialto::common::VideoDecoderCapability result;
    for (YAML::const_iterator capabilitiesIt = capability.begin(); capabilitiesIt != capability.end(); ++capabilitiesIt)
    {
        if (capabilitiesIt->second["codecCapabilities"])
        {
            for (const auto &codecCapability : capabilitiesIt->second["codecCapabilities"])
            {
                for (YAML::const_iterator codecCapabilitiesIt = codecCapability.begin();
                     codecCapabilitiesIt != codecCapability.end(); ++codecCapabilitiesIt)
                {
                    const std::string kCodecName = codecCapabilitiesIt->first.as<std::string>();
                    const YAML::Node &codecNode = codecCapabilitiesIt->second;
                    if ("MPEG2_VIDEO" == kCodecName)
                    {
                        Mpeg2CodecCapability c;
                        c.profiles = buildMpeg2Profiles(codecNode["profiles"]);
                        c.dynamicRanges = getDynamicRanges(codecNode["dynamicRange"]);
                        result.codecCapabilities.mpeg2 = std::move(c);
                    }
                    else if ("H264_AVC" == kCodecName)
                    {
                        H264CodecCapability c;
                        c.profiles = buildH264Profiles(codecNode["profiles"]);
                        c.dynamicRanges = getDynamicRanges(codecNode["dynamicRange"]);
                        result.codecCapabilities.h264 = std::move(c);
                    }
                    else if ("H265_HEVC" == kCodecName)
                    {
                        H265CodecCapability c;
                        c.profiles = buildH265Profiles(codecNode["profiles"]);
                        c.dynamicRanges = getDynamicRanges(codecNode["dynamicRange"]);
                        result.codecCapabilities.h265 = std::move(c);
                    }
                    else if ("VP9" == kCodecName)
                    {
                        Vp9CodecCapability c;
                        c.profiles = buildVp9Profiles(codecNode["profiles"]);
                        c.dynamicRanges = getDynamicRanges(codecNode["dynamicRange"]);
                        result.codecCapabilities.vp9 = std::move(c);
                    }
                    else if ("AV1" == kCodecName)
                    {
                        Av1CodecCapability c;
                        c.profiles = buildAv1Profiles(codecNode["profiles"]);
                        c.dynamicRanges = getDynamicRanges(codecNode["dynamicRange"]);
                        result.codecCapabilities.av1 = std::move(c);
                    }
                }
            }
        }
    }
    return result;
}

} // namespace

namespace firebolt::rialto::wrappers
{
std::shared_ptr<IYamlCppWrapper> YamlCppWrapperFactory::createYamlCppWrapper()
{
    return std::make_shared<YamlCppWrapper>();
}

::firebolt::rialto::common::DecoderCapabilitiesStatus
YamlCppWrapper::getAudioDecoderCapabilities(::firebolt::rialto::common::AudioDecoderCapabilities &capabilities) const
try
{
    YAML::Node audioCapsFile = YAML::LoadFile(kAudioCapabilitiesFilePath);
    if (audioCapsFile.IsNull())
    {
        return ::firebolt::rialto::common::DecoderCapabilitiesStatus::CONFIG_NOT_FOUND;
    }
    if (audioCapsFile["audiodecoder"])
    {
        if (audioCapsFile["audiodecoder"]["interfaceVersion"])
        {
            capabilities.interfaceVersion = audioCapsFile["audiodecoder"]["interfaceVersion"].as<std::string>();
        }
        if (audioCapsFile["audiodecoder"]["schemaVersion"])
        {
            capabilities.schemaVersion = audioCapsFile["audiodecoder"]["schemaVersion"].as<std::string>();
        }
        if (audioCapsFile["audiodecoder"]["Capabilities"])
        {
            for (const auto &capability : audioCapsFile["audiodecoder"]["Capabilities"])
            {
                capabilities.capabilities.push_back(buildAudioDecoderCapability(capability));
            }
        }
    }
    return ::firebolt::rialto::common::DecoderCapabilitiesStatus::OK;
}
catch (const YAML::BadFile &)
{
    return ::firebolt::rialto::common::DecoderCapabilitiesStatus::CONFIG_NOT_FOUND;
}
catch (const std::exception &)
{
    return ::firebolt::rialto::common::DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED;
}

::firebolt::rialto::common::DecoderCapabilitiesStatus
YamlCppWrapper::getVideoDecoderCapabilities(::firebolt::rialto::common::VideoDecoderCapabilities &capabilities) const
try
{
    YAML::Node videoCapsFile = YAML::LoadFile(kVideoCapabilitiesFilePath);
    if (videoCapsFile.IsNull())
    {
        return ::firebolt::rialto::common::DecoderCapabilitiesStatus::CONFIG_NOT_FOUND;
    }
    if (videoCapsFile["videodecoder"])
    {
        if (videoCapsFile["videodecoder"]["interfaceVersion"])
        {
            capabilities.interfaceVersion = videoCapsFile["videodecoder"]["interfaceVersion"].as<std::string>();
        }
        if (videoCapsFile["videodecoder"]["schemaVersion"])
        {
            capabilities.schemaVersion = videoCapsFile["videodecoder"]["schemaVersion"].as<std::string>();
        }
        if (videoCapsFile["videodecoder"]["Capabilities"])
        {
            for (const auto &capability : videoCapsFile["videodecoder"]["Capabilities"])
            {
                capabilities.capabilities.push_back(buildVideoDecoderCapability(capability));
            }
        }
    }
    return ::firebolt::rialto::common::DecoderCapabilitiesStatus::OK;
}
catch (const std::exception &e)
{
    return ::firebolt::rialto::common::DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED;
}

} // namespace firebolt::rialto::wrappers
