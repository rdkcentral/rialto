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

#include "MediaPipelineCapabilitiesIpc.h"
#include "RialtoClientLogging.h"
#include "RialtoCommonIpc.h"

namespace
{
using AudioCapabilitiesResponse = firebolt::rialto::GetSupportedAudioCapabilitiesResponse;

std::optional<uint32_t> convertOptionalUint32(bool has, uint32_t value)
{
    return has ? std::optional<uint32_t>(value) : std::nullopt;
}

firebolt::rialto::AacProfile convertAacProfile(const AudioCapabilitiesResponse::AacProfile &proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::AAC_PROFILE_LC:
        return firebolt::rialto::AacProfile::LC;
    case AudioCapabilitiesResponse::AAC_PROFILE_HE_V1:
        return firebolt::rialto::AacProfile::HE_V1;
    case AudioCapabilitiesResponse::AAC_PROFILE_HE_V2:
        return firebolt::rialto::AacProfile::HE_V2;
    case AudioCapabilitiesResponse::AAC_PROFILE_ELD:
        return firebolt::rialto::AacProfile::ELD;
    case AudioCapabilitiesResponse::AAC_PROFILE_X_HE:
        return firebolt::rialto::AacProfile::X_HE;
    default:
        return firebolt::rialto::AacProfile::LC;
    }
}

firebolt::rialto::DolbyAc3Profile convertDolbyAc3Profile(const AudioCapabilitiesResponse::DolbyAc3Profile &proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::DOLBY_AC3_PROFILE_STANDARD:
        return firebolt::rialto::DolbyAc3Profile::STANDARD;
    case AudioCapabilitiesResponse::DOLBY_AC3_PROFILE_PLUS:
        return firebolt::rialto::DolbyAc3Profile::PLUS;
    case AudioCapabilitiesResponse::DOLBY_AC3_PROFILE_PLUS_JOC:
        return firebolt::rialto::DolbyAc3Profile::PLUS_JOC;
    default:
        return firebolt::rialto::DolbyAc3Profile::STANDARD;
    }
}

firebolt::rialto::DolbyMatProfile convertDolbyMatProfile(const AudioCapabilitiesResponse::DolbyMatProfile &proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::DOLBY_MAT_PROFILE_V1:
        return firebolt::rialto::DolbyMatProfile::V1;
    case AudioCapabilitiesResponse::DOLBY_MAT_PROFILE_V2:
        return firebolt::rialto::DolbyMatProfile::V2;
    default:
        return firebolt::rialto::DolbyMatProfile::V1;
    }
}

firebolt::rialto::WmaProfile convertWmaProfile(const AudioCapabilitiesResponse::WmaProfile &proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::WMA_PROFILE_STANDARD:
        return firebolt::rialto::WmaProfile::STANDARD;
    case AudioCapabilitiesResponse::WMA_PROFILE_PRO:
        return firebolt::rialto::WmaProfile::PRO;
    case AudioCapabilitiesResponse::WMA_PROFILE_LOSSLESS:
        return firebolt::rialto::WmaProfile::LOSSLESS;
    default:
        return firebolt::rialto::WmaProfile::STANDARD;
    }
}

firebolt::rialto::RealAudioProfile convertRealAudioProfile(const AudioCapabilitiesResponse::RealAudioProfile &proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA8:
        return firebolt::rialto::RealAudioProfile::RA8;
    case AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA10:
        return firebolt::rialto::RealAudioProfile::RA10;
    default:
        return firebolt::rialto::RealAudioProfile::RA8;
    }
}

firebolt::rialto::UsacProfile convertUsacProfile(const AudioCapabilitiesResponse::UsacProfile &proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::USAC_PROFILE_BASELINE:
        return firebolt::rialto::UsacProfile::BASELINE;
    case AudioCapabilitiesResponse::USAC_PROFILE_EXTENDED_HE_AAC:
        return firebolt::rialto::UsacProfile::EXTENDED_HE_AAC;
    default:
        return firebolt::rialto::UsacProfile::BASELINE;
    }
}

