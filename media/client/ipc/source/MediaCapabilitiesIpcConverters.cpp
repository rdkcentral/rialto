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

#include "MediaCapabilitiesIpcConverters.h"

namespace firebolt::rialto::client
{
namespace
{
using AudioCapabilitiesResponse = firebolt::rialto::GetSupportedAudioCapabilitiesResponse;
using VideoCapabilitiesResponse = firebolt::rialto::GetSupportedVideoCapabilitiesResponse;

// ---------- Audio converter helpers ----------

firebolt::rialto::common::AudioProfileCapability
convertAudioProfileCapability(const AudioCapabilitiesResponse::AudioProfileCapability &proto)
{
    firebolt::rialto::common::AudioProfileCapability cap{};
    if (proto.has_max_bitrate_in_bps())
        cap.maxBitrateInBps = proto.max_bitrate_in_bps();
    if (proto.has_max_channels())
        cap.maxChannels = proto.max_channels();
    if (proto.has_max_sample_rate_in_hz())
        cap.maxSampleRateInHz = proto.max_sample_rate_in_hz();
    if (proto.has_max_bit_depth())
        cap.maxBitDepth = proto.max_bit_depth();
    return cap;
}

firebolt::rialto::common::AacProfile convertAacProfile(AudioCapabilitiesResponse::AacProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::AAC_PROFILE_LC:
        return firebolt::rialto::common::AacProfile::LC;
    case AudioCapabilitiesResponse::AAC_PROFILE_HE_V1:
        return firebolt::rialto::common::AacProfile::HE_V1;
    case AudioCapabilitiesResponse::AAC_PROFILE_HE_V2:
        return firebolt::rialto::common::AacProfile::HE_V2;
    case AudioCapabilitiesResponse::AAC_PROFILE_ELD:
        return firebolt::rialto::common::AacProfile::ELD;
    case AudioCapabilitiesResponse::AAC_PROFILE_X_HE:
        return firebolt::rialto::common::AacProfile::X_HE;
    }
    return firebolt::rialto::common::AacProfile::LC;
}

firebolt::rialto::common::DolbyAc3Profile convertDolbyAc3Profile(AudioCapabilitiesResponse::DolbyAc3Profile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::DOLBY_AC3_PROFILE_STANDARD:
        return firebolt::rialto::common::DolbyAc3Profile::STANDARD;
    }
    return firebolt::rialto::common::DolbyAc3Profile::STANDARD;
}

firebolt::rialto::common::DolbyEac3Profile convertDolbyEac3Profile(AudioCapabilitiesResponse::DolbyEac3Profile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::DOLBY_EAC3_PROFILE_PLUS:
        return firebolt::rialto::common::DolbyEac3Profile::PLUS;
    case AudioCapabilitiesResponse::DOLBY_EAC3_PROFILE_PLUS_JOC:
        return firebolt::rialto::common::DolbyEac3Profile::PLUS_JOC;
    }
    return firebolt::rialto::common::DolbyEac3Profile::PLUS;
}

firebolt::rialto::common::MpegAudioProfile convertMpegAudioProfile(AudioCapabilitiesResponse::MpegAudioProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::MPEG_AUDIO_PROFILE_LAYER_1:
        return firebolt::rialto::common::MpegAudioProfile::LAYER_1;
    case AudioCapabilitiesResponse::MPEG_AUDIO_PROFILE_LAYER_2:
        return firebolt::rialto::common::MpegAudioProfile::LAYER_2;
    }
    return firebolt::rialto::common::MpegAudioProfile::LAYER_1;
}

firebolt::rialto::common::RealAudioProfile convertRealAudioProfile(AudioCapabilitiesResponse::RealAudioProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA8:
        return firebolt::rialto::common::RealAudioProfile::RA8;
    case AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA10:
        return firebolt::rialto::common::RealAudioProfile::RA10;
    }
    return firebolt::rialto::common::RealAudioProfile::RA8;
}

firebolt::rialto::common::UsacProfile convertUsacProfile(AudioCapabilitiesResponse::UsacProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::USAC_PROFILE_BASELINE:
        return firebolt::rialto::common::UsacProfile::BASELINE;
    case AudioCapabilitiesResponse::USAC_PROFILE_EXTENDED_HE_AAC:
        return firebolt::rialto::common::UsacProfile::EXTENDED_HE_AAC;
    }
    return firebolt::rialto::common::UsacProfile::BASELINE;
}

