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

std::vector<firebolt::rialto::DolbyAc3Profile> getDolbyAc3Profiles(const YAML::Node &codecData)
{
    std::vector<firebolt::rialto::DolbyAc3Profile> result;
    if (codecData["profiles"] && codecData["profiles"].IsSequence())
    {
        for (std::size_t i = 0; i < codecData["profiles"].size(); ++i)
        {
            const std::string kProfileName = codecData["profiles"][i].as<std::string>();
            if ("STANDARD" == kProfileName)
            {
                result.push_back(firebolt::rialto::DolbyAc3Profile::STANDARD);
            }
            else if ("PLUS" == kProfileName)
            {
                result.push_back(firebolt::rialto::DolbyAc3Profile::PLUS);
            }
            else if ("PLUS_JOC" == kProfileName)
            {
                result.push_back(firebolt::rialto::DolbyAc3Profile::PLUS_JOC);
            }
        }
    }
    return result;
}

std::vector<firebolt::rialto::DolbyMatProfile> getDolbyMatProfiles(const YAML::Node &codecData)
{
    std::vector<firebolt::rialto::DolbyMatProfile> result;
    if (codecData["profiles"] && codecData["profiles"].IsSequence())
    {
        for (std::size_t i = 0; i < codecData["profiles"].size(); ++i)
        {
            const std::string kProfileName = codecData["profiles"][i].as<std::string>();
            if ("V1" == kProfileName)
            {
                result.push_back(firebolt::rialto::DolbyMatProfile::V1);
            }
            else if ("V2" == kProfileName)
            {
                result.push_back(firebolt::rialto::DolbyMatProfile::V2);
            }
        }
    }
    return result;
}

std::vector<firebolt::rialto::WmaProfile> getWmaProfiles(const YAML::Node &codecData)
{
    std::vector<firebolt::rialto::WmaProfile> result;
    if (codecData["profiles"] && codecData["profiles"].IsSequence())
    {
        for (std::size_t i = 0; i < codecData["profiles"].size(); ++i)
        {
            const std::string kProfileName = codecData["profiles"][i].as<std::string>();
            if ("STANDARD" == kProfileName)
            {
                result.push_back(firebolt::rialto::WmaProfile::STANDARD);
            }
            else if ("PRO" == kProfileName)
            {
                result.push_back(firebolt::rialto::WmaProfile::PRO);
            }
            else if ("LOSSLESS" == kProfileName)
            {
                result.push_back(firebolt::rialto::WmaProfile::LOSSLESS);
            }
        }
    }
    return result;
}

std::vector<firebolt::rialto::RealAudioProfile> getRealAudioProfiles(const YAML::Node &codecData)
{
    std::vector<firebolt::rialto::RealAudioProfile> result;
    if (codecData["profiles"] && codecData["profiles"].IsSequence())
    {
        for (std::size_t i = 0; i < codecData["profiles"].size(); ++i)
        {
            const std::string kProfileName = codecData["profiles"][i].as<std::string>();
            if ("RA8" == kProfileName)
            {
                result.push_back(firebolt::rialto::RealAudioProfile::RA8);
            }
            else if ("RA10" == kProfileName)
            {
                result.push_back(firebolt::rialto::RealAudioProfile::RA10);
            }
        }
    }
    return result;
}

std::vector<firebolt::rialto::UsacProfile> getUsacProfiles(const YAML::Node &codecData)
{
    std::vector<firebolt::rialto::UsacProfile> result;
    if (codecData["profiles"] && codecData["profiles"].IsSequence())
    {
        for (std::size_t i = 0; i < codecData["profiles"].size(); ++i)
        {
            const std::string kProfileName = codecData["profiles"][i].as<std::string>();
            if ("BASELINE" == kProfileName)
            {
                result.push_back(firebolt::rialto::UsacProfile::BASELINE);
            }
            else if ("EXTENDED_HE_AAC" == kProfileName)
            {
                result.push_back(firebolt::rialto::UsacProfile::EXTENDED_HE_AAC);
            }
        }
    }
    return result;
}

std::vector<firebolt::rialto::DtsProfile> getDtsProfiles(const YAML::Node &codecData)
{
    std::vector<firebolt::rialto::DtsProfile> result;
    if (codecData["profiles"] && codecData["profiles"].IsSequence())
    {
        for (std::size_t i = 0; i < codecData["profiles"].size(); ++i)
        {
            const std::string kProfileName = codecData["profiles"][i].as<std::string>();
            if ("CORE" == kProfileName)
            {
                result.push_back(firebolt::rialto::DtsProfile::CORE);
            }
            else if ("HD_HRA" == kProfileName)
            {
                result.push_back(firebolt::rialto::DtsProfile::HD_HRA);
            }
            else if ("HD_MA" == kProfileName)
            {
                result.push_back(firebolt::rialto::DtsProfile::HD_MA);
            }
        }
    }
    return result;
}