firebolt::rialto::DtsProfile convertDtsProfile(const AudioCapabilitiesResponse::DtsProfile &proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::DTS_PROFILE_CORE:
        return firebolt::rialto::DtsProfile::CORE;
    case AudioCapabilitiesResponse::DTS_PROFILE_HD_HRA:
        return firebolt::rialto::DtsProfile::HD_HRA;
    case AudioCapabilitiesResponse::DTS_PROFILE_HD_MA:
        return firebolt::rialto::DtsProfile::HD_MA;
    default:
        return firebolt::rialto::DtsProfile::CORE;
    }
}

firebolt::rialto::AvsProfile convertAvsProfile(const AudioCapabilitiesResponse::AvsProfile &proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::AVS_PROFILE_AVS1_PART2:
        return firebolt::rialto::AvsProfile::AVS1_PART2;
    case AudioCapabilitiesResponse::AVS_PROFILE_AVS2:
        return firebolt::rialto::AvsProfile::AVS2;
    case AudioCapabilitiesResponse::AVS_PROFILE_AVS3:
        return firebolt::rialto::AvsProfile::AVS3;
    default:
        return firebolt::rialto::AvsProfile::AVS1_PART2;
    }
}

firebolt::rialto::PcmCapability convertPcmCapability(const AudioCapabilitiesResponse::PcmCapability &proto)
{
    return {proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::AacCapability convertAacCapability(const AudioCapabilitiesResponse::AacCapability &proto)
{
    std::vector<firebolt::rialto::AacProfile> profiles;
    for (int i = 0; i < proto.profiles_size(); ++i)
        profiles.push_back(convertAacProfile(proto.profiles(i)));
    return {std::move(profiles), proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::MpegAudioCapability convertMpegAudioCapability(const AudioCapabilitiesResponse::MpegAudioCapability &proto)
{
    return {proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::Mp3Capability convertMp3Capability(const AudioCapabilitiesResponse::Mp3Capability &proto)
{
    return {proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::AlacCapability convertAlacCapability(const AudioCapabilitiesResponse::AlacCapability &proto)
{
    return {proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::SbcCapability convertSbcCapability(const AudioCapabilitiesResponse::SbcCapability &proto)
{
    return {proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::DolbyAc3Capability convertDolbyAc3Capability(const AudioCapabilitiesResponse::DolbyAc3Capability &proto)
{
    std::vector<firebolt::rialto::DolbyAc3Profile> profiles;
    for (int i = 0; i < proto.profiles_size(); ++i)
        profiles.push_back(convertDolbyAc3Profile(proto.profiles(i)));
    return {std::move(profiles), proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::DolbyAc4Capability convertDolbyAc4Capability(const AudioCapabilitiesResponse::DolbyAc4Capability &proto)
{
    return {proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::DolbyMatCapability convertDolbyMatCapability(const AudioCapabilitiesResponse::DolbyMatCapability &proto)
{
    std::vector<firebolt::rialto::DolbyMatProfile> profiles;
    for (int i = 0; i < proto.profiles_size(); ++i)
        profiles.push_back(convertDolbyMatProfile(proto.profiles(i)));
    return {std::move(profiles), proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::DolbyTruehdCapability
convertDolbyTruehdCapability(const AudioCapabilitiesResponse::DolbyTruehdCapability &proto)
{
    return {proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::FlacCapability convertFlacCapability(const AudioCapabilitiesResponse::FlacCapability &proto)
{
    return {proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::VorbisCapability convertVorbisCapability(const AudioCapabilitiesResponse::VorbisCapability &proto)
{
    return {proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::OpusCapability convertOpusCapability(const AudioCapabilitiesResponse::OpusCapability &proto)
{
    return {proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::WmaCapability convertWmaCapability(const AudioCapabilitiesResponse::WmaCapability &proto)
{
    std::vector<firebolt::rialto::WmaProfile> profiles;
    for (int i = 0; i < proto.profiles_size(); ++i)
        profiles.push_back(convertWmaProfile(proto.profiles(i)));
    return {std::move(profiles), proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::RealAudioCapability convertRealAudioCapability(const AudioCapabilitiesResponse::RealAudioCapability &proto)
{
    std::vector<firebolt::rialto::RealAudioProfile> profiles;
    for (int i = 0; i < proto.profiles_size(); ++i)
        profiles.push_back(convertRealAudioProfile(proto.profiles(i)));
    return {std::move(profiles), proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::UsacCapability convertUsacCapability(const AudioCapabilitiesResponse::UsacCapability &proto)
{
    std::vector<firebolt::rialto::UsacProfile> profiles;
    for (int i = 0; i < proto.profiles_size(); ++i)
        profiles.push_back(convertUsacProfile(proto.profiles(i)));
    return {std::move(profiles), proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::DtsCapability convertDtsCapability(const AudioCapabilitiesResponse::DtsCapability &proto)
{
    std::vector<firebolt::rialto::DtsProfile> profiles;
    for (int i = 0; i < proto.profiles_size(); ++i)
        profiles.push_back(convertDtsProfile(proto.profiles(i)));
    return {std::move(profiles), proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::AvsCapability convertAvsCapability(const AudioCapabilitiesResponse::AvsCapability &proto)
{
    std::vector<firebolt::rialto::AvsProfile> profiles;
    for (int i = 0; i < proto.profiles_size(); ++i)
        profiles.push_back(convertAvsProfile(proto.profiles(i)));
    return {std::move(profiles), proto.max_bitrate_in_bps(), proto.max_channels(), proto.max_sample_rate_in_hz(),
            convertOptionalUint32(proto.has_max_bit_depth(), proto.max_bit_depth())};
}

firebolt::rialto::AudioDecoderCapability
convertAudioDecoderCapability(const AudioCapabilitiesResponse::AudioDecoderCapability &proto)
{
    firebolt::rialto::AudioDecoderCapability result;
    if (proto.has_pcm())
        result.pcm = convertPcmCapability(proto.pcm());
    if (proto.has_aac())
        result.aac = convertAacCapability(proto.aac());
    if (proto.has_mpeg_audio())
        result.mpegAudio = convertMpegAudioCapability(proto.mpeg_audio());
    if (proto.has_mp3())
        result.mp3 = convertMp3Capability(proto.mp3());
    if (proto.has_alac())
        result.alac = convertAlacCapability(proto.alac());
    if (proto.has_sbc())
        result.sbc = convertSbcCapability(proto.sbc());
    if (proto.has_dolby_ac3())
        result.dolbyAc3 = convertDolbyAc3Capability(proto.dolby_ac3());
    if (proto.has_dolby_ac4())
        result.dolbyAc4 = convertDolbyAc4Capability(proto.dolby_ac4());
    if (proto.has_dolby_mat())
        result.dolbyMat = convertDolbyMatCapability(proto.dolby_mat());
    if (proto.has_dolby_truehd())
        result.dolbyTruehd = convertDolbyTruehdCapability(proto.dolby_truehd());
    if (proto.has_flac())
        result.flac = convertFlacCapability(proto.flac());
    if (proto.has_vorbis())
        result.vorbis = convertVorbisCapability(proto.vorbis());
    if (proto.has_opus())
        result.opus = convertOpusCapability(proto.opus());
    if (proto.has_wma())
        result.wma = convertWmaCapability(proto.wma());
    if (proto.has_real_audio())
        result.realAudio = convertRealAudioCapability(proto.real_audio());
    if (proto.has_usac())
        result.usac = convertUsacCapability(proto.usac());
    if (proto.has_dts())
        result.dts = convertDtsCapability(proto.dts());
    if (proto.has_avs())
        result.avs = convertAvsCapability(proto.avs());
    return result;
}

firebolt::rialto::AudioDecoderCapabilities convertAudioDecoderCapabilities(const AudioCapabilitiesResponse &response)
{
    firebolt::rialto::AudioDecoderCapabilities result;
    result.interfaceVersion = response.interface_version();
    result.schemaVersion = response.schema_version();
    for (const auto &cap : response.capabilities())
    {
        result.capabilities.push_back(convertAudioDecoderCapability(cap));
    }
    return result;
}

using VideoCapabilitiesResponse = firebolt::rialto::GetSupportedVideoCapabilitiesResponse;

firebolt::rialto::DynamicRange convertDynamicRange(VideoCapabilitiesResponse::DynamicRange proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::DYNAMIC_RANGE_SDR:
        return firebolt::rialto::DynamicRange::SDR;
    case VideoCapabilitiesResponse::DYNAMIC_RANGE_HLG:
        return firebolt::rialto::DynamicRange::HLG;
    case VideoCapabilitiesResponse::DYNAMIC_RANGE_HDR10:
        return firebolt::rialto::DynamicRange::HDR10;
    case VideoCapabilitiesResponse::DYNAMIC_RANGE_HDR10PLUS:
        return firebolt::rialto::DynamicRange::HDR10PLUS;
    case VideoCapabilitiesResponse::DYNAMIC_RANGE_DOLBY_VISION:
        return firebolt::rialto::DynamicRange::DOLBY_VISION;
    default:
        return firebolt::rialto::DynamicRange::SDR;
    }
}

firebolt::rialto::Mpeg2ProfileType convertMpeg2ProfileType(VideoCapabilitiesResponse::Mpeg2ProfileType proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::MPEG2_PROFILE_MAIN:
        return firebolt::rialto::Mpeg2ProfileType::MPEG2_MAIN;
    case VideoCapabilitiesResponse::MPEG2_PROFILE_SIMPLE:
        return firebolt::rialto::Mpeg2ProfileType::MPEG2_SIMPLE;
    default:
        return firebolt::rialto::Mpeg2ProfileType::MPEG2_MAIN;
    }
}

firebolt::rialto::Mpeg2Level convertMpeg2Level(VideoCapabilitiesResponse::Mpeg2Level proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::MPEG2_LEVEL_LOW:
        return firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_LOW;
    case VideoCapabilitiesResponse::MPEG2_LEVEL_MAIN:
        return firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_MAIN;
    case VideoCapabilitiesResponse::MPEG2_LEVEL_HIGH:
        return firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_HIGH;
    default:
        return firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_LOW;
    }
}

firebolt::rialto::H264ProfileType convertH264ProfileType(VideoCapabilitiesResponse::H264ProfileType proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::H264_PROFILE_BASELINE:
        return firebolt::rialto::H264ProfileType::H264_BASELINE;
    case VideoCapabilitiesResponse::H264_PROFILE_MAIN:
        return firebolt::rialto::H264ProfileType::H264_MAIN;
    case VideoCapabilitiesResponse::H264_PROFILE_HIGH:
        return firebolt::rialto::H264ProfileType::H264_HIGH;
    default:
        return firebolt::rialto::H264ProfileType::H264_BASELINE;
    }
}

firebolt::rialto::H264Level convertH264Level(VideoCapabilitiesResponse::H264Level proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::H264_LEVEL_3:
        return firebolt::rialto::H264Level::H264_LEVEL_3;
    case VideoCapabilitiesResponse::H264_LEVEL_3_1:
        return firebolt::rialto::H264Level::H264_LEVEL_3_1;
    case VideoCapabilitiesResponse::H264_LEVEL_4:
        return firebolt::rialto::H264Level::H264_LEVEL_4;
    case VideoCapabilitiesResponse::H264_LEVEL_4_1:
        return firebolt::rialto::H264Level::H264_LEVEL_4_1;
    case VideoCapabilitiesResponse::H264_LEVEL_5:
        return firebolt::rialto::H264Level::H264_LEVEL_5;
    case VideoCapabilitiesResponse::H264_LEVEL_5_1:
        return firebolt::rialto::H264Level::H264_LEVEL_5_1;
    case VideoCapabilitiesResponse::H264_LEVEL_5_2:
        return firebolt::rialto::H264Level::H264_LEVEL_5_2;
    default:
        return firebolt::rialto::H264Level::H264_LEVEL_3;
    }
}

firebolt::rialto::H265ProfileType convertH265ProfileType(VideoCapabilitiesResponse::H265ProfileType proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::H265_PROFILE_MAIN:
        return firebolt::rialto::H265ProfileType::H265_MAIN;
    case VideoCapabilitiesResponse::H265_PROFILE_MAIN_10:
        return firebolt::rialto::H265ProfileType::H265_MAIN_10;
    case VideoCapabilitiesResponse::H265_PROFILE_MAIN_10_HDR10:
        return firebolt::rialto::H265ProfileType::H265_MAIN_10_HDR10;
    default:
        return firebolt::rialto::H265ProfileType::H265_MAIN;
    }
}

firebolt::rialto::H265Level convertH265Level(VideoCapabilitiesResponse::H265Level proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::H265_LEVEL_4:
        return firebolt::rialto::H265Level::H265_LEVEL_4;
    case VideoCapabilitiesResponse::H265_LEVEL_4_1:
        return firebolt::rialto::H265Level::H265_LEVEL_4_1;
    case VideoCapabilitiesResponse::H265_LEVEL_5:
        return firebolt::rialto::H265Level::H265_LEVEL_5;
    case VideoCapabilitiesResponse::H265_LEVEL_5_1:
        return firebolt::rialto::H265Level::H265_LEVEL_5_1;
    case VideoCapabilitiesResponse::H265_LEVEL_5_2:
        return firebolt::rialto::H265Level::H265_LEVEL_5_2;
    case VideoCapabilitiesResponse::H265_LEVEL_6:
        return firebolt::rialto::H265Level::H265_LEVEL_6;
    case VideoCapabilitiesResponse::H265_LEVEL_6_1:
        return firebolt::rialto::H265Level::H265_LEVEL_6_1;
    case VideoCapabilitiesResponse::H265_LEVEL_6_2:
        return firebolt::rialto::H265Level::H265_LEVEL_6_2;
    default:
        return firebolt::rialto::H265Level::H265_LEVEL_4;
    }
}

firebolt::rialto::Vp9ProfileType convertVp9ProfileType(VideoCapabilitiesResponse::Vp9ProfileType proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::VP9_PROFILE_0:
        return firebolt::rialto::Vp9ProfileType::VP9_PROFILE_0;
    case VideoCapabilitiesResponse::VP9_PROFILE_1:
        return firebolt::rialto::Vp9ProfileType::VP9_PROFILE_1;
    case VideoCapabilitiesResponse::VP9_PROFILE_2:
        return firebolt::rialto::Vp9ProfileType::VP9_PROFILE_2;
    case VideoCapabilitiesResponse::VP9_PROFILE_3:
        return firebolt::rialto::Vp9ProfileType::VP9_PROFILE_3;
    default:
        return firebolt::rialto::Vp9ProfileType::VP9_PROFILE_0;
    }
}

firebolt::rialto::Vp9Level convertVp9Level(VideoCapabilitiesResponse::Vp9Level proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::VP9_LEVEL_1:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_1;
    case VideoCapabilitiesResponse::VP9_LEVEL_1_1:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_1_1;
    case VideoCapabilitiesResponse::VP9_LEVEL_2:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_2;
    case VideoCapabilitiesResponse::VP9_LEVEL_2_1:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_2_1;
    case VideoCapabilitiesResponse::VP9_LEVEL_3:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_3;
    case VideoCapabilitiesResponse::VP9_LEVEL_3_1:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_3_1;
    case VideoCapabilitiesResponse::VP9_LEVEL_4:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_4;
    case VideoCapabilitiesResponse::VP9_LEVEL_4_1:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_4_1;
    case VideoCapabilitiesResponse::VP9_LEVEL_5:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_5;
    case VideoCapabilitiesResponse::VP9_LEVEL_5_1:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_5_1;
    case VideoCapabilitiesResponse::VP9_LEVEL_5_2:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_5_2;
    case VideoCapabilitiesResponse::VP9_LEVEL_6:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_6;
    case VideoCapabilitiesResponse::VP9_LEVEL_6_1:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_6_1;
    case VideoCapabilitiesResponse::VP9_LEVEL_6_2:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_6_2;
    default:
        return firebolt::rialto::Vp9Level::VP9_LEVEL_1;
    }
}

firebolt::rialto::Av1ProfileType convertAv1ProfileType(VideoCapabilitiesResponse::Av1ProfileType proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::AV1_PROFILE_MAIN:
        return firebolt::rialto::Av1ProfileType::AV1_MAIN;
    case VideoCapabilitiesResponse::AV1_PROFILE_HIGH:
        return firebolt::rialto::Av1ProfileType::AV1_HIGH;
    default:
        return firebolt::rialto::Av1ProfileType::AV1_MAIN;
    }
}

firebolt::rialto::Av1Level convertAv1Level(VideoCapabilitiesResponse::Av1Level proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::AV1_LEVEL_4_0:
        return firebolt::rialto::Av1Level::AV1_LEVEL_4_0;
    case VideoCapabilitiesResponse::AV1_LEVEL_4_1:
        return firebolt::rialto::Av1Level::AV1_LEVEL_4_1;
    case VideoCapabilitiesResponse::AV1_LEVEL_5_0:
        return firebolt::rialto::Av1Level::AV1_LEVEL_5_0;
    case VideoCapabilitiesResponse::AV1_LEVEL_5_1:
        return firebolt::rialto::Av1Level::AV1_LEVEL_5_1;
    case VideoCapabilitiesResponse::AV1_LEVEL_5_2:
        return firebolt::rialto::Av1Level::AV1_LEVEL_5_2;
    case VideoCapabilitiesResponse::AV1_LEVEL_6_0:
        return firebolt::rialto::Av1Level::AV1_LEVEL_6_0;
    case VideoCapabilitiesResponse::AV1_LEVEL_6_1:
        return firebolt::rialto::Av1Level::AV1_LEVEL_6_1;
    case VideoCapabilitiesResponse::AV1_LEVEL_6_2:
        return firebolt::rialto::Av1Level::AV1_LEVEL_6_2;
    default:
        return firebolt::rialto::Av1Level::AV1_LEVEL_4_0;
    }
}

firebolt::rialto::Mpeg2Profile convertMpeg2Profile(const VideoCapabilitiesResponse::Mpeg2Profile &proto)
{
    return {convertMpeg2ProfileType(proto.type()), convertMpeg2Level(proto.max_level()), proto.max_bitrate_in_bps()};
}

firebolt::rialto::H264Profile convertH264Profile(const VideoCapabilitiesResponse::H264Profile &proto)
{
    return {convertH264ProfileType(proto.type()), convertH264Level(proto.max_level()), proto.max_bitrate_in_bps()};
}

firebolt::rialto::H265Profile convertH265Profile(const VideoCapabilitiesResponse::H265Profile &proto)
{
    return {convertH265ProfileType(proto.type()), convertH265Level(proto.max_level()), proto.max_bitrate_in_bps()};
}

firebolt::rialto::Vp9Profile convertVp9Profile(const VideoCapabilitiesResponse::Vp9Profile &proto)
{
    return {convertVp9ProfileType(proto.type()), convertVp9Level(proto.max_level()), proto.max_bitrate_in_bps()};
}

firebolt::rialto::Av1Profile convertAv1Profile(const VideoCapabilitiesResponse::Av1Profile &proto)
{
    return {convertAv1ProfileType(proto.type()), convertAv1Level(proto.max_level()), proto.max_bitrate_in_bps()};
}

firebolt::rialto::VideoCodecCapabilities
convertVideoCodecCapabilities(const VideoCapabilitiesResponse::VideoCodecCapabilities &proto)
{
    firebolt::rialto::VideoCodecCapabilities result;
    for (const auto &p : proto.mpeg2_profiles())
        result.mpeg2Profiles.push_back(convertMpeg2Profile(p));
    for (const auto &p : proto.h264_profiles())
        result.h264Profiles.push_back(convertH264Profile(p));
    for (const auto &p : proto.h265_profiles())
        result.h265Profiles.push_back(convertH265Profile(p));
    for (const auto &p : proto.vp9_profiles())
        result.vp9Profiles.push_back(convertVp9Profile(p));
    for (const auto &p : proto.av1_profiles())
        result.av1Profiles.push_back(convertAv1Profile(p));
    return result;
}

firebolt::rialto::VideoDecoderCapability
convertVideoDecoderCapability(const VideoCapabilitiesResponse::VideoDecoderCapability &proto)
{
    firebolt::rialto::VideoDecoderCapability result;
    if (proto.has_codec_capabilities())
        result.codecCapabilities = convertVideoCodecCapabilities(proto.codec_capabilities());
    for (int i = 0; i < proto.dynamic_ranges_size(); ++i)
        result.dynamicRanges.push_back(convertDynamicRange(proto.dynamic_ranges(i)));
    return result;
}

firebolt::rialto::VideoDecoderCapabilities convertVideoDecoderCapabilities(const VideoCapabilitiesResponse &response)
{
    firebolt::rialto::VideoDecoderCapabilities result;
    result.interfaceVersion = response.interface_version();
    result.schemaVersion = response.schema_version();
    for (const auto &cap : response.capabilities())
    {
        result.capabilities.push_back(convertVideoDecoderCapability(cap));
    }
    return result;
}
} // namespace

namespace firebolt::rialto::client
{
std::shared_ptr<IMediaPipelineCapabilitiesIpcFactory> IMediaPipelineCapabilitiesIpcFactory::createFactory()
{
    std::shared_ptr<IMediaPipelineCapabilitiesIpcFactory> factory;

    try
    {
        factory = std::make_shared<MediaPipelineCapabilitiesIpcFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the media pipeline capabilities ipc factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IMediaPipelineCapabilities> MediaPipelineCapabilitiesIpcFactory::createMediaPipelineCapabilitiesIpc() const
{
    std::unique_ptr<IMediaPipelineCapabilities> mediaPipelineCapabilitiesIpc;

    try
    {
        mediaPipelineCapabilitiesIpc =
            std::make_unique<client::MediaPipelineCapabilitiesIpc>(IIpcClientAccessor::instance().getIpcClient());
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the media pipeline capabilities ipc, reason: %s", e.what());
    }

    return mediaPipelineCapabilitiesIpc;
}

MediaPipelineCapabilitiesIpc::MediaPipelineCapabilitiesIpc(IIpcClient &ipcClient) : IpcModule(ipcClient)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    if (!attachChannel())
    {
        throw std::runtime_error("Failed attach to the ipc channel");
    }
}

MediaPipelineCapabilitiesIpc::~MediaPipelineCapabilitiesIpc()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    detachChannel();
}

bool MediaPipelineCapabilitiesIpc::createRpcStubs(const std::shared_ptr<ipc::IChannel> &ipcChannel)
{
    m_mediaPipelineCapabilitiesStub =
        std::make_unique<::firebolt::rialto::MediaPipelineCapabilitiesModule_Stub>(ipcChannel.get());
    if (!m_mediaPipelineCapabilitiesStub)
    {
        return false;
    }
    return true;
}

std::vector<std::string> MediaPipelineCapabilitiesIpc::getSupportedMimeTypes(MediaSourceType sourceType)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return {};
    }

    firebolt::rialto::GetSupportedMimeTypesRequest request;
    request.set_media_type(convertProtoMediaSourceType(sourceType));

    firebolt::rialto::GetSupportedMimeTypesResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_mediaPipelineCapabilitiesStub->getSupportedMimeTypes(ipcController.get(), &request, &response,
                                                           blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to get supported mime types due to '%s'", ipcController->ErrorText().c_str());
        return {};
    }

    return std::vector<std::string>{response.mime_types().begin(), response.mime_types().end()};
}

bool MediaPipelineCapabilitiesIpc::isMimeTypeSupported(const std::string &mimeType)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::IsMimeTypeSupportedRequest request;
    request.set_mime_type(mimeType);

    firebolt::rialto::IsMimeTypeSupportedResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_mediaPipelineCapabilitiesStub->isMimeTypeSupported(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to check if mime type '%s' is supported due to '%s'", mimeType.c_str(),
                                ipcController->ErrorText().c_str());
        return false;
    }

    return response.is_supported();
}

std::vector<std::string> MediaPipelineCapabilitiesIpc::getSupportedProperties(MediaSourceType mediaType,
                                                                              const std::vector<std::string> &propertyNames)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return {};
    }

    firebolt::rialto::GetSupportedPropertiesRequest request;
    request.set_media_type(convertProtoMediaSourceType(mediaType));
    for (const std::string &property : propertyNames)
        request.add_property_names(property);

    firebolt::rialto::GetSupportedPropertiesResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_mediaPipelineCapabilitiesStub->getSupportedProperties(ipcController.get(), &request, &response,
                                                            blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed due to '%s'", ipcController->ErrorText().c_str());
        return std::vector<std::string>{};
    }

    return std::vector<std::string>{response.supported_properties().begin(), response.supported_properties().end()};
}

bool MediaPipelineCapabilitiesIpc::isVideoMaster(bool &isVideoMaster)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return {};
    }

    firebolt::rialto::IsVideoMasterRequest request;
    firebolt::rialto::IsVideoMasterResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_mediaPipelineCapabilitiesStub->isVideoMaster(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    isVideoMaster = response.is_video_master();

    return true;
}

AudioDecoderCapabilities MediaPipelineCapabilitiesIpc::getSupportedAudioCapabilities()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return AudioDecoderCapabilities{};
    }

    firebolt::rialto::GetSupportedAudioCapabilitiesRequest request;
    firebolt::rialto::GetSupportedAudioCapabilitiesResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_mediaPipelineCapabilitiesStub->getSupportedAudioCapabilities(ipcController.get(), &request, &response,
                                                                   blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed due to '%s'", ipcController->ErrorText().c_str());
        return AudioDecoderCapabilities{};
    }

    return convertAudioDecoderCapabilities(response);
}

VideoDecoderCapabilities MediaPipelineCapabilitiesIpc::getSupportedVideoCapabilities()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return VideoDecoderCapabilities{};
    }

    firebolt::rialto::GetSupportedVideoCapabilitiesRequest request;
    firebolt::rialto::GetSupportedVideoCapabilitiesResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_mediaPipelineCapabilitiesStub->getSupportedVideoCapabilities(ipcController.get(), &request, &response,
                                                                   blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed due to '%s'", ipcController->ErrorText().c_str());
        return VideoDecoderCapabilities{};
    }

    return convertVideoDecoderCapabilities(response);
}
}; // namespace firebolt::rialto::client