firebolt::rialto::common::DtsProfile convertDtsProfile(AudioCapabilitiesResponse::DtsProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::DTS_PROFILE_CORE:
        return firebolt::rialto::common::DtsProfile::CORE;
    case AudioCapabilitiesResponse::DTS_PROFILE_HD_HRA:
        return firebolt::rialto::common::DtsProfile::HD_HRA;
    case AudioCapabilitiesResponse::DTS_PROFILE_HD_MA:
        return firebolt::rialto::common::DtsProfile::HD_MA;
    }
    return firebolt::rialto::common::DtsProfile::CORE;
}

firebolt::rialto::common::AvsProfile convertAvsProfile(AudioCapabilitiesResponse::AvsProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::AVS_PROFILE_AVS1_PART2:
        return firebolt::rialto::common::AvsProfile::AVS1_PART2;
    case AudioCapabilitiesResponse::AVS_PROFILE_AVS2:
        return firebolt::rialto::common::AvsProfile::AVS2;
    case AudioCapabilitiesResponse::AVS_PROFILE_AVS3:
        return firebolt::rialto::common::AvsProfile::AVS3;
    }
    return firebolt::rialto::common::AvsProfile::AVS1_PART2;
}

template <typename MapType, typename ProtoCapType, typename ProfileConverter>
MapType deserializeNamedProfileMap(const ProtoCapType &proto, ProfileConverter convertProfile)
{
    MapType result;
    for (int i = 0; i < proto.profiles_size(); ++i)
    {
        const auto &entry = proto.profiles(i);
        if (entry.has_profile() && entry.has_capability())
            result.emplace(convertProfile(entry.profile()), convertAudioProfileCapability(entry.capability()));
    }
    return result;
}

firebolt::rialto::common::AudioDecoderCapability
convertAudioDecoderCapability(const AudioCapabilitiesResponse::AudioDecoderCapability &proto)
{
    firebolt::rialto::common::AudioDecoderCapability result;

    if (proto.has_pcm())
        result.pcm = firebolt::rialto::common::PcmCapability{convertAudioProfileCapability(proto.pcm().base())};
    if (proto.has_mp3())
        result.mp3 = firebolt::rialto::common::Mp3Capability{convertAudioProfileCapability(proto.mp3().base())};
    if (proto.has_alac())
        result.alac = firebolt::rialto::common::AlacCapability{convertAudioProfileCapability(proto.alac().base())};
    if (proto.has_sbc())
        result.sbc = firebolt::rialto::common::SbcCapability{convertAudioProfileCapability(proto.sbc().base())};
    if (proto.has_dolby_ac4())
        result.dolbyAc4 =
            firebolt::rialto::common::DolbyAc4Capability{convertAudioProfileCapability(proto.dolby_ac4().base())};
    if (proto.has_dolby_truehd())
        result.dolbyTruehd =
            firebolt::rialto::common::DolbyTruehdCapability{convertAudioProfileCapability(proto.dolby_truehd().base())};
    if (proto.has_flac())
        result.flac = firebolt::rialto::common::FlacCapability{convertAudioProfileCapability(proto.flac().base())};
    if (proto.has_vorbis())
        result.vorbis = firebolt::rialto::common::VorbisCapability{convertAudioProfileCapability(proto.vorbis().base())};
    if (proto.has_opus())
        result.opus = firebolt::rialto::common::OpusCapability{convertAudioProfileCapability(proto.opus().base())};

    if (proto.has_aac())
        result.aac = firebolt::rialto::common::AacCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::common::AacProfile,
                                                firebolt::rialto::common::AudioProfileCapability>>(proto.aac(),
                                                                                                   convertAacProfile)};
    if (proto.has_mpeg_audio())
        result.mpegAudio = firebolt::rialto::common::MpegAudioCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::common::MpegAudioProfile,
                                                firebolt::rialto::common::AudioProfileCapability>>(proto.mpeg_audio(),
                                                                                                   convertMpegAudioProfile)};
    if (proto.has_dolby_ac3())
        result.dolbyAc3 = firebolt::rialto::common::DolbyAc3Capability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::common::DolbyAc3Profile,
                                                firebolt::rialto::common::AudioProfileCapability>>(proto.dolby_ac3(),
                                                                                                   convertDolbyAc3Profile)};
    if (proto.has_dolby_eac3())
        result.dolbyEac3 = firebolt::rialto::common::DolbyEac3Capability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::common::DolbyEac3Profile,
                                                firebolt::rialto::common::AudioProfileCapability>>(proto.dolby_eac3(),
                                                                                                   convertDolbyEac3Profile)};
    if (proto.has_real_audio())
        result.realAudio = firebolt::rialto::common::RealAudioCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::common::RealAudioProfile,
                                                firebolt::rialto::common::AudioProfileCapability>>(proto.real_audio(),
                                                                                                   convertRealAudioProfile)};
    if (proto.has_usac())
        result.usac = firebolt::rialto::common::UsacCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::common::UsacProfile,
                                                firebolt::rialto::common::AudioProfileCapability>>(proto.usac(),
                                                                                                   convertUsacProfile)};
    if (proto.has_dts())
        result.dts = firebolt::rialto::common::DtsCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::common::DtsProfile,
                                                firebolt::rialto::common::AudioProfileCapability>>(proto.dts(),
                                                                                                   convertDtsProfile)};
    if (proto.has_avs())
        result.avs = firebolt::rialto::common::AvsCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::common::AvsProfile,
                                                firebolt::rialto::common::AudioProfileCapability>>(proto.avs(),
                                                                                                   convertAvsProfile)};

    return result;
}