std::vector<firebolt::rialto::AvsProfile> getAvsProfiles(const YAML::Node &codecData)
{
    std::vector<firebolt::rialto::AvsProfile> result;
    if (codecData["profiles"] && codecData["profiles"].IsSequence())
    {
        for (std::size_t i = 0; i < codecData["profiles"].size(); ++i)
        {
            const std::string kProfileName = codecData["profiles"][i].as<std::string>();
            if ("AVS1_PART2" == kProfileName)
            {
                result.push_back(firebolt::rialto::AvsProfile::AVS1_PART2);
            }
            else if ("AVS2" == kProfileName)
            {
                result.push_back(firebolt::rialto::AvsProfile::AVS2);
            }
            else if ("AVS3" == kProfileName)
            {
                result.push_back(firebolt::rialto::AvsProfile::AVS3);
            }
        }
    }
    return result;
}

uint64_t getMaxBitrateInBps(const YAML::Node &codecData)
{
    if (codecData["maxBitrateInBps"])
    {
        return codecData["maxBitrateInBps"].as<uint64_t>();
    }
    return 0;
}

uint32_t getMaxChannels(const YAML::Node &codecData)
{
    if (codecData["maxChannels"])
    {
        return codecData["maxChannels"].as<uint32_t>();
    }
    return 0;
}

uint32_t getMaxSampleRateInHz(const YAML::Node &codecData)
{
    if (codecData["maxSampleRateInHz"])
    {
        return codecData["maxSampleRateInHz"].as<uint32_t>();
    }
    return 0;
}

std::optional<uint32_t> getMaxBitDepth(const YAML::Node &codecData)
{
    if (codecData["maxBitDepth"])
    {
        return codecData["maxBitDepth"].as<uint32_t>();
    }
    return std::nullopt;
}

template <typename T> void getCommonAudioParams(T &capability, const YAML::Node &codecData)
{
    capability.maxBitrateInBps = getMaxBitrateInBps(codecData);
    capability.maxChannels = getMaxChannels(codecData);
    capability.maxSampleRateInHz = getMaxSampleRateInHz(codecData);
    capability.maxBitDepth = getMaxBitDepth(codecData);
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
                        result.pcm = firebolt::rialto::PcmCapability{};
                        getCommonAudioParams(*result.pcm, kCodecData);
                    }
                    else if ("AAC" == kCodecName)
                    {
                        result.aac = firebolt::rialto::AacCapability{};
                        result.aac->profiles = getAacProfiles(kCodecData);
                        getCommonAudioParams(*result.aac, kCodecData);
                    }
                    else if ("MPEG_AUDIO" == kCodecName)
                    {
                        result.mpegAudio = firebolt::rialto::MpegAudioCapability{};
                        getCommonAudioParams(*result.mpegAudio, kCodecData);
                    }
                    else if ("MP3" == kCodecName)
                    {
                        result.mp3 = firebolt::rialto::Mp3Capability{};
                        getCommonAudioParams(*result.mp3, kCodecData);
                    }
                    else if ("ALAC" == kCodecName)
                    {
                        result.alac = firebolt::rialto::AlacCapability{};
                        getCommonAudioParams(*result.alac, kCodecData);
                    }
                    else if ("SBC" == kCodecName)
                    {
                        result.sbc = firebolt::rialto::SbcCapability{};
                        getCommonAudioParams(*result.sbc, kCodecData);
                    }
                    else if ("DOLBY_AC3" == kCodecName)
                    {
                        result.dolbyAc3 = firebolt::rialto::DolbyAc3Capability{};
                        result.dolbyAc3->profiles = getDolbyAc3Profiles(kCodecData);
                        getCommonAudioParams(*result.dolbyAc3, kCodecData);
                    }
                    else if ("DOLBY_AC4" == kCodecName)
                    {
                        result.dolbyAc4 = firebolt::rialto::DolbyAc4Capability{};
                        getCommonAudioParams(*result.dolbyAc4, kCodecData);
                    }
                    else if ("DOLBY_MAT" == kCodecName)
                    {
                        result.dolbyMat = firebolt::rialto::DolbyMatCapability{};
                        result.dolbyMat->profiles = getDolbyMatProfiles(kCodecData);
                        getCommonAudioParams(*result.dolbyMat, kCodecData);
                    }
                    else if ("DOLBY_TRUEHD" == kCodecName)
                    {
                        result.dolbyTruehd = firebolt::rialto::DolbyTruehdCapability{};
                        getCommonAudioParams(*result.dolbyTruehd, kCodecData);
                    }
                    else if ("FLAC" == kCodecName)
                    {
                        result.flac = firebolt::rialto::FlacCapability{};
                        getCommonAudioParams(*result.flac, kCodecData);
                    }
                    else if ("VORBIS" == kCodecName)
                    {
                        result.vorbis = firebolt::rialto::VorbisCapability{};
                        getCommonAudioParams(*result.vorbis, kCodecData);
                    }
                    else if ("OPUS" == kCodecName)
                    {
                        result.opus = firebolt::rialto::OpusCapability{};
                        getCommonAudioParams(*result.opus, kCodecData);
                    }
                    else if ("WMA" == kCodecName)
                    {
                        result.wma = firebolt::rialto::WmaCapability{};
                        result.wma->profiles = getWmaProfiles(kCodecData);
                        getCommonAudioParams(*result.wma, kCodecData);
                    }
                    else if ("REALAUDIO" == kCodecName)
                    {
                        result.realAudio = firebolt::rialto::RealAudioCapability{};
                        result.realAudio->profiles = getRealAudioProfiles(kCodecData);
                        getCommonAudioParams(*result.realAudio, kCodecData);
                    }
                    else if ("USAC" == kCodecName)
                    {
                        result.usac = firebolt::rialto::UsacCapability{};
                        result.usac->profiles = getUsacProfiles(kCodecData);
                        getCommonAudioParams(*result.usac, kCodecData);
                    }
                    else if ("DTS" == kCodecName)
                    {
                        result.dts = firebolt::rialto::DtsCapability{};
                        result.dts->profiles = getDtsProfiles(kCodecData);
                        getCommonAudioParams(*result.dts, kCodecData);
                    }
                    else if ("AVS" == kCodecName)
                    {
                        result.avs = firebolt::rialto::AvsCapability{};
                        result.avs->profiles = getAvsProfiles(kCodecData);
                        getCommonAudioParams(*result.avs, kCodecData);
                    }
                }
            }
        }
    }
    return result;
}

