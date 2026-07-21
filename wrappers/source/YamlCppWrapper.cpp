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
#include <yaml-cpp/yaml.h>

namespace
{
const std::string kAudioCapabilitiesFilePath{"/product/hfp/config/hfp-audiodecoder.yaml"};
const std::string kVideoCapabilitiesFilePath{"/product/hfp/config/hfp-videodecoder.yaml"};

std::vector<firebolt::rialto::AacProfile> getAacProfiles(const YAML::Node &codecData)
{
    std::vector<firebolt::rialto::AacProfile> result;
    if (codecData["profiles"] && codecData["profiles"].IsSequence())
    {
        for (std::size_t i = 0; i < codecData["profiles"].size(); ++i)
        {
            const std::string kProfileName = codecData["profiles"][i].as<std::string>();
            if ("LC" == kProfileName)
            {
                result.push_back(firebolt::rialto::AacProfile::LC);
            }
            else if ("HE_V1" == kProfileName)
            {
                result.push_back(firebolt::rialto::AacProfile::HE_V1);
            }
            else if ("HE_V2" == kProfileName)
            {
                result.push_back(firebolt::rialto::AacProfile::HE_V2);
            }
            else if ("ELD" == kProfileName)
            {
                result.push_back(firebolt::rialto::AacProfile::ELD);
            }
            else if ("X_HE" == kProfileName)
            {
                result.push_back(firebolt::rialto::AacProfile::X_HE);
            }
        }
    }
    return result;
}

firebolt::rialto::AudioProfileCapability parseAudioProfileCapability(const YAML::Node &node)
{
    firebolt::rialto::AudioProfileCapability cap{};
    if (node["maxBitrateInBps"])
        cap.maxBitrateInBps = node["maxBitrateInBps"].as<uint64_t>();
    if (node["maxChannels"])
        cap.maxChannels = node["maxChannels"].as<uint32_t>();
    if (node["maxSampleRateInHz"])
        cap.maxSampleRateInHz = node["maxSampleRateInHz"].as<uint32_t>();
    if (node["maxBitDepth"])
        cap.maxBitDepth = node["maxBitDepth"].as<uint32_t>();
    return cap;
}

/**
 * @brief Parse the single BASE profile capability from a codec node.
 *
 * YAML structure: { profiles: [{BASE: {maxBitrateInBps:..., ...}}] }
 */
firebolt::rialto::AudioProfileCapability parseBaseProfileCapability(const YAML::Node &codecData)
{
    if (codecData["profiles"] && codecData["profiles"].IsSequence())
    {
        for (const auto &entry : codecData["profiles"])
        {
            for (YAML::const_iterator it = entry.begin(); it != entry.end(); ++it)
            {
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

std::optional<firebolt::rialto::AacProfile> convertAacProfileName(const std::string &name)
{
    if ("LC" == name)       return firebolt::rialto::AacProfile::LC;
    if ("HE_V1" == name)    return firebolt::rialto::AacProfile::HE_V1;
    if ("HE_V2" == name)    return firebolt::rialto::AacProfile::HE_V2;
    if ("ELD" == name)      return firebolt::rialto::AacProfile::ELD;
    if ("X_HE" == name)     return firebolt::rialto::AacProfile::X_HE;
    return std::nullopt;
}

std::optional<firebolt::rialto::DolbyAc3Profile> convertDolbyAc3ProfileName(const std::string &name)
{
    if ("STANDARD" == name) return firebolt::rialto::DolbyAc3Profile::STANDARD;
    return std::nullopt;
}

std::optional<firebolt::rialto::DolbyEac3Profile> convertDolbyEac3ProfileName(const std::string &name)
{
    if ("PLUS" == name)     return firebolt::rialto::DolbyEac3Profile::PLUS;
    if ("PLUS_JOC" == name) return firebolt::rialto::DolbyEac3Profile::PLUS_JOC;
    return std::nullopt;
}

std::optional<firebolt::rialto::MpegAudioProfile> convertMpegAudioProfileName(const std::string &name)
{
    if ("LAYER_1" == name) return firebolt::rialto::MpegAudioProfile::LAYER_1;
    if ("LAYER_2" == name) return firebolt::rialto::MpegAudioProfile::LAYER_2;
    return std::nullopt;
}

std::optional<firebolt::rialto::RealAudioProfile> convertRealAudioProfileName(const std::string &name)
{
    if ("RA8" == name)  return firebolt::rialto::RealAudioProfile::RA8;
    if ("RA10" == name) return firebolt::rialto::RealAudioProfile::RA10;
    return std::nullopt;
}

std::optional<firebolt::rialto::UsacProfile> convertUsacProfileName(const std::string &name)
{
    if ("BASELINE" == name)          return firebolt::rialto::UsacProfile::BASELINE;
    if ("EXTENDED_HE_AAC" == name)   return firebolt::rialto::UsacProfile::EXTENDED_HE_AAC;
    return std::nullopt;
}

std::optional<firebolt::rialto::DtsProfile> convertDtsProfileName(const std::string &name)
{
    if ("CORE" == name)   return firebolt::rialto::DtsProfile::CORE;
    if ("HD_HRA" == name) return firebolt::rialto::DtsProfile::HD_HRA;
    if ("HD_MA" == name)  return firebolt::rialto::DtsProfile::HD_MA;
    return std::nullopt;
}

std::optional<firebolt::rialto::AvsProfile> convertAvsProfileName(const std::string &name)
{
    if ("AVS1_PART2" == name) return firebolt::rialto::AvsProfile::AVS1_PART2;
    if ("AVS2" == name)       return firebolt::rialto::AvsProfile::AVS2;
    if ("AVS3" == name)       return firebolt::rialto::AvsProfile::AVS3;
    return std::nullopt;
}

firebolt::rialto::AudioDecoderCapability buildAudioDecoderCapability(const YAML::Node &capability)
{
    firebolt::rialto::AudioDecoderCapability result;
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
                        result.pcm = firebolt::rialto::PcmCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("AAC" == kCodecName)
                    {
                        result.aac = firebolt::rialto::AacCapability{
                            parseNamedProfileMap<std::map<firebolt::rialto::AacProfile,
                                                          firebolt::rialto::AudioProfileCapability>>(
                                kCodecData, convertAacProfileName)};
                    }
                    else if ("MPEG_AUDIO" == kCodecName)
                    {
                        result.mpegAudio = firebolt::rialto::MpegAudioCapability{
                            parseNamedProfileMap<std::map<firebolt::rialto::MpegAudioProfile,
                                                          firebolt::rialto::AudioProfileCapability>>(
                                kCodecData, convertMpegAudioProfileName)};
                    }
                    else if ("MP3" == kCodecName)
                    {
                        result.mp3 = firebolt::rialto::Mp3Capability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("ALAC" == kCodecName)
                    {
                        result.alac = firebolt::rialto::AlacCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("SBC" == kCodecName)
                    {
                        result.sbc = firebolt::rialto::SbcCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("DOLBY_AC3" == kCodecName)
                    {
                        result.dolbyAc3 = firebolt::rialto::DolbyAc3Capability{
                            parseNamedProfileMap<std::map<firebolt::rialto::DolbyAc3Profile,
                                                          firebolt::rialto::AudioProfileCapability>>(
                                kCodecData, convertDolbyAc3ProfileName)};
                    }
                    else if ("DOLBY_AC4" == kCodecName)
                    {
                        result.dolbyAc4 = firebolt::rialto::DolbyAc4Capability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("DOLBY_EAC3" == kCodecName)
                    {
                        result.dolbyEac3 = firebolt::rialto::DolbyEac3Capability{
                            parseNamedProfileMap<std::map<firebolt::rialto::DolbyEac3Profile,
                                                          firebolt::rialto::AudioProfileCapability>>(
                                kCodecData, convertDolbyEac3ProfileName)};
                    }
                    else if ("DOLBY_TRUEHD" == kCodecName)
                    {
                        result.dolbyTruehd = firebolt::rialto::DolbyTruehdCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("FLAC" == kCodecName)
                    {
                        result.flac = firebolt::rialto::FlacCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("VORBIS" == kCodecName)
                    {
                        result.vorbis = firebolt::rialto::VorbisCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("OPUS" == kCodecName)
                    {
                        result.opus = firebolt::rialto::OpusCapability{parseBaseProfileCapability(kCodecData)};
                    }
                    else if ("REALAUDIO" == kCodecName)
                    {
                        result.realAudio = firebolt::rialto::RealAudioCapability{
                            parseNamedProfileMap<std::map<firebolt::rialto::RealAudioProfile,
                                                          firebolt::rialto::AudioProfileCapability>>(
                                kCodecData, convertRealAudioProfileName)};
                    }
                    else if ("USAC" == kCodecName)
                    {
                        result.usac = firebolt::rialto::UsacCapability{
                            parseNamedProfileMap<std::map<firebolt::rialto::UsacProfile,
                                                          firebolt::rialto::AudioProfileCapability>>(
                                kCodecData, convertUsacProfileName)};
                    }
                    else if ("DTS" == kCodecName)
                    {
                        result.dts = firebolt::rialto::DtsCapability{
                            parseNamedProfileMap<std::map<firebolt::rialto::DtsProfile,
                                                          firebolt::rialto::AudioProfileCapability>>(
                                kCodecData, convertDtsProfileName)};
                    }
                    else if ("AVS" == kCodecName)
                    {
                        result.avs = firebolt::rialto::AvsCapability{
                            parseNamedProfileMap<std::map<firebolt::rialto::AvsProfile,
                                                          firebolt::rialto::AudioProfileCapability>>(
                                kCodecData, convertAvsProfileName)};
                    }
                }
            }
        }
    }
    return result;
}

std::vector<firebolt::rialto::DynamicRange> getDynamicRanges(const YAML::Node &ranges)
{
    std::vector<firebolt::rialto::DynamicRange> result;
    if (!ranges || !ranges.IsSequence()) return result;
    for (const auto &r : ranges)
    {
        const std::string kName = r.as<std::string>();
        if ("SDR" == kName)           result.push_back(firebolt::rialto::DynamicRange::SDR);
        else if ("HLG" == kName)      result.push_back(firebolt::rialto::DynamicRange::HLG);
        else if ("HDR10" == kName)    result.push_back(firebolt::rialto::DynamicRange::HDR10);
        else if ("HDR10PLUS" == kName)result.push_back(firebolt::rialto::DynamicRange::HDR10PLUS);
        else if ("DOLBY_VISION" == kName) result.push_back(firebolt::rialto::DynamicRange::DOLBY_VISION);
    }
    return result;
}

// ---------- Video profile builders ----------
// Each profiles node is a YAML sequence of single-key maps:
//   [{PROFILE_NAME: {maxLevel: ..., maxBitrateInBps: ...}}, ...]

std::vector<firebolt::rialto::Mpeg2Profile> buildMpeg2Profiles(const YAML::Node &profilesNode)
{
    std::vector<firebolt::rialto::Mpeg2Profile> result;
    if (!profilesNode.IsSequence()) return result;
    for (const auto &entry : profilesNode)
    {
        for (YAML::const_iterator it = entry.begin(); it != entry.end(); ++it)
        {
            const std::string kName = it->first.as<std::string>();
            firebolt::rialto::Mpeg2Profile p{};
            if ("MPEG2_SIMPLE" == kName) p.type = firebolt::rialto::Mpeg2ProfileType::MPEG2_SIMPLE;
            else if ("MPEG2_MAIN" == kName) p.type = firebolt::rialto::Mpeg2ProfileType::MPEG2_MAIN;
            else continue;
            const auto &v = it->second;
            if (v["maxBitrateInBps"]) p.maxBitrateInBps = v["maxBitrateInBps"].as<uint64_t>();
            if (v["maxLevel"])
            {
                const std::string kLevel = v["maxLevel"].as<std::string>();
                if ("MPEG2_LEVEL_LOW" == kLevel)  p.maxLevel = firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_LOW;
                else if ("MPEG2_LEVEL_MAIN" == kLevel) p.maxLevel = firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_MAIN;
                else if ("MPEG2_LEVEL_HIGH" == kLevel) p.maxLevel = firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_HIGH;
            }
            result.push_back(p);
        }
    }
    return result;
}

std::vector<firebolt::rialto::H264Profile> buildH264Profiles(const YAML::Node &profilesNode)
{
    std::vector<firebolt::rialto::H264Profile> result;
    if (!profilesNode.IsSequence()) return result;
    for (const auto &entry : profilesNode)
    {
        for (YAML::const_iterator it = entry.begin(); it != entry.end(); ++it)
        {
            const std::string kName = it->first.as<std::string>();
            firebolt::rialto::H264Profile p{};
            if ("H264_BASELINE" == kName) p.type = firebolt::rialto::H264ProfileType::H264_BASELINE;
            else if ("H264_MAIN" == kName) p.type = firebolt::rialto::H264ProfileType::H264_MAIN;
            else if ("H264_HIGH" == kName) p.type = firebolt::rialto::H264ProfileType::H264_HIGH;
            else continue;
            const auto &v = it->second;
            if (v["maxBitrateInBps"]) p.maxBitrateInBps = v["maxBitrateInBps"].as<uint64_t>();
            if (v["maxLevel"])
            {
                const std::string kLevel = v["maxLevel"].as<std::string>();
                if ("H264_LEVEL_3" == kLevel)   p.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_3;
                else if ("H264_LEVEL_3_1" == kLevel) p.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_3_1;
                else if ("H264_LEVEL_4" == kLevel)   p.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_4;
                else if ("H264_LEVEL_4_1" == kLevel) p.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_4_1;
                else if ("H264_LEVEL_5" == kLevel)   p.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_5;
                else if ("H264_LEVEL_5_1" == kLevel) p.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_5_1;
                else if ("H264_LEVEL_5_2" == kLevel) p.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_5_2;
            }
            result.push_back(p);
        }
    }
    return result;
}

std::vector<firebolt::rialto::H265Profile> buildH265Profiles(const YAML::Node &profilesNode)
{
    std::vector<firebolt::rialto::H265Profile> result;
    if (!profilesNode.IsSequence()) return result;
    for (const auto &entry : profilesNode)
    {
        for (YAML::const_iterator it = entry.begin(); it != entry.end(); ++it)
        {
            const std::string kName = it->first.as<std::string>();
            firebolt::rialto::H265Profile p{};
            if ("H265_MAIN" == kName) p.type = firebolt::rialto::H265ProfileType::H265_MAIN;
            else if ("H265_MAIN_10" == kName) p.type = firebolt::rialto::H265ProfileType::H265_MAIN_10;
            else if ("H265_MAIN_10_HDR10" == kName) p.type = firebolt::rialto::H265ProfileType::H265_MAIN_10_HDR10;
            else continue;
            const auto &v = it->second;
            if (v["maxBitrateInBps"]) p.maxBitrateInBps = v["maxBitrateInBps"].as<uint64_t>();
            if (v["maxLevel"])
            {
                const std::string kLevel = v["maxLevel"].as<std::string>();
                if ("H265_LEVEL_4" == kLevel)   p.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_4;
                else if ("H265_LEVEL_4_1" == kLevel) p.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_4_1;
                else if ("H265_LEVEL_5" == kLevel)   p.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_5;
                else if ("H265_LEVEL_5_1" == kLevel) p.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_5_1;
                else if ("H265_LEVEL_5_2" == kLevel) p.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_5_2;
                else if ("H265_LEVEL_6" == kLevel)   p.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_6;
                else if ("H265_LEVEL_6_1" == kLevel) p.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_6_1;
                else if ("H265_LEVEL_6_2" == kLevel) p.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_6_2;
            }
            result.push_back(p);
        }
    }
    return result;
}

std::vector<firebolt::rialto::Vp9Profile> buildVp9Profiles(const YAML::Node &profilesNode)
{
    std::vector<firebolt::rialto::Vp9Profile> result;
    if (!profilesNode.IsSequence()) return result;
    for (const auto &entry : profilesNode)
    {
        for (YAML::const_iterator it = entry.begin(); it != entry.end(); ++it)
        {
            const std::string kName = it->first.as<std::string>();
            firebolt::rialto::Vp9Profile p{};
            if ("VP9_PROFILE_0" == kName) p.type = firebolt::rialto::Vp9ProfileType::VP9_PROFILE_0;
            else if ("VP9_PROFILE_1" == kName) p.type = firebolt::rialto::Vp9ProfileType::VP9_PROFILE_1;
            else if ("VP9_PROFILE_2" == kName) p.type = firebolt::rialto::Vp9ProfileType::VP9_PROFILE_2;
            else if ("VP9_PROFILE_3" == kName) p.type = firebolt::rialto::Vp9ProfileType::VP9_PROFILE_3;
            else continue;
            const auto &v = it->second;
            if (v["maxBitrateInBps"]) p.maxBitrateInBps = v["maxBitrateInBps"].as<uint64_t>();
            if (v["maxLevel"])
            {
                const std::string kLevel = v["maxLevel"].as<std::string>();
                if ("VP9_LEVEL_1" == kLevel)   p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_1;
                else if ("VP9_LEVEL_1_1" == kLevel) p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_1_1;
                else if ("VP9_LEVEL_2" == kLevel)   p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_2;
                else if ("VP9_LEVEL_2_1" == kLevel) p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_2_1;
                else if ("VP9_LEVEL_3" == kLevel)   p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_3;
                else if ("VP9_LEVEL_3_1" == kLevel) p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_3_1;
                else if ("VP9_LEVEL_4" == kLevel)   p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_4;
                else if ("VP9_LEVEL_4_1" == kLevel) p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_4_1;
                else if ("VP9_LEVEL_5" == kLevel)   p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_5;
                else if ("VP9_LEVEL_5_1" == kLevel) p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_5_1;
                else if ("VP9_LEVEL_5_2" == kLevel) p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_5_2;
                else if ("VP9_LEVEL_6" == kLevel)   p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_6;
                else if ("VP9_LEVEL_6_1" == kLevel) p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_6_1;
                else if ("VP9_LEVEL_6_2" == kLevel) p.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_6_2;
            }
            result.push_back(p);
        }
    }
    return result;
}

std::vector<firebolt::rialto::Av1Profile> buildAv1Profiles(const YAML::Node &profilesNode)
{
    std::vector<firebolt::rialto::Av1Profile> result;
    if (!profilesNode.IsSequence()) return result;
    for (const auto &entry : profilesNode)
    {
        for (YAML::const_iterator it = entry.begin(); it != entry.end(); ++it)
        {
            const std::string kName = it->first.as<std::string>();
            firebolt::rialto::Av1Profile p{};
            if ("AV1_MAIN" == kName) p.type = firebolt::rialto::Av1ProfileType::AV1_MAIN;
            else if ("AV1_HIGH" == kName) p.type = firebolt::rialto::Av1ProfileType::AV1_HIGH;
            else continue;
            const auto &v = it->second;
            if (v["maxBitrateInBps"]) p.maxBitrateInBps = v["maxBitrateInBps"].as<uint64_t>();
            if (v["maxLevel"])
            {
                const std::string kLevel = v["maxLevel"].as<std::string>();
                if ("AV1_LEVEL_4_0" == kLevel) p.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_4_0;
                else if ("AV1_LEVEL_4_1" == kLevel) p.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_4_1;
                else if ("AV1_LEVEL_5_0" == kLevel) p.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_5_0;
                else if ("AV1_LEVEL_5_1" == kLevel) p.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_5_1;
                else if ("AV1_LEVEL_5_2" == kLevel) p.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_5_2;
                else if ("AV1_LEVEL_6_0" == kLevel) p.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_6_0;
                else if ("AV1_LEVEL_6_1" == kLevel) p.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_6_1;
                else if ("AV1_LEVEL_6_2" == kLevel) p.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_6_2;
            }
            result.push_back(p);
        }
    }
    return result;
}

firebolt::rialto::VideoDecoderCapability buildVideoDecoderCapability(const YAML::Node &capability)
{
    firebolt::rialto::VideoDecoderCapability result;
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
                        firebolt::rialto::Mpeg2CodecCapability c;
                        c.profiles = buildMpeg2Profiles(codecNode["profiles"]);
                        c.dynamicRanges = getDynamicRanges(codecNode["dynamicRange"]);
                        result.codecCapabilities.mpeg2 = std::move(c);
                    }
                    else if ("H264_AVC" == kCodecName)
                    {
                        firebolt::rialto::H264CodecCapability c;
                        c.profiles = buildH264Profiles(codecNode["profiles"]);
                        c.dynamicRanges = getDynamicRanges(codecNode["dynamicRange"]);
                        result.codecCapabilities.h264 = std::move(c);
                    }
                    else if ("H265_HEVC" == kCodecName)
                    {
                        firebolt::rialto::H265CodecCapability c;
                        c.profiles = buildH265Profiles(codecNode["profiles"]);
                        c.dynamicRanges = getDynamicRanges(codecNode["dynamicRange"]);
                        result.codecCapabilities.h265 = std::move(c);
                    }
                    else if ("VP9" == kCodecName)
                    {
                        firebolt::rialto::Vp9CodecCapability c;
                        c.profiles = buildVp9Profiles(codecNode["profiles"]);
                        c.dynamicRanges = getDynamicRanges(codecNode["dynamicRange"]);
                        result.codecCapabilities.vp9 = std::move(c);
                    }
                    else if ("AV1" == kCodecName)
                    {
                        firebolt::rialto::Av1CodecCapability c;
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

DecoderCapabilitiesStatus YamlCppWrapper::getAudioDecoderCapabilities(AudioDecoderCapabilities &capabilities) const
try
{
    YAML::Node audioCapsFile = YAML::LoadFile(kAudioCapabilitiesFilePath);
    if (audioCapsFile.IsNull())
    {
        return DecoderCapabilitiesStatus::CONFIG_NOT_FOUND;
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
    return DecoderCapabilitiesStatus::OK;
}
catch (const std::exception &)
{
    return DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED;
}

DecoderCapabilitiesStatus YamlCppWrapper::getVideoDecoderCapabilities(VideoDecoderCapabilities &capabilities) const
try
{
    YAML::Node videoCapsFile = YAML::LoadFile(kVideoCapabilitiesFilePath);
    if (videoCapsFile.IsNull())
    {
        return DecoderCapabilitiesStatus::CONFIG_NOT_FOUND;
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
    return DecoderCapabilitiesStatus::OK;
}
catch (const std::exception &)
{
    return DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED;
}

} // namespace firebolt::rialto::wrappers