// ---------- Video converter helpers ----------

firebolt::rialto::common::DynamicRange convertDynamicRange(VideoCapabilitiesResponse::DynamicRange proto)
{
    switch (proto)
    {
    case VideoCapabilitiesResponse::DYNAMIC_RANGE_SDR:
        return firebolt::rialto::common::DynamicRange::SDR;
    case VideoCapabilitiesResponse::DYNAMIC_RANGE_HLG:
        return firebolt::rialto::common::DynamicRange::HLG;
    case VideoCapabilitiesResponse::DYNAMIC_RANGE_HDR10:
        return firebolt::rialto::common::DynamicRange::HDR10;
    case VideoCapabilitiesResponse::DYNAMIC_RANGE_HDR10PLUS:
        return firebolt::rialto::common::DynamicRange::HDR10PLUS;
    case VideoCapabilitiesResponse::DYNAMIC_RANGE_DOLBY_VISION:
        return firebolt::rialto::common::DynamicRange::DOLBY_VISION;
    }
    return firebolt::rialto::common::DynamicRange::SDR;
}

firebolt::rialto::common::Mpeg2Profile convertMpeg2Profile(const VideoCapabilitiesResponse::Mpeg2Profile &p)
{
    firebolt::rialto::common::Mpeg2Profile r{};
    if (p.has_type())
    {
        switch (p.type())
        {
        case VideoCapabilitiesResponse::MPEG2_PROFILE_MAIN:
            r.type = firebolt::rialto::common::Mpeg2ProfileType::MPEG2_MAIN;
            break;
        case VideoCapabilitiesResponse::MPEG2_PROFILE_SIMPLE:
            r.type = firebolt::rialto::common::Mpeg2ProfileType::MPEG2_SIMPLE;
            break;
        }
    }
    if (p.has_max_level())
    {
        switch (p.max_level())
        {
        case VideoCapabilitiesResponse::MPEG2_LEVEL_LOW:
            r.maxLevel = firebolt::rialto::common::Mpeg2Level::MPEG2_LEVEL_LOW;
            break;
        case VideoCapabilitiesResponse::MPEG2_LEVEL_MAIN:
            r.maxLevel = firebolt::rialto::common::Mpeg2Level::MPEG2_LEVEL_MAIN;
            break;
        case VideoCapabilitiesResponse::MPEG2_LEVEL_HIGH:
            r.maxLevel = firebolt::rialto::common::Mpeg2Level::MPEG2_LEVEL_HIGH;
            break;
        }
    }
    if (p.has_max_bitrate_in_bps())
        r.maxBitrateInBps = p.max_bitrate_in_bps();
    return r;
}

