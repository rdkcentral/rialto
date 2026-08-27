/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#include "CapabilityDeserialiser.h"

namespace
{
using AudioCap = rialto::AudioCapabilities;
using VideoCap = rialto::VideoCapabilities;

firebolt::rialto::common::AudioProfileCapability toAudioProfileCapability(const AudioCap::AudioProfileCapability &src)
{
    firebolt::rialto::common::AudioProfileCapability dst{};
    dst.maxBitrateInBps = src.max_bitrate_in_bps();
    dst.maxChannels = src.max_channels();
    dst.maxSampleRateInHz = src.max_sample_rate_in_hz();
    dst.maxBitDepth = src.max_bit_depth();
    return dst;
}

firebolt::rialto::common::AacProfile fromProto(AudioCap::AacProfile p)
{
    switch (p)
    {
    case AudioCap::AAC_PROFILE_HE_V1:
        return firebolt::rialto::common::AacProfile::HE_V1;
    case AudioCap::AAC_PROFILE_HE_V2:
        return firebolt::rialto::common::AacProfile::HE_V2;
    case AudioCap::AAC_PROFILE_ELD:
        return firebolt::rialto::common::AacProfile::ELD;
    case AudioCap::AAC_PROFILE_X_HE:
        return firebolt::rialto::common::AacProfile::X_HE;
    default:
        return firebolt::rialto::common::AacProfile::LC;
    }
}
firebolt::rialto::common::DolbyEac3Profile fromProto(AudioCap::DolbyEac3Profile p)
{
    return (p == AudioCap::DOLBY_EAC3_PROFILE_PLUS_JOC) ? firebolt::rialto::common::DolbyEac3Profile::PLUS_JOC
                                                        : firebolt::rialto::common::DolbyEac3Profile::PLUS;
}
firebolt::rialto::common::MpegAudioProfile fromProto(AudioCap::MpegAudioProfile p)
{
    return (p == AudioCap::MPEG_AUDIO_PROFILE_LAYER_2) ? firebolt::rialto::common::MpegAudioProfile::LAYER_2
                                                       : firebolt::rialto::common::MpegAudioProfile::LAYER_1;
}
firebolt::rialto::common::RealAudioProfile fromProto(AudioCap::RealAudioProfile p)
{
    return (p == AudioCap::REALAUDIO_PROFILE_RA10) ? firebolt::rialto::common::RealAudioProfile::RA10
                                                   : firebolt::rialto::common::RealAudioProfile::RA8;
}
firebolt::rialto::common::UsacProfile fromProto(AudioCap::UsacProfile p)
{
    return (p == AudioCap::USAC_PROFILE_EXTENDED_HE_AAC) ? firebolt::rialto::common::UsacProfile::EXTENDED_HE_AAC
                                                         : firebolt::rialto::common::UsacProfile::BASELINE;
}
firebolt::rialto::common::DtsProfile fromProto(AudioCap::DtsProfile p)
{
    switch (p)
    {
    case AudioCap::DTS_PROFILE_HD_HRA:
        return firebolt::rialto::common::DtsProfile::HD_HRA;
    case AudioCap::DTS_PROFILE_HD_MA:
        return firebolt::rialto::common::DtsProfile::HD_MA;
    default:
        return firebolt::rialto::common::DtsProfile::CORE;
    }
}
firebolt::rialto::common::AvsProfile fromProto(AudioCap::AvsProfile p)
{
    switch (p)
    {
    case AudioCap::AVS_PROFILE_AVS2:
        return firebolt::rialto::common::AvsProfile::AVS2;
    case AudioCap::AVS_PROFILE_AVS3:
        return firebolt::rialto::common::AvsProfile::AVS3;
    default:
        return firebolt::rialto::common::AvsProfile::AVS1_PART2;
    }
}

template <typename ProtoRepeated, typename Map, typename Conv>
void fillProfileMap(const ProtoRepeated &src, Map &dst, Conv conv)
{
    for (const auto &entry : src)
    {
        if (entry.has_profile() && entry.has_capability())
            dst[conv(entry.profile())] = toAudioProfileCapability(entry.capability());
    }
}

firebolt::rialto::common::AudioDecoderCapability toAudioDecoderCapability(const AudioCap::AudioDecoderCapability &src)
{
    firebolt::rialto::common::AudioDecoderCapability dst;
    if (src.has_pcm())
        dst.pcm = firebolt::rialto::common::PcmCapability{toAudioProfileCapability(src.pcm().base())};
    if (src.has_mp3())
        dst.mp3 = firebolt::rialto::common::Mp3Capability{toAudioProfileCapability(src.mp3().base())};
    if (src.has_alac())
        dst.alac = firebolt::rialto::common::AlacCapability{toAudioProfileCapability(src.alac().base())};
    if (src.has_sbc())
        dst.sbc = firebolt::rialto::common::SbcCapability{toAudioProfileCapability(src.sbc().base())};
    if (src.has_dolby_ac4())
        dst.dolbyAc4 = firebolt::rialto::common::DolbyAc4Capability{toAudioProfileCapability(src.dolby_ac4().base())};
    if (src.has_dolby_truehd())
        dst.dolbyTruehd =
            firebolt::rialto::common::DolbyTruehdCapability{toAudioProfileCapability(src.dolby_truehd().base())};
    if (src.has_flac())
        dst.flac = firebolt::rialto::common::FlacCapability{toAudioProfileCapability(src.flac().base())};
    if (src.has_vorbis())
        dst.vorbis = firebolt::rialto::common::VorbisCapability{toAudioProfileCapability(src.vorbis().base())};
    if (src.has_opus())
        dst.opus = firebolt::rialto::common::OpusCapability{toAudioProfileCapability(src.opus().base())};
    if (src.has_dolby_ac3())
    {
        firebolt::rialto::common::DolbyAc3Capability cap;
        for (const auto &entry : src.dolby_ac3().profiles())
            if (entry.has_capability())
                cap.profiles[firebolt::rialto::common::DolbyAc3Profile::STANDARD] =
                    toAudioProfileCapability(entry.capability());
        dst.dolbyAc3 = cap;
    }
    if (src.has_aac())
    {
        firebolt::rialto::common::AacCapability cap;
        fillProfileMap(src.aac().profiles(), cap.profiles, [](auto p) { return fromProto(p); });
        dst.aac = cap;
    }
    if (src.has_mpeg_audio())
    {
        firebolt::rialto::common::MpegAudioCapability cap;
        fillProfileMap(src.mpeg_audio().profiles(), cap.profiles, [](auto p) { return fromProto(p); });
        dst.mpegAudio = cap;
    }
    if (src.has_dolby_eac3())
    {
        firebolt::rialto::common::DolbyEac3Capability cap;
        fillProfileMap(src.dolby_eac3().profiles(), cap.profiles, [](auto p) { return fromProto(p); });
        dst.dolbyEac3 = cap;
    }
    if (src.has_real_audio())
    {
        firebolt::rialto::common::RealAudioCapability cap;
        fillProfileMap(src.real_audio().profiles(), cap.profiles, [](auto p) { return fromProto(p); });
        dst.realAudio = cap;
    }
    if (src.has_usac())
    {
        firebolt::rialto::common::UsacCapability cap;
        fillProfileMap(src.usac().profiles(), cap.profiles, [](auto p) { return fromProto(p); });
        dst.usac = cap;
    }
    if (src.has_dts())
    {
        firebolt::rialto::common::DtsCapability cap;
        fillProfileMap(src.dts().profiles(), cap.profiles, [](auto p) { return fromProto(p); });
        dst.dts = cap;
    }
    if (src.has_avs())
    {
        firebolt::rialto::common::AvsCapability cap;
        fillProfileMap(src.avs().profiles(), cap.profiles, [](auto p) { return fromProto(p); });
        dst.avs = cap;
    }
    return dst;
}

firebolt::rialto::common::DynamicRange fromDR(VideoCap::DynamicRange dr)
{
    switch (dr)
    {
    case VideoCap::DYNAMIC_RANGE_HLG:
        return firebolt::rialto::common::DynamicRange::HLG;
    case VideoCap::DYNAMIC_RANGE_HDR10:
        return firebolt::rialto::common::DynamicRange::HDR10;
    case VideoCap::DYNAMIC_RANGE_HDR10PLUS:
        return firebolt::rialto::common::DynamicRange::HDR10PLUS;
    case VideoCap::DYNAMIC_RANGE_DOLBY_VISION:
        return firebolt::rialto::common::DynamicRange::DOLBY_VISION;
    default:
        return firebolt::rialto::common::DynamicRange::SDR;
    }
}

firebolt::rialto::common::Mpeg2ProfileType fromProto(VideoCap::Mpeg2ProfileType t)
{
    return (t == VideoCap::MPEG2_PROFILE_SIMPLE) ? firebolt::rialto::common::Mpeg2ProfileType::MPEG2_SIMPLE
                                                 : firebolt::rialto::common::Mpeg2ProfileType::MPEG2_MAIN;
}
firebolt::rialto::common::Mpeg2Level fromProto(VideoCap::Mpeg2Level l)
{
    switch (l)
    {
    case VideoCap::MPEG2_LEVEL_MAIN:
        return firebolt::rialto::common::Mpeg2Level::MPEG2_LEVEL_MAIN;
    case VideoCap::MPEG2_LEVEL_HIGH:
        return firebolt::rialto::common::Mpeg2Level::MPEG2_LEVEL_HIGH;
    default:
        return firebolt::rialto::common::Mpeg2Level::MPEG2_LEVEL_LOW;
    }
}
firebolt::rialto::common::H264ProfileType fromProto(VideoCap::H264ProfileType t)
{
    switch (t)
    {
    case VideoCap::H264_PROFILE_MAIN:
        return firebolt::rialto::common::H264ProfileType::H264_MAIN;
    case VideoCap::H264_PROFILE_HIGH:
        return firebolt::rialto::common::H264ProfileType::H264_HIGH;
    default:
        return firebolt::rialto::common::H264ProfileType::H264_BASELINE;
    }
}
firebolt::rialto::common::H264Level fromProto(VideoCap::H264Level l)
{
    switch (l)
    {
    case VideoCap::H264_LEVEL_3_1:
        return firebolt::rialto::common::H264Level::H264_LEVEL_3_1;
    case VideoCap::H264_LEVEL_4:
        return firebolt::rialto::common::H264Level::H264_LEVEL_4;
    case VideoCap::H264_LEVEL_4_1:
        return firebolt::rialto::common::H264Level::H264_LEVEL_4_1;
    case VideoCap::H264_LEVEL_5:
        return firebolt::rialto::common::H264Level::H264_LEVEL_5;
    case VideoCap::H264_LEVEL_5_1:
        return firebolt::rialto::common::H264Level::H264_LEVEL_5_1;
    case VideoCap::H264_LEVEL_5_2:
        return firebolt::rialto::common::H264Level::H264_LEVEL_5_2;
    default:
        return firebolt::rialto::common::H264Level::H264_LEVEL_3;
    }
}
firebolt::rialto::common::H265ProfileType fromProto(VideoCap::H265ProfileType t)
{
    switch (t)
    {
    case VideoCap::H265_PROFILE_MAIN_10:
        return firebolt::rialto::common::H265ProfileType::H265_MAIN_10;
    case VideoCap::H265_PROFILE_MAIN_10_HDR10:
        return firebolt::rialto::common::H265ProfileType::H265_MAIN_10_HDR10;
    default:
        return firebolt::rialto::common::H265ProfileType::H265_MAIN;
    }
}
firebolt::rialto::common::H265Level fromProto(VideoCap::H265Level l)
{
    switch (l)
    {
    case VideoCap::H265_LEVEL_4_1:
        return firebolt::rialto::common::H265Level::H265_LEVEL_4_1;
    case VideoCap::H265_LEVEL_5:
        return firebolt::rialto::common::H265Level::H265_LEVEL_5;
    case VideoCap::H265_LEVEL_5_1:
        return firebolt::rialto::common::H265Level::H265_LEVEL_5_1;
    case VideoCap::H265_LEVEL_5_2:
        return firebolt::rialto::common::H265Level::H265_LEVEL_5_2;
    case VideoCap::H265_LEVEL_6:
        return firebolt::rialto::common::H265Level::H265_LEVEL_6;
    case VideoCap::H265_LEVEL_6_1:
        return firebolt::rialto::common::H265Level::H265_LEVEL_6_1;
    case VideoCap::H265_LEVEL_6_2:
        return firebolt::rialto::common::H265Level::H265_LEVEL_6_2;
    default:
        return firebolt::rialto::common::H265Level::H265_LEVEL_4;
    }
}
firebolt::rialto::common::Vp9ProfileType fromProto(VideoCap::Vp9ProfileType t)
{
    switch (t)
    {
    case VideoCap::VP9_PROFILE_1:
        return firebolt::rialto::common::Vp9ProfileType::VP9_PROFILE_1;
    case VideoCap::VP9_PROFILE_2:
        return firebolt::rialto::common::Vp9ProfileType::VP9_PROFILE_2;
    case VideoCap::VP9_PROFILE_3:
        return firebolt::rialto::common::Vp9ProfileType::VP9_PROFILE_3;
    default:
        return firebolt::rialto::common::Vp9ProfileType::VP9_PROFILE_0;
    }
}
firebolt::rialto::common::Vp9Level fromProto(VideoCap::Vp9Level l)
{
    switch (l)
    {
    case VideoCap::VP9_LEVEL_1_1:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_1_1;
    case VideoCap::VP9_LEVEL_2:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_2;
    case VideoCap::VP9_LEVEL_2_1:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_2_1;
    case VideoCap::VP9_LEVEL_3:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_3;
    case VideoCap::VP9_LEVEL_3_1:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_3_1;
    case VideoCap::VP9_LEVEL_4:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_4;
    case VideoCap::VP9_LEVEL_4_1:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_4_1;
    case VideoCap::VP9_LEVEL_5:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_5;
    case VideoCap::VP9_LEVEL_5_1:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_5_1;
    case VideoCap::VP9_LEVEL_5_2:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_5_2;
    case VideoCap::VP9_LEVEL_6:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_6;
    case VideoCap::VP9_LEVEL_6_1:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_6_1;
    case VideoCap::VP9_LEVEL_6_2:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_6_2;
    default:
        return firebolt::rialto::common::Vp9Level::VP9_LEVEL_1;
    }
}
firebolt::rialto::common::Av1ProfileType fromProto(VideoCap::Av1ProfileType t)
{
    return (t == VideoCap::AV1_PROFILE_HIGH) ? firebolt::rialto::common::Av1ProfileType::AV1_HIGH
                                             : firebolt::rialto::common::Av1ProfileType::AV1_MAIN;
}
firebolt::rialto::common::Av1Level fromProto(VideoCap::Av1Level l)
{
    switch (l)
    {
    case VideoCap::AV1_LEVEL_4_1:
        return firebolt::rialto::common::Av1Level::AV1_LEVEL_4_1;
    case VideoCap::AV1_LEVEL_5_0:
        return firebolt::rialto::common::Av1Level::AV1_LEVEL_5_0;
    case VideoCap::AV1_LEVEL_5_1:
        return firebolt::rialto::common::Av1Level::AV1_LEVEL_5_1;
    case VideoCap::AV1_LEVEL_5_2:
        return firebolt::rialto::common::Av1Level::AV1_LEVEL_5_2;
    case VideoCap::AV1_LEVEL_6_0:
        return firebolt::rialto::common::Av1Level::AV1_LEVEL_6_0;
    case VideoCap::AV1_LEVEL_6_1:
        return firebolt::rialto::common::Av1Level::AV1_LEVEL_6_1;
    case VideoCap::AV1_LEVEL_6_2:
        return firebolt::rialto::common::Av1Level::AV1_LEVEL_6_2;
    default:
        return firebolt::rialto::common::Av1Level::AV1_LEVEL_4_0;
    }
}

template <typename CommonProfile, typename ProtoProfile, typename TypeConv, typename LevelConv>
CommonProfile toVideoProfile(const ProtoProfile &src, TypeConv typeConv, LevelConv levelConv)
{
    CommonProfile dst{};
    dst.type = typeConv(src.type());
    dst.maxLevel = levelConv(src.max_level());
    dst.maxBitrateInBps = src.max_bitrate_in_bps();
    return dst;
}

template <typename CommonCodecCapability, typename ProtoCodecCapability, typename TypeConv, typename LevelConv>
CommonCodecCapability toVideoCodecCapability(const ProtoCodecCapability &src, TypeConv typeConv, LevelConv levelConv)
{
    using CommonProfile = typename decltype(CommonCodecCapability{}.profiles)::value_type;
    CommonCodecCapability cap;
    for (const auto &p : src.profiles())
        cap.profiles.push_back(toVideoProfile<CommonProfile>(p, typeConv, levelConv));
    for (const auto &dr : src.dynamic_ranges())
        cap.dynamicRanges.push_back(fromDR(static_cast<VideoCap::DynamicRange>(dr)));
    return cap;
}

firebolt::rialto::common::VideoDecoderCapability toVideoDecoderCapability(const VideoCap::VideoDecoderCapability &src)
{
    firebolt::rialto::common::VideoDecoderCapability dst;
    if (!src.has_codec_capabilities())
        return dst;
    const auto &cc = src.codec_capabilities();
    if (cc.has_mpeg2())
        dst.codecCapabilities.mpeg2 = toVideoCodecCapability<firebolt::rialto::common::Mpeg2CodecCapability>(
            cc.mpeg2(), [](auto t) { return fromProto(t); }, [](auto l) { return fromProto(l); });
    if (cc.has_h264())
        dst.codecCapabilities.h264 = toVideoCodecCapability<firebolt::rialto::common::H264CodecCapability>(
            cc.h264(), [](auto t) { return fromProto(t); }, [](auto l) { return fromProto(l); });
    if (cc.has_h265())
        dst.codecCapabilities.h265 = toVideoCodecCapability<firebolt::rialto::common::H265CodecCapability>(
            cc.h265(), [](auto t) { return fromProto(t); }, [](auto l) { return fromProto(l); });
    if (cc.has_vp9())
        dst.codecCapabilities.vp9 = toVideoCodecCapability<firebolt::rialto::common::Vp9CodecCapability>(
            cc.vp9(), [](auto t) { return fromProto(t); }, [](auto l) { return fromProto(l); });
    if (cc.has_av1())
        dst.codecCapabilities.av1 = toVideoCodecCapability<firebolt::rialto::common::Av1CodecCapability>(
            cc.av1(), [](auto t) { return fromProto(t); }, [](auto l) { return fromProto(l); });
    return dst;
}
} // namespace

namespace firebolt::rialto::server::ipc
{
firebolt::rialto::common::AudioDecoderCapabilities deserialiseAudioCapabilities(const ::rialto::AudioCapabilities &src)
{
    firebolt::rialto::common::AudioDecoderCapabilities result;
    result.interfaceVersion = src.interface_version();
    result.schemaVersion = src.schema_version();
    for (const auto &cap : src.capabilities())
        result.capabilities.push_back(toAudioDecoderCapability(cap));
    return result;
}

firebolt::rialto::common::VideoDecoderCapabilities deserialiseVideoCapabilities(const ::rialto::VideoCapabilities &src)
{
    firebolt::rialto::common::VideoDecoderCapabilities result;
    result.interfaceVersion = src.interface_version();
    result.schemaVersion = src.schema_version();
    for (const auto &cap : src.capabilities())
        result.capabilities.push_back(toVideoDecoderCapability(cap));
    return result;
}
} // namespace firebolt::rialto::server::ipc