std::vector<firebolt::rialto::Mpeg2Profile> buildMpeg2Profiles(const YAML::Node &profilesNode)
{
    std::vector<firebolt::rialto::Mpeg2Profile> result;
    for (const auto &profiles : profilesNode)
    {
        for (YAML::const_iterator profilesIt = profiles.begin(); profilesIt != profiles.end(); ++profilesIt)
        {
            firebolt::rialto::Mpeg2Profile profile;
            const std::string kProfileType{profilesIt->first.as<std::string>()};
            const std::string kMaxLevel{profilesIt->second["maxLevel"].as<std::string>()};
            if ("MPEG2_MAIN" == kProfileType)
            {
                profile.type = firebolt::rialto::Mpeg2ProfileType::MPEG2_MAIN;
            }
            else if ("MPEG2_SIMPLE" == kProfileType)
            {
                profile.type = firebolt::rialto::Mpeg2ProfileType::MPEG2_SIMPLE;
            }

            if ("MPEG2_LEVEL_LOW" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_LOW;
            }
            else if ("MPEG2_LEVEL_MAIN" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_MAIN;
            }
            else if ("MPEG2_LEVEL_HIGH" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_HIGH;
            }
            profile.maxBitrateInBps = profilesIt->second["maxBitrateInBps"].as<uint64_t>();
            result.push_back(profile);
        }
    }
    return result;
}

std::vector<firebolt::rialto::H264Profile> buildH264Profiles(const YAML::Node &profilesNode)
{
    std::vector<firebolt::rialto::H264Profile> result;
    for (const auto &profiles : profilesNode)
    {
        for (YAML::const_iterator profilesIt = profiles.begin(); profilesIt != profiles.end(); ++profilesIt)
        {
            firebolt::rialto::H264Profile profile;
            const std::string kProfileType{profilesIt->first.as<std::string>()};
            const std::string kMaxLevel{profilesIt->second["maxLevel"].as<std::string>()};
            if ("H264_BASELINE" == kProfileType)
            {
                profile.type = firebolt::rialto::H264ProfileType::H264_BASELINE;
            }
            else if ("H264_MAIN" == kProfileType)
            {
                profile.type = firebolt::rialto::H264ProfileType::H264_MAIN;
            }
            else if ("H264_HIGH" == kProfileType)
            {
                profile.type = firebolt::rialto::H264ProfileType::H264_HIGH;
            }

            if ("H264_LEVEL_3" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_3;
            }
            else if ("H264_LEVEL_3_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_3_1;
            }
            else if ("H264_LEVEL_4" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_4;
            }
            else if ("H264_LEVEL_4_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_4_1;
            }
            else if ("H264_LEVEL_5" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_5;
            }
            else if ("H264_LEVEL_5_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_5_1;
            }
            else if ("H264_LEVEL_5_2" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H264Level::H264_LEVEL_5_2;
            }
            profile.maxBitrateInBps = profilesIt->second["maxBitrateInBps"].as<uint64_t>();
            result.push_back(profile);
        }
    }
    return result;
}