firebolt::rialto::common::H264Profile convertH264Profile(const VideoCapabilitiesResponse::H264Profile &p)
{
    firebolt::rialto::common::H264Profile r{};
    if (p.has_type())
    {
        switch (p.type())
        {
        case VideoCapabilitiesResponse::H264_PROFILE_BASELINE:
            r.type = firebolt::rialto::common::H264ProfileType::H264_BASELINE;
            break;
        case VideoCapabilitiesResponse::H264_PROFILE_MAIN:
            r.type = firebolt::rialto::common::H264ProfileType::H264_MAIN;
            break;
        case VideoCapabilitiesResponse::H264_PROFILE_HIGH:
            r.type = firebolt::rialto::common::H264ProfileType::H264_HIGH;
            break;
        }
    }
    if (p.has_max_level())
    {
        switch (p.max_level())
        {
        case VideoCapabilitiesResponse::H264_LEVEL_3:
            r.maxLevel = firebolt::rialto::common::H264Level::H264_LEVEL_3;
            break;
        case VideoCapabilitiesResponse::H264_LEVEL_3_1:
            r.maxLevel = firebolt::rialto::common::H264Level::H264_LEVEL_3_1;
            break;
        case VideoCapabilitiesResponse::H264_LEVEL_4:
            r.maxLevel = firebolt::rialto::common::H264Level::H264_LEVEL_4;
            break;
        case VideoCapabilitiesResponse::H264_LEVEL_4_1:
            r.maxLevel = firebolt::rialto::common::H264Level::H264_LEVEL_4_1;
            break;
        case VideoCapabilitiesResponse::H264_LEVEL_5:
            r.maxLevel = firebolt::rialto::common::H264Level::H264_LEVEL_5;
            break;
        case VideoCapabilitiesResponse::H264_LEVEL_5_1:
            r.maxLevel = firebolt::rialto::common::H264Level::H264_LEVEL_5_1;
            break;
        case VideoCapabilitiesResponse::H264_LEVEL_5_2:
            r.maxLevel = firebolt::rialto::common::H264Level::H264_LEVEL_5_2;
            break;
        }
    }
    if (p.has_max_bitrate_in_bps())
        r.maxBitrateInBps = p.max_bitrate_in_bps();
    return r;
}

firebolt::rialto::common::H265Profile convertH265Profile(const VideoCapabilitiesResponse::H265Profile &p)
{
    firebolt::rialto::common::H265Profile r{};
    if (p.has_type())
    {
        switch (p.type())
        {
        case VideoCapabilitiesResponse::H265_PROFILE_MAIN:
            r.type = firebolt::rialto::common::H265ProfileType::H265_MAIN;
            break;
        case VideoCapabilitiesResponse::H265_PROFILE_MAIN_10:
            r.type = firebolt::rialto::common::H265ProfileType::H265_MAIN_10;
            break;
        case VideoCapabilitiesResponse::H265_PROFILE_MAIN_10_HDR10:
            r.type = firebolt::rialto::common::H265ProfileType::H265_MAIN_10_HDR10;
            break;
        }
    }
    if (p.has_max_level())
    {
        switch (p.max_level())
        {
        case VideoCapabilitiesResponse::H265_LEVEL_4:
            r.maxLevel = firebolt::rialto::common::H265Level::H265_LEVEL_4;
            break;
        case VideoCapabilitiesResponse::H265_LEVEL_4_1:
            r.maxLevel = firebolt::rialto::common::H265Level::H265_LEVEL_4_1;
            break;
        case VideoCapabilitiesResponse::H265_LEVEL_5:
            r.maxLevel = firebolt::rialto::common::H265Level::H265_LEVEL_5;
            break;
        case VideoCapabilitiesResponse::H265_LEVEL_5_1:
            r.maxLevel = firebolt::rialto::common::H265Level::H265_LEVEL_5_1;
            break;
        case VideoCapabilitiesResponse::H265_LEVEL_5_2:
            r.maxLevel = firebolt::rialto::common::H265Level::H265_LEVEL_5_2;
            break;
        case VideoCapabilitiesResponse::H265_LEVEL_6:
            r.maxLevel = firebolt::rialto::common::H265Level::H265_LEVEL_6;
            break;
        case VideoCapabilitiesResponse::H265_LEVEL_6_1:
            r.maxLevel = firebolt::rialto::common::H265Level::H265_LEVEL_6_1;
            break;
        case VideoCapabilitiesResponse::H265_LEVEL_6_2:
            r.maxLevel = firebolt::rialto::common::H265Level::H265_LEVEL_6_2;
            break;
        }
    }
    if (p.has_max_bitrate_in_bps())
        r.maxBitrateInBps = p.max_bitrate_in_bps();
    return r;
}

