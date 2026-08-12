/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#include "CapabilitySerialiser.h"

namespace
{
using AudioCap = rialto::AudioCapabilities;
using VideoCap = rialto::VideoCapabilities;

void fillProfileCap(const firebolt::rialto::common::AudioProfileCapability &src, AudioCap::AudioProfileCapability *dst)
{
    dst->set_max_bitrate_in_bps(src.maxBitrateInBps);
    dst->set_max_channels(src.maxChannels);
    dst->set_max_sample_rate_in_hz(src.maxSampleRateInHz);
    dst->set_max_bit_depth(src.maxBitDepth);
}

AudioCap::AacProfile toProto(firebolt::rialto::common::AacProfile p)
{
    switch (p)
    {
    case firebolt::rialto::common::AacProfile::HE_V1:     return AudioCap::AAC_PROFILE_HE_V1;
    case firebolt::rialto::common::AacProfile::HE_V2:     return AudioCap::AAC_PROFILE_HE_V2;
    case firebolt::rialto::common::AacProfile::ELD:       return AudioCap::AAC_PROFILE_ELD;
    case firebolt::rialto::common::AacProfile::X_HE:      return AudioCap::AAC_PROFILE_X_HE;
    default:                                       return AudioCap::AAC_PROFILE_LC;
    }
}
AudioCap::DolbyEac3Profile toProto(firebolt::rialto::common::DolbyEac3Profile p)
{
    return (p == firebolt::rialto::common::DolbyEac3Profile::PLUS_JOC) ? AudioCap::DOLBY_EAC3_PROFILE_PLUS_JOC
                                                                : AudioCap::DOLBY_EAC3_PROFILE_PLUS;
}
AudioCap::MpegAudioProfile toProto(firebolt::rialto::common::MpegAudioProfile p)
{
    return (p == firebolt::rialto::common::MpegAudioProfile::LAYER_2) ? AudioCap::MPEG_AUDIO_PROFILE_LAYER_2
                                                                      : AudioCap::MPEG_AUDIO_PROFILE_LAYER_1;
}
AudioCap::RealAudioProfile toProto(firebolt::rialto::common::RealAudioProfile p)
{
    return (p == firebolt::rialto::common::RealAudioProfile::RA10) ? AudioCap::REALAUDIO_PROFILE_RA10
                                                                  : AudioCap::REALAUDIO_PROFILE_RA8;
}
AudioCap::UsacProfile toProto(firebolt::rialto::common::UsacProfile p)
{
    return (p == firebolt::rialto::common::UsacProfile::EXTENDED_HE_AAC) ? AudioCap::USAC_PROFILE_EXTENDED_HE_AAC
                                                                         : AudioCap::USAC_PROFILE_BASELINE;
}
AudioCap::DtsProfile toProto(firebolt::rialto::common::DtsProfile p)
{
    switch (p)
    {
    case firebolt::rialto::common::DtsProfile::HD_HRA: return AudioCap::DTS_PROFILE_HD_HRA;
    case firebolt::rialto::common::DtsProfile::HD_MA:  return AudioCap::DTS_PROFILE_HD_MA;
    default:                                    return AudioCap::DTS_PROFILE_CORE;
    }
}
AudioCap::AvsProfile toProto(firebolt::rialto::common::AvsProfile p)
{
    switch (p)
    {
    case firebolt::rialto::common::AvsProfile::AVS2: return AudioCap::AVS_PROFILE_AVS2;
    case firebolt::rialto::common::AvsProfile::AVS3: return AudioCap::AVS_PROFILE_AVS3;
    default:                                  return AudioCap::AVS_PROFILE_AVS1_PART2;
    }
}

template <typename Map, typename ProtoEntry, typename Conv>
void fillNamedProfiles(const Map &m, ProtoEntry *proto, Conv conv)
{
    for (const auto &[profile, cap] : m)
    {
        auto *e = proto->add_profiles();
        e->set_profile(conv(profile));
        fillProfileCap(cap, e->mutable_capability());
    }
}

void fillAudioDecoderCapability(const firebolt::rialto::common::AudioDecoderCapability &src,
                                AudioCap::AudioDecoderCapability *dst)
{
    auto fillBase = [&](const firebolt::rialto::common::AudioProfileCapability &cap, auto *proto)
    { fillProfileCap(cap, proto->mutable_base()); };

    if (src.pcm)        fillBase(src.pcm->base,        dst->mutable_pcm());
    if (src.mp3)        fillBase(src.mp3->base,        dst->mutable_mp3());
    if (src.alac)       fillBase(src.alac->base,       dst->mutable_alac());
    if (src.sbc)        fillBase(src.sbc->base,        dst->mutable_sbc());
    if (src.dolbyAc4)   fillBase(src.dolbyAc4->base,   dst->mutable_dolby_ac4());
    if (src.dolbyTruehd) fillBase(src.dolbyTruehd->base, dst->mutable_dolby_truehd());
    if (src.flac)       fillBase(src.flac->base,       dst->mutable_flac());
    if (src.vorbis)     fillBase(src.vorbis->base,     dst->mutable_vorbis());
    if (src.opus)       fillBase(src.opus->base,       dst->mutable_opus());
    if (src.dolbyAc3)
    {
        for (const auto &[p, c] : src.dolbyAc3->profiles)
        {
            auto *e = dst->mutable_dolby_ac3()->add_profiles();
            e->set_profile(AudioCap::DOLBY_AC3_PROFILE_STANDARD);
            fillProfileCap(c, e->mutable_capability());
        }
    }
    if (src.aac)       fillNamedProfiles(src.aac->profiles,       dst->mutable_aac(),        [](auto p){ return toProto(p); });
    if (src.mpegAudio) fillNamedProfiles(src.mpegAudio->profiles, dst->mutable_mpeg_audio(), [](auto p){ return toProto(p); });
    if (src.dolbyEac3) fillNamedProfiles(src.dolbyEac3->profiles, dst->mutable_dolby_eac3(), [](auto p){ return toProto(p); });
    if (src.realAudio) fillNamedProfiles(src.realAudio->profiles, dst->mutable_real_audio(), [](auto p){ return toProto(p); });
    if (src.usac)      fillNamedProfiles(src.usac->profiles,      dst->mutable_usac(),       [](auto p){ return toProto(p); });
    if (src.dts)       fillNamedProfiles(src.dts->profiles,       dst->mutable_dts(),        [](auto p){ return toProto(p); });
    if (src.avs)       fillNamedProfiles(src.avs->profiles,       dst->mutable_avs(),        [](auto p){ return toProto(p); });
}

VideoCap::DynamicRange toDR(firebolt::rialto::common::DynamicRange dr)
{
    switch (dr)
    {
    case firebolt::rialto::common::DynamicRange::HLG:          return VideoCap::DYNAMIC_RANGE_HLG;
    case firebolt::rialto::common::DynamicRange::HDR10:        return VideoCap::DYNAMIC_RANGE_HDR10;
    case firebolt::rialto::common::DynamicRange::HDR10PLUS:    return VideoCap::DYNAMIC_RANGE_HDR10PLUS;
    case firebolt::rialto::common::DynamicRange::DOLBY_VISION: return VideoCap::DYNAMIC_RANGE_DOLBY_VISION;
    default:                                            return VideoCap::DYNAMIC_RANGE_SDR;
    }
}
} // namespace