std::vector<firebolt::rialto::H265Profile> buildH265Profiles(const YAML::Node &profilesNode)
{
    std::vector<firebolt::rialto::H265Profile> result;
    for (const auto &profiles : profilesNode)
    {
        for (YAML::const_iterator profilesIt = profiles.begin(); profilesIt != profiles.end(); ++profilesIt)
        {
            firebolt::rialto::H265Profile profile;
            const std::string kProfileType{profilesIt->first.as<std::string>()};
            const std::string kMaxLevel{profilesIt->second["maxLevel"].as<std::string>()};
            if ("H265_MAIN" == kProfileType)
            {
                profile.type = firebolt::rialto::H265ProfileType::H265_MAIN;
            }
            else if ("H265_MAIN_10" == kProfileType)
            {
                profile.type = firebolt::rialto::H265ProfileType::H265_MAIN_10;
            }
            else if ("H265_MAIN_10_HDR10" == kProfileType)
            {
                profile.type = firebolt::rialto::H265ProfileType::H265_MAIN_10_HDR10;
            }

            if ("H265_LEVEL_4" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_4;
            }
            else if ("H265_LEVEL_4_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_4_1;
            }
            else if ("H265_LEVEL_5" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_5;
            }
            else if ("H265_LEVEL_5_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_5_1;
            }
            else if ("H265_LEVEL_5_2" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_5_2;
            }
            else if ("H265_LEVEL_6" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_6;
            }
            else if ("H265_LEVEL_6_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_6_1;
            }
            else if ("H265_LEVEL_6_2" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::H265Level::H265_LEVEL_6_2;
            }
            profile.maxBitrateInBps = profilesIt->second["maxBitrateInBps"].as<uint64_t>();
            result.push_back(profile);
        }
    }
    return result;
}

std::vector<firebolt::rialto::Vp9Profile> buildVp9Profiles(const YAML::Node &profilesNode)
{
    std::vector<firebolt::rialto::Vp9Profile> result;
    for (const auto &profiles : profilesNode)
    {
        for (YAML::const_iterator profilesIt = profiles.begin(); profilesIt != profiles.end(); ++profilesIt)
        {
            firebolt::rialto::Vp9Profile profile;
            const std::string kProfileType{profilesIt->first.as<std::string>()};
            const std::string kMaxLevel{profilesIt->second["maxLevel"].as<std::string>()};
            if ("VP9_PROFILE_0" == kProfileType)
            {
                profile.type = firebolt::rialto::Vp9ProfileType::VP9_PROFILE_0;
            }
            else if ("VP9_PROFILE_1" == kProfileType)
            {
                profile.type = firebolt::rialto::Vp9ProfileType::VP9_PROFILE_1;
            }
            else if ("VP9_PROFILE_2" == kProfileType)
            {
                profile.type = firebolt::rialto::Vp9ProfileType::VP9_PROFILE_2;
            }
            else if ("VP9_PROFILE_3" == kProfileType)
            {
                profile.type = firebolt::rialto::Vp9ProfileType::VP9_PROFILE_3;
            }

            if ("VP9_LEVEL_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_1;
            }
            else if ("VP9_LEVEL_1_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_1_1;
            }
            else if ("VP9_LEVEL_2" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_2;
            }
            else if ("VP9_LEVEL_2_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_2_1;
            }
            else if ("VP9_LEVEL_3" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_3;
            }
            else if ("VP9_LEVEL_3_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_3_1;
            }
            else if ("VP9_LEVEL_4" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_4;
            }
            else if ("VP9_LEVEL_4_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_4_1;
            }
            else if ("VP9_LEVEL_5" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_5;
            }
            else if ("VP9_LEVEL_5_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_5_1;
            }
            else if ("VP9_LEVEL_5_2" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_5_2;
            }
            else if ("VP9_LEVEL_6" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_6;
            }
            else if ("VP9_LEVEL_6_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_6_1;
            }
            else if ("VP9_LEVEL_6_2" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Vp9Level::VP9_LEVEL_6_2;
            }
            profile.maxBitrateInBps = profilesIt->second["maxBitrateInBps"].as<uint64_t>();
            result.push_back(profile);
        }
    }
    return result;
}