firebolt::rialto::common::Vp9Profile convertVp9Profile(const VideoCapabilitiesResponse::Vp9Profile &p)
{
    firebolt::rialto::common::Vp9Profile r{};
    if (p.has_type())
    {
        switch (p.type())
        {
        case VideoCapabilitiesResponse::VP9_PROFILE_0:
            r.type = firebolt::rialto::common::Vp9ProfileType::VP9_PROFILE_0;
            break;
        case VideoCapabilitiesResponse::VP9_PROFILE_1:
            r.type = firebolt::rialto::common::Vp9ProfileType::VP9_PROFILE_1;
            break;
        case VideoCapabilitiesResponse::VP9_PROFILE_2:
            r.type = firebolt::rialto::common::Vp9ProfileType::VP9_PROFILE_2;
            break;
        case VideoCapabilitiesResponse::VP9_PROFILE_3:
            r.type = firebolt::rialto::common::Vp9ProfileType::VP9_PROFILE_3;
            break;
        }
    }
    if (p.has_max_level())
    {
        switch (p.max_level())
        {
        case VideoCapabilitiesResponse::VP9_LEVEL_1:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_1;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_1_1:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_1_1;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_2:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_2;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_2_1:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_2_1;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_3:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_3;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_3_1:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_3_1;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_4:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_4;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_4_1:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_4_1;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_5:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_5;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_5_1:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_5_1;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_5_2:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_5_2;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_6:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_6;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_6_1:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_6_1;
            break;
        case VideoCapabilitiesResponse::VP9_LEVEL_6_2:
            r.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_6_2;
            break;
        }
    }
    if (p.has_max_bitrate_in_bps())
        r.maxBitrateInBps = p.max_bitrate_in_bps();
    return r;
}

firebolt::rialto::common::Av1Profile convertAv1Profile(const VideoCapabilitiesResponse::Av1Profile &p)
{
    firebolt::rialto::common::Av1Profile r{};
    if (p.has_type())
    {
        switch (p.type())
        {
        case VideoCapabilitiesResponse::AV1_PROFILE_MAIN:
            r.type = firebolt::rialto::common::Av1ProfileType::AV1_MAIN;
            break;
        case VideoCapabilitiesResponse::AV1_PROFILE_HIGH:
            r.type = firebolt::rialto::common::Av1ProfileType::AV1_HIGH;
            break;
        }
    }
    if (p.has_max_level())
    {
        switch (p.max_level())
        {
        case VideoCapabilitiesResponse::AV1_LEVEL_4_0:
            r.maxLevel = firebolt::rialto::common::Av1Level::AV1_LEVEL_4_0;
            break;
        case VideoCapabilitiesResponse::AV1_LEVEL_4_1:
            r.maxLevel = firebolt::rialto::common::Av1Level::AV1_LEVEL_4_1;
            break;
        case VideoCapabilitiesResponse::AV1_LEVEL_5_0:
            r.maxLevel = firebolt::rialto::common::Av1Level::AV1_LEVEL_5_0;
            break;
        case VideoCapabilitiesResponse::AV1_LEVEL_5_1:
            r.maxLevel = firebolt::rialto::common::Av1Level::AV1_LEVEL_5_1;
            break;
        case VideoCapabilitiesResponse::AV1_LEVEL_5_2:
            r.maxLevel = firebolt::rialto::common::Av1Level::AV1_LEVEL_5_2;
            break;
        case VideoCapabilitiesResponse::AV1_LEVEL_6_0:
            r.maxLevel = firebolt::rialto::common::Av1Level::AV1_LEVEL_6_0;
            break;
        case VideoCapabilitiesResponse::AV1_LEVEL_6_1:
            r.maxLevel = firebolt::rialto::common::Av1Level::AV1_LEVEL_6_1;
            break;
        case VideoCapabilitiesResponse::AV1_LEVEL_6_2:
            r.maxLevel = firebolt::rialto::common::Av1Level::AV1_LEVEL_6_2;
            break;
        }
    }
    if (p.has_max_bitrate_in_bps())
        r.maxBitrateInBps = p.max_bitrate_in_bps();
    return r;
}