namespace rialto::servermanager::ipc
{
void serialiseAudioCapabilities(const firebolt::rialto::common::AudioDecoderCapabilities &src, rialto::AudioCapabilities *dst)
{
    dst->set_interface_version(src.interfaceVersion);
    dst->set_schema_version(src.schemaVersion);
    for (const auto &cap : src.capabilities)
        fillAudioDecoderCapability(cap, dst->add_capabilities());
}

void serialiseVideoCapabilities(const firebolt::rialto::common::VideoDecoderCapabilities &src, rialto::VideoCapabilities *dst)
{
    dst->set_interface_version(src.interfaceVersion);
    dst->set_schema_version(src.schemaVersion);
    for (const auto &cap : src.capabilities)
    {
        auto *dstCap = dst->add_capabilities();
        auto *cc = dstCap->mutable_codec_capabilities();
        const auto &sc = cap.codecCapabilities;
        if (sc.mpeg2)
        {
            auto *c = cc->mutable_mpeg2();
            for (const auto &p : sc.mpeg2->profiles)
            {
                auto *e = c->add_profiles();
                e->set_type(static_cast<VideoCap::Mpeg2ProfileType>(p.type));
                e->set_max_level(static_cast<VideoCap::Mpeg2Level>(p.maxLevel));
                e->set_max_bitrate_in_bps(p.maxBitrateInBps);
            }
            for (const auto &dr : sc.mpeg2->dynamicRanges) c->add_dynamic_ranges(toDR(dr));
        }
        if (sc.h264)
        {
            auto *c = cc->mutable_h264();
            for (const auto &p : sc.h264->profiles)
            {
                auto *e = c->add_profiles();
                e->set_type(static_cast<VideoCap::H264ProfileType>(p.type));
                e->set_max_level(static_cast<VideoCap::H264Level>(p.maxLevel));
                e->set_max_bitrate_in_bps(p.maxBitrateInBps);
            }
            for (const auto &dr : sc.h264->dynamicRanges) c->add_dynamic_ranges(toDR(dr));
        }
        if (sc.h265)
        {
            auto *c = cc->mutable_h265();
            for (const auto &p : sc.h265->profiles)
            {
                auto *e = c->add_profiles();
                e->set_type(static_cast<VideoCap::H265ProfileType>(p.type));
                e->set_max_level(static_cast<VideoCap::H265Level>(p.maxLevel));
                e->set_max_bitrate_in_bps(p.maxBitrateInBps);
            }
            for (const auto &dr : sc.h265->dynamicRanges) c->add_dynamic_ranges(toDR(dr));
        }
        if (sc.vp9)
        {
            auto *c = cc->mutable_vp9();
            for (const auto &p : sc.vp9->profiles)
            {
                auto *e = c->add_profiles();
                e->set_type(static_cast<VideoCap::Vp9ProfileType>(p.type));
                e->set_max_level(static_cast<VideoCap::Vp9Level>(p.maxLevel));
                e->set_max_bitrate_in_bps(p.maxBitrateInBps);
            }
            for (const auto &dr : sc.vp9->dynamicRanges) c->add_dynamic_ranges(toDR(dr));
        }
        if (sc.av1)
        {
            auto *c = cc->mutable_av1();
            for (const auto &p : sc.av1->profiles)
            {
                auto *e = c->add_profiles();
                e->set_type(static_cast<VideoCap::Av1ProfileType>(p.type));
                e->set_max_level(static_cast<VideoCap::Av1Level>(p.maxLevel));
                e->set_max_bitrate_in_bps(p.maxBitrateInBps);
            }
            for (const auto &dr : sc.av1->dynamicRanges) c->add_dynamic_ranges(toDR(dr));
        }
    }
}

} // namespace rialto::servermanager::ipc