std::vector<firebolt::rialto::Av1Profile> buildAv1Profiles(const YAML::Node &profilesNode)
{
    std::vector<firebolt::rialto::Av1Profile> result;
    for (const auto &profiles : profilesNode)
    {
        for (YAML::const_iterator profilesIt = profiles.begin(); profilesIt != profiles.end(); ++profilesIt)
        {
            firebolt::rialto::Av1Profile profile;
            const std::string kProfileType{profilesIt->first.as<std::string>()};
            const std::string kMaxLevel{profilesIt->second["maxLevel"].as<std::string>()};
            if ("AV1_MAIN" == kProfileType)
            {
                profile.type = firebolt::rialto::Av1ProfileType::AV1_MAIN;
            }
            else if ("AV1_HIGH" == kProfileType)
            {
                profile.type = firebolt::rialto::Av1ProfileType::AV1_HIGH;
            }

            if ("AV1_LEVEL_4_0" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_4_0;
            }
            else if ("AV1_LEVEL_4_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_4_1;
            }
            else if ("AV1_LEVEL_5_0" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_5_0;
            }
            else if ("AV1_LEVEL_5_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_5_1;
            }
            else if ("AV1_LEVEL_5_2" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_5_2;
            }
            else if ("AV1_LEVEL_6_0" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_6_0;
            }
            else if ("AV1_LEVEL_6_1" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_6_1;
            }
            else if ("AV1_LEVEL_6_2" == kMaxLevel)
            {
                profile.maxLevel = firebolt::rialto::Av1Level::AV1_LEVEL_6_2;
            }
            profile.maxBitrateInBps = profilesIt->second["maxBitrateInBps"].as<uint64_t>();
            result.push_back(profile);
        }
    }
    return result;
}

std::vector<firebolt::rialto::DynamicRange> getDynamicRanges(const YAML::Node &ranges)
{
    std::vector<firebolt::rialto::DynamicRange> result;
    if (ranges && ranges.IsSequence())
    {
        for (std::size_t i = 0; i < ranges.size(); ++i)
        {
            const std::string kRange{ranges[i].as<std::string>()};
            if ("SDR" == kRange)
            {
                result.push_back(firebolt::rialto::DynamicRange::SDR);
            }
            else if ("HLG" == kRange)
            {
                result.push_back(firebolt::rialto::DynamicRange::HLG);
            }
            else if ("HDR10" == kRange)
            {
                result.push_back(firebolt::rialto::DynamicRange::HDR10);
            }
            else if ("HDR10PLUS" == kRange)
            {
                result.push_back(firebolt::rialto::DynamicRange::HDR10PLUS);
            }
            else if ("DOLBY_VISION" == kRange)
            {
                result.push_back(firebolt::rialto::DynamicRange::DOLBY_VISION);
            }
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
                    if ("MPEG2_VIDEO" == kCodecName)
                    {
                        result.codecCapabilities.mpeg2Profiles =
                            buildMpeg2Profiles(codecCapabilitiesIt->second["profiles"]);
                    }
                    else if ("H264_AVC" == kCodecName)
                    {
                        result.codecCapabilities.h264Profiles =
                            buildH264Profiles(codecCapabilitiesIt->second["profiles"]);
                    }
                    else if ("H265_HEVC" == kCodecName)
                    {
                        result.codecCapabilities.h265Profiles =
                            buildH265Profiles(codecCapabilitiesIt->second["profiles"]);
                    }
                    else if ("VP9" == kCodecName)
                    {
                        result.codecCapabilities.vp9Profiles =
                            buildVp9Profiles(codecCapabilitiesIt->second["profiles"]);
                    }
                    else if ("AV1" == kCodecName)
                    {
                        result.codecCapabilities.av1Profiles =
                            buildAv1Profiles(codecCapabilitiesIt->second["profiles"]);
                    }
                }
            }
        }
        result.dynamicRanges = getDynamicRanges(capabilitiesIt->second["dynamicRange"]);
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
catch (const std::exception &e)
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
catch (const std::exception &e)
{
    return DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED;
}

} // namespace firebolt::rialto::wrappers