firebolt::rialto::common::VideoCodecCapabilities
convertVideoCodecCapabilities(const VideoCapabilitiesResponse::VideoCodecCapabilities &proto)
{
    firebolt::rialto::common::VideoCodecCapabilities result;

    auto fillDr = [](const auto &protoCodec, std::vector<firebolt::rialto::common::DynamicRange> &out)
    {
        for (int i = 0; i < protoCodec.dynamic_ranges_size(); ++i)
            out.push_back(convertDynamicRange(protoCodec.dynamic_ranges(i)));
    };

    if (proto.has_mpeg2())
    {
        firebolt::rialto::common::Mpeg2CodecCapability c;
        for (const auto &p : proto.mpeg2().profiles())
            c.profiles.push_back(convertMpeg2Profile(p));
        fillDr(proto.mpeg2(), c.dynamicRanges);
        result.mpeg2 = std::move(c);
    }
    if (proto.has_h264())
    {
        firebolt::rialto::common::H264CodecCapability c;
        for (const auto &p : proto.h264().profiles())
            c.profiles.push_back(convertH264Profile(p));
        fillDr(proto.h264(), c.dynamicRanges);
        result.h264 = std::move(c);
    }
    if (proto.has_h265())
    {
        firebolt::rialto::common::H265CodecCapability c;
        for (const auto &p : proto.h265().profiles())
            c.profiles.push_back(convertH265Profile(p));
        fillDr(proto.h265(), c.dynamicRanges);
        result.h265 = std::move(c);
    }
    if (proto.has_vp9())
    {
        firebolt::rialto::common::Vp9CodecCapability c;
        for (const auto &p : proto.vp9().profiles())
            c.profiles.push_back(convertVp9Profile(p));
        fillDr(proto.vp9(), c.dynamicRanges);
        result.vp9 = std::move(c);
    }
    if (proto.has_av1())
    {
        firebolt::rialto::common::Av1CodecCapability c;
        for (const auto &p : proto.av1().profiles())
            c.profiles.push_back(convertAv1Profile(p));
        fillDr(proto.av1(), c.dynamicRanges);
        result.av1 = std::move(c);
    }
    return result;
}

firebolt::rialto::common::VideoDecoderCapability
convertVideoDecoderCapability(const VideoCapabilitiesResponse::VideoDecoderCapability &proto)
{
    firebolt::rialto::common::VideoDecoderCapability result;
    if (proto.has_codec_capabilities())
        result.codecCapabilities = convertVideoCodecCapabilities(proto.codec_capabilities());
    return result;
}

} // namespace

firebolt::rialto::common::AudioDecoderCapabilities
convertAudioDecoderCapabilities(const firebolt::rialto::GetSupportedAudioCapabilitiesResponse &response)
{
    firebolt::rialto::common::AudioDecoderCapabilities result;
    result.interfaceVersion = response.interface_version();
    result.schemaVersion = response.schema_version();
    for (const auto &cap : response.capabilities())
    {
        result.capabilities.push_back(convertAudioDecoderCapability(cap));
    }
    return result;
}

firebolt::rialto::common::VideoDecoderCapabilities
convertVideoDecoderCapabilities(const firebolt::rialto::GetSupportedVideoCapabilitiesResponse &response)
{
    firebolt::rialto::common::VideoDecoderCapabilities result;
    result.interfaceVersion = response.interface_version();
    result.schemaVersion = response.schema_version();
    for (const auto &cap : response.capabilities())
    {
        result.capabilities.push_back(convertVideoDecoderCapability(cap));
    }
    return result;
}

} // namespace firebolt::rialto::client
