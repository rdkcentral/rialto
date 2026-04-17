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

#include "MediaPipelineCapabilitiesModuleService.h"
#include "RialtoCommonModule.h"
#include "RialtoServerLogging.h"
#include <IIpcController.h>

namespace
{
using AudioCapabilitiesResponse = firebolt::rialto::GetSupportedAudioCapabilitiesResponse;
using VideoCapabilitiesResponse = firebolt::rialto::GetSupportedVideoCapabilitiesResponse;

AudioCapabilitiesResponse::AacProfile convertAacProfile(firebolt::rialto::AacProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::AacProfile::LC:
        return AudioCapabilitiesResponse::AAC_PROFILE_LC;
    case firebolt::rialto::AacProfile::HE_V1:
        return AudioCapabilitiesResponse::AAC_PROFILE_HE_V1;
    case firebolt::rialto::AacProfile::HE_V2:
        return AudioCapabilitiesResponse::AAC_PROFILE_HE_V2;
    case firebolt::rialto::AacProfile::ELD:
        return AudioCapabilitiesResponse::AAC_PROFILE_ELD;
    case firebolt::rialto::AacProfile::X_HE:
        return AudioCapabilitiesResponse::AAC_PROFILE_X_HE;
    }
    return AudioCapabilitiesResponse::AAC_PROFILE_LC;
}

AudioCapabilitiesResponse::DolbyAc3Profile convertDolbyAc3Profile(firebolt::rialto::DolbyAc3Profile profile)
{
    switch (profile)
    {
    case firebolt::rialto::DolbyAc3Profile::STANDARD:
        return AudioCapabilitiesResponse::DOLBY_AC3_PROFILE_STANDARD;
    case firebolt::rialto::DolbyAc3Profile::PLUS:
        return AudioCapabilitiesResponse::DOLBY_AC3_PROFILE_PLUS;
    case firebolt::rialto::DolbyAc3Profile::PLUS_JOC:
        return AudioCapabilitiesResponse::DOLBY_AC3_PROFILE_PLUS_JOC;
    }
    return AudioCapabilitiesResponse::DOLBY_AC3_PROFILE_STANDARD;
}

AudioCapabilitiesResponse::DolbyMatProfile convertDolbyMatProfile(firebolt::rialto::DolbyMatProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::DolbyMatProfile::V1:
        return AudioCapabilitiesResponse::DOLBY_MAT_PROFILE_V1;
    case firebolt::rialto::DolbyMatProfile::V2:
        return AudioCapabilitiesResponse::DOLBY_MAT_PROFILE_V2;
    }
    return AudioCapabilitiesResponse::DOLBY_MAT_PROFILE_V1;
}

AudioCapabilitiesResponse::WmaProfile convertWmaProfile(firebolt::rialto::WmaProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::WmaProfile::STANDARD:
        return AudioCapabilitiesResponse::WMA_PROFILE_STANDARD;
    case firebolt::rialto::WmaProfile::PRO:
        return AudioCapabilitiesResponse::WMA_PROFILE_PRO;
    case firebolt::rialto::WmaProfile::LOSSLESS:
        return AudioCapabilitiesResponse::WMA_PROFILE_LOSSLESS;
    }
    return AudioCapabilitiesResponse::WMA_PROFILE_STANDARD;
}

AudioCapabilitiesResponse::RealAudioProfile convertRealAudioProfile(firebolt::rialto::RealAudioProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::RealAudioProfile::RA8:
        return AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA8;
    case firebolt::rialto::RealAudioProfile::RA10:
        return AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA10;
    }
    return AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA8;
}

AudioCapabilitiesResponse::UsacProfile convertUsacProfile(firebolt::rialto::UsacProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::UsacProfile::BASELINE:
        return AudioCapabilitiesResponse::USAC_PROFILE_BASELINE;
    case firebolt::rialto::UsacProfile::EXTENDED_HE_AAC:
        return AudioCapabilitiesResponse::USAC_PROFILE_EXTENDED_HE_AAC;
    }
    return AudioCapabilitiesResponse::USAC_PROFILE_BASELINE;
}

AudioCapabilitiesResponse::DtsProfile convertDtsProfile(firebolt::rialto::DtsProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::DtsProfile::CORE:
        return AudioCapabilitiesResponse::DTS_PROFILE_CORE;
    case firebolt::rialto::DtsProfile::HD_HRA:
        return AudioCapabilitiesResponse::DTS_PROFILE_HD_HRA;
    case firebolt::rialto::DtsProfile::HD_MA:
        return AudioCapabilitiesResponse::DTS_PROFILE_HD_MA;
    }
    return AudioCapabilitiesResponse::DTS_PROFILE_CORE;
}

AudioCapabilitiesResponse::AvsProfile convertAvsProfile(firebolt::rialto::AvsProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::AvsProfile::AVS1_PART2:
        return AudioCapabilitiesResponse::AVS_PROFILE_AVS1_PART2;
    case firebolt::rialto::AvsProfile::AVS2:
        return AudioCapabilitiesResponse::AVS_PROFILE_AVS2;
    case firebolt::rialto::AvsProfile::AVS3:
        return AudioCapabilitiesResponse::AVS_PROFILE_AVS3;
    }
    return AudioCapabilitiesResponse::AVS_PROFILE_AVS1_PART2;
}

void convertAudioDecoderCapability(const firebolt::rialto::AudioDecoderCapability &src,
                                   AudioCapabilitiesResponse::AudioDecoderCapability *dst)
{
    if (src.pcm)
    {
        auto *pcm = dst->mutable_pcm();
        pcm->set_max_bitrate_in_bps(src.pcm->maxBitrateInBps);
        pcm->set_max_channels(src.pcm->maxChannels);
        pcm->set_max_sample_rate_in_hz(src.pcm->maxSampleRateInHz);
        if (src.pcm->maxBitDepth)
            pcm->set_max_bit_depth(*src.pcm->maxBitDepth);
    }
    if (src.aac)
    {
        auto *aac = dst->mutable_aac();
        for (const auto &p : src.aac->profiles)
            aac->add_profiles(convertAacProfile(p));
        aac->set_max_bitrate_in_bps(src.aac->maxBitrateInBps);
        aac->set_max_channels(src.aac->maxChannels);
        aac->set_max_sample_rate_in_hz(src.aac->maxSampleRateInHz);
        if (src.aac->maxBitDepth)
            aac->set_max_bit_depth(*src.aac->maxBitDepth);
    }
    if (src.mpegAudio)
    {
        auto *mpegAudio = dst->mutable_mpeg_audio();
        mpegAudio->set_max_bitrate_in_bps(src.mpegAudio->maxBitrateInBps);
        mpegAudio->set_max_channels(src.mpegAudio->maxChannels);
        mpegAudio->set_max_sample_rate_in_hz(src.mpegAudio->maxSampleRateInHz);
        if (src.mpegAudio->maxBitDepth)
            mpegAudio->set_max_bit_depth(*src.mpegAudio->maxBitDepth);
    }
    if (src.mp3)
    {
        auto *mp3 = dst->mutable_mp3();
        mp3->set_max_bitrate_in_bps(src.mp3->maxBitrateInBps);
        mp3->set_max_channels(src.mp3->maxChannels);
        mp3->set_max_sample_rate_in_hz(src.mp3->maxSampleRateInHz);
        if (src.mp3->maxBitDepth)
            mp3->set_max_bit_depth(*src.mp3->maxBitDepth);
    }
    if (src.alac)
    {
        auto *alac = dst->mutable_alac();
        alac->set_max_bitrate_in_bps(src.alac->maxBitrateInBps);
        alac->set_max_channels(src.alac->maxChannels);
        alac->set_max_sample_rate_in_hz(src.alac->maxSampleRateInHz);
        if (src.alac->maxBitDepth)
            alac->set_max_bit_depth(*src.alac->maxBitDepth);
    }
    if (src.sbc)
    {
        auto *sbc = dst->mutable_sbc();
        sbc->set_max_bitrate_in_bps(src.sbc->maxBitrateInBps);
        sbc->set_max_channels(src.sbc->maxChannels);
        sbc->set_max_sample_rate_in_hz(src.sbc->maxSampleRateInHz);
        if (src.sbc->maxBitDepth)
            sbc->set_max_bit_depth(*src.sbc->maxBitDepth);
    }
    if (src.dolbyAc3)
    {
        auto *dolbyAc3 = dst->mutable_dolby_ac3();
        for (const auto &p : src.dolbyAc3->profiles)
            dolbyAc3->add_profiles(convertDolbyAc3Profile(p));
        dolbyAc3->set_max_bitrate_in_bps(src.dolbyAc3->maxBitrateInBps);
        dolbyAc3->set_max_channels(src.dolbyAc3->maxChannels);
        dolbyAc3->set_max_sample_rate_in_hz(src.dolbyAc3->maxSampleRateInHz);
        if (src.dolbyAc3->maxBitDepth)
            dolbyAc3->set_max_bit_depth(*src.dolbyAc3->maxBitDepth);
    }
    if (src.dolbyAc4)
    {
        auto *dolbyAc4 = dst->mutable_dolby_ac4();
        dolbyAc4->set_max_bitrate_in_bps(src.dolbyAc4->maxBitrateInBps);
        dolbyAc4->set_max_channels(src.dolbyAc4->maxChannels);
        dolbyAc4->set_max_sample_rate_in_hz(src.dolbyAc4->maxSampleRateInHz);
        if (src.dolbyAc4->maxBitDepth)
            dolbyAc4->set_max_bit_depth(*src.dolbyAc4->maxBitDepth);
    }
    if (src.dolbyMat)
    {
        auto *dolbyMat = dst->mutable_dolby_mat();
        for (const auto &p : src.dolbyMat->profiles)
            dolbyMat->add_profiles(convertDolbyMatProfile(p));
        dolbyMat->set_max_bitrate_in_bps(src.dolbyMat->maxBitrateInBps);
        dolbyMat->set_max_channels(src.dolbyMat->maxChannels);
        dolbyMat->set_max_sample_rate_in_hz(src.dolbyMat->maxSampleRateInHz);
        if (src.dolbyMat->maxBitDepth)
            dolbyMat->set_max_bit_depth(*src.dolbyMat->maxBitDepth);
    }
    if (src.dolbyTruehd)
    {
        auto *dolbyTruehd = dst->mutable_dolby_truehd();
        dolbyTruehd->set_max_bitrate_in_bps(src.dolbyTruehd->maxBitrateInBps);
        dolbyTruehd->set_max_channels(src.dolbyTruehd->maxChannels);
        dolbyTruehd->set_max_sample_rate_in_hz(src.dolbyTruehd->maxSampleRateInHz);
        if (src.dolbyTruehd->maxBitDepth)
            dolbyTruehd->set_max_bit_depth(*src.dolbyTruehd->maxBitDepth);
    }
    if (src.flac)
    {
        auto *flac = dst->mutable_flac();
        flac->set_max_bitrate_in_bps(src.flac->maxBitrateInBps);
        flac->set_max_channels(src.flac->maxChannels);
        flac->set_max_sample_rate_in_hz(src.flac->maxSampleRateInHz);
        if (src.flac->maxBitDepth)
            flac->set_max_bit_depth(*src.flac->maxBitDepth);
    }
    if (src.vorbis)
    {
        auto *vorbis = dst->mutable_vorbis();
        vorbis->set_max_bitrate_in_bps(src.vorbis->maxBitrateInBps);
        vorbis->set_max_channels(src.vorbis->maxChannels);
        vorbis->set_max_sample_rate_in_hz(src.vorbis->maxSampleRateInHz);
        if (src.vorbis->maxBitDepth)
            vorbis->set_max_bit_depth(*src.vorbis->maxBitDepth);
    }
    if (src.opus)
    {
        auto *opus = dst->mutable_opus();
        opus->set_max_bitrate_in_bps(src.opus->maxBitrateInBps);
        opus->set_max_channels(src.opus->maxChannels);
        opus->set_max_sample_rate_in_hz(src.opus->maxSampleRateInHz);
        if (src.opus->maxBitDepth)
            opus->set_max_bit_depth(*src.opus->maxBitDepth);
    }
    if (src.wma)
    {
        auto *wma = dst->mutable_wma();
        for (const auto &p : src.wma->profiles)
            wma->add_profiles(convertWmaProfile(p));
        wma->set_max_bitrate_in_bps(src.wma->maxBitrateInBps);
        wma->set_max_channels(src.wma->maxChannels);
        wma->set_max_sample_rate_in_hz(src.wma->maxSampleRateInHz);
        if (src.wma->maxBitDepth)
            wma->set_max_bit_depth(*src.wma->maxBitDepth);
    }
    if (src.realAudio)
    {
        auto *realAudio = dst->mutable_real_audio();
        for (const auto &p : src.realAudio->profiles)
            realAudio->add_profiles(convertRealAudioProfile(p));
        realAudio->set_max_bitrate_in_bps(src.realAudio->maxBitrateInBps);
        realAudio->set_max_channels(src.realAudio->maxChannels);
        realAudio->set_max_sample_rate_in_hz(src.realAudio->maxSampleRateInHz);
        if (src.realAudio->maxBitDepth)
            realAudio->set_max_bit_depth(*src.realAudio->maxBitDepth);
    }
    if (src.usac)
    {
        auto *usac = dst->mutable_usac();
        for (const auto &p : src.usac->profiles)
            usac->add_profiles(convertUsacProfile(p));
        usac->set_max_bitrate_in_bps(src.usac->maxBitrateInBps);
        usac->set_max_channels(src.usac->maxChannels);
        usac->set_max_sample_rate_in_hz(src.usac->maxSampleRateInHz);
        if (src.usac->maxBitDepth)
            usac->set_max_bit_depth(*src.usac->maxBitDepth);
    }
    if (src.dts)
    {
        auto *dts = dst->mutable_dts();
        for (const auto &p : src.dts->profiles)
            dts->add_profiles(convertDtsProfile(p));
        dts->set_max_bitrate_in_bps(src.dts->maxBitrateInBps);
        dts->set_max_channels(src.dts->maxChannels);
        dts->set_max_sample_rate_in_hz(src.dts->maxSampleRateInHz);
        if (src.dts->maxBitDepth)
            dts->set_max_bit_depth(*src.dts->maxBitDepth);
    }
    if (src.avs)
    {
        auto *avs = dst->mutable_avs();
        for (const auto &p : src.avs->profiles)
            avs->add_profiles(convertAvsProfile(p));
        avs->set_max_bitrate_in_bps(src.avs->maxBitrateInBps);
        avs->set_max_channels(src.avs->maxChannels);
        avs->set_max_sample_rate_in_hz(src.avs->maxSampleRateInHz);
        if (src.avs->maxBitDepth)
            avs->set_max_bit_depth(*src.avs->maxBitDepth);
    }
}

void convertAudioDecoderCapabilities(const firebolt::rialto::AudioDecoderCapabilities &src, AudioCapabilitiesResponse *dst)
{
    dst->set_interface_version(src.interfaceVersion);
    dst->set_schema_version(src.schemaVersion);
    for (const auto &cap : src.capabilities)
    {
        convertAudioDecoderCapability(cap, dst->add_capabilities());
    }
}

VideoCapabilitiesResponse::DynamicRange convertDynamicRange(firebolt::rialto::DynamicRange dynamicRange)
{
    switch (dynamicRange)
    {
    case firebolt::rialto::DynamicRange::SDR:
        return VideoCapabilitiesResponse::DYNAMIC_RANGE_SDR;
    case firebolt::rialto::DynamicRange::HLG:
        return VideoCapabilitiesResponse::DYNAMIC_RANGE_HLG;
    case firebolt::rialto::DynamicRange::HDR10:
        return VideoCapabilitiesResponse::DYNAMIC_RANGE_HDR10;
    case firebolt::rialto::DynamicRange::HDR10PLUS:
        return VideoCapabilitiesResponse::DYNAMIC_RANGE_HDR10PLUS;
    case firebolt::rialto::DynamicRange::DOLBY_VISION:
        return VideoCapabilitiesResponse::DYNAMIC_RANGE_DOLBY_VISION;
    }
    return VideoCapabilitiesResponse::DYNAMIC_RANGE_SDR;
}

VideoCapabilitiesResponse::Mpeg2ProfileType convertMpeg2ProfileType(firebolt::rialto::Mpeg2ProfileType type)
{
    switch (type)
    {
    case firebolt::rialto::Mpeg2ProfileType::MPEG2_MAIN:
        return VideoCapabilitiesResponse::MPEG2_PROFILE_MAIN;
    case firebolt::rialto::Mpeg2ProfileType::MPEG2_SIMPLE:
        return VideoCapabilitiesResponse::MPEG2_PROFILE_SIMPLE;
    }
    return VideoCapabilitiesResponse::MPEG2_PROFILE_MAIN;
}

VideoCapabilitiesResponse::Mpeg2Level convertMpeg2Level(firebolt::rialto::Mpeg2Level level)
{
    switch (level)
    {
    case firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_LOW:
        return VideoCapabilitiesResponse::MPEG2_LEVEL_LOW;
    case firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_MAIN:
        return VideoCapabilitiesResponse::MPEG2_LEVEL_MAIN;
    case firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_HIGH:
        return VideoCapabilitiesResponse::MPEG2_LEVEL_HIGH;
    }
    return VideoCapabilitiesResponse::MPEG2_LEVEL_LOW;
}

VideoCapabilitiesResponse::H264ProfileType convertH264ProfileType(firebolt::rialto::H264ProfileType type)
{
    switch (type)
    {
    case firebolt::rialto::H264ProfileType::H264_BASELINE:
        return VideoCapabilitiesResponse::H264_PROFILE_BASELINE;
    case firebolt::rialto::H264ProfileType::H264_MAIN:
        return VideoCapabilitiesResponse::H264_PROFILE_MAIN;
    case firebolt::rialto::H264ProfileType::H264_HIGH:
        return VideoCapabilitiesResponse::H264_PROFILE_HIGH;
    }
    return VideoCapabilitiesResponse::H264_PROFILE_BASELINE;
}

VideoCapabilitiesResponse::H264Level convertH264Level(firebolt::rialto::H264Level level)
{
    switch (level)
    {
    case firebolt::rialto::H264Level::H264_LEVEL_3:
        return VideoCapabilitiesResponse::H264_LEVEL_3;
    case firebolt::rialto::H264Level::H264_LEVEL_3_1:
        return VideoCapabilitiesResponse::H264_LEVEL_3_1;
    case firebolt::rialto::H264Level::H264_LEVEL_4:
        return VideoCapabilitiesResponse::H264_LEVEL_4;
    case firebolt::rialto::H264Level::H264_LEVEL_4_1:
        return VideoCapabilitiesResponse::H264_LEVEL_4_1;
    case firebolt::rialto::H264Level::H264_LEVEL_5:
        return VideoCapabilitiesResponse::H264_LEVEL_5;
    case firebolt::rialto::H264Level::H264_LEVEL_5_1:
        return VideoCapabilitiesResponse::H264_LEVEL_5_1;
    case firebolt::rialto::H264Level::H264_LEVEL_5_2:
        return VideoCapabilitiesResponse::H264_LEVEL_5_2;
    }
    return VideoCapabilitiesResponse::H264_LEVEL_3;
}

VideoCapabilitiesResponse::H265ProfileType convertH265ProfileType(firebolt::rialto::H265ProfileType type)
{
    switch (type)
    {
    case firebolt::rialto::H265ProfileType::H265_MAIN:
        return VideoCapabilitiesResponse::H265_PROFILE_MAIN;
    case firebolt::rialto::H265ProfileType::H265_MAIN_10:
        return VideoCapabilitiesResponse::H265_PROFILE_MAIN_10;
    case firebolt::rialto::H265ProfileType::H265_MAIN_10_HDR10:
        return VideoCapabilitiesResponse::H265_PROFILE_MAIN_10_HDR10;
    }
    return VideoCapabilitiesResponse::H265_PROFILE_MAIN;
}

VideoCapabilitiesResponse::H265Level convertH265Level(firebolt::rialto::H265Level level)
{
    switch (level)
    {
    case firebolt::rialto::H265Level::H265_LEVEL_4:
        return VideoCapabilitiesResponse::H265_LEVEL_4;
    case firebolt::rialto::H265Level::H265_LEVEL_4_1:
        return VideoCapabilitiesResponse::H265_LEVEL_4_1;
    case firebolt::rialto::H265Level::H265_LEVEL_5:
        return VideoCapabilitiesResponse::H265_LEVEL_5;
    case firebolt::rialto::H265Level::H265_LEVEL_5_1:
        return VideoCapabilitiesResponse::H265_LEVEL_5_1;
    case firebolt::rialto::H265Level::H265_LEVEL_5_2:
        return VideoCapabilitiesResponse::H265_LEVEL_5_2;
    case firebolt::rialto::H265Level::H265_LEVEL_6:
        return VideoCapabilitiesResponse::H265_LEVEL_6;
    case firebolt::rialto::H265Level::H265_LEVEL_6_1:
        return VideoCapabilitiesResponse::H265_LEVEL_6_1;
    case firebolt::rialto::H265Level::H265_LEVEL_6_2:
        return VideoCapabilitiesResponse::H265_LEVEL_6_2;
    }
    return VideoCapabilitiesResponse::H265_LEVEL_4;
}

VideoCapabilitiesResponse::Vp9ProfileType convertVp9ProfileType(firebolt::rialto::Vp9ProfileType type)
{
    switch (type)
    {
    case firebolt::rialto::Vp9ProfileType::VP9_PROFILE_0:
        return VideoCapabilitiesResponse::VP9_PROFILE_0;
    case firebolt::rialto::Vp9ProfileType::VP9_PROFILE_1:
        return VideoCapabilitiesResponse::VP9_PROFILE_1;
    case firebolt::rialto::Vp9ProfileType::VP9_PROFILE_2:
        return VideoCapabilitiesResponse::VP9_PROFILE_2;
    case firebolt::rialto::Vp9ProfileType::VP9_PROFILE_3:
        return VideoCapabilitiesResponse::VP9_PROFILE_3;
    }
    return VideoCapabilitiesResponse::VP9_PROFILE_0;
}

VideoCapabilitiesResponse::Vp9Level convertVp9Level(firebolt::rialto::Vp9Level level)
{
    switch (level)
    {
    case firebolt::rialto::Vp9Level::VP9_LEVEL_1:
        return VideoCapabilitiesResponse::VP9_LEVEL_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_1_1:
        return VideoCapabilitiesResponse::VP9_LEVEL_1_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_2:
        return VideoCapabilitiesResponse::VP9_LEVEL_2;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_2_1:
        return VideoCapabilitiesResponse::VP9_LEVEL_2_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_3:
        return VideoCapabilitiesResponse::VP9_LEVEL_3;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_3_1:
        return VideoCapabilitiesResponse::VP9_LEVEL_3_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_4:
        return VideoCapabilitiesResponse::VP9_LEVEL_4;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_4_1:
        return VideoCapabilitiesResponse::VP9_LEVEL_4_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_5:
        return VideoCapabilitiesResponse::VP9_LEVEL_5;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_5_1:
        return VideoCapabilitiesResponse::VP9_LEVEL_5_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_5_2:
        return VideoCapabilitiesResponse::VP9_LEVEL_5_2;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_6:
        return VideoCapabilitiesResponse::VP9_LEVEL_6;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_6_1:
        return VideoCapabilitiesResponse::VP9_LEVEL_6_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_6_2:
        return VideoCapabilitiesResponse::VP9_LEVEL_6_2;
    }
    return VideoCapabilitiesResponse::VP9_LEVEL_1;
}

VideoCapabilitiesResponse::Av1ProfileType convertAv1ProfileType(firebolt::rialto::Av1ProfileType type)
{
    switch (type)
    {
    case firebolt::rialto::Av1ProfileType::AV1_MAIN:
        return VideoCapabilitiesResponse::AV1_PROFILE_MAIN;
    case firebolt::rialto::Av1ProfileType::AV1_HIGH:
        return VideoCapabilitiesResponse::AV1_PROFILE_HIGH;
    }
    return VideoCapabilitiesResponse::AV1_PROFILE_MAIN;
}

VideoCapabilitiesResponse::Av1Level convertAv1Level(firebolt::rialto::Av1Level level)
{
    switch (level)
    {
    case firebolt::rialto::Av1Level::AV1_LEVEL_4_0:
        return VideoCapabilitiesResponse::AV1_LEVEL_4_0;
    case firebolt::rialto::Av1Level::AV1_LEVEL_4_1:
        return VideoCapabilitiesResponse::AV1_LEVEL_4_1;
    case firebolt::rialto::Av1Level::AV1_LEVEL_5_0:
        return VideoCapabilitiesResponse::AV1_LEVEL_5_0;
    case firebolt::rialto::Av1Level::AV1_LEVEL_5_1:
        return VideoCapabilitiesResponse::AV1_LEVEL_5_1;
    case firebolt::rialto::Av1Level::AV1_LEVEL_5_2:
        return VideoCapabilitiesResponse::AV1_LEVEL_5_2;
    case firebolt::rialto::Av1Level::AV1_LEVEL_6_0:
        return VideoCapabilitiesResponse::AV1_LEVEL_6_0;
    case firebolt::rialto::Av1Level::AV1_LEVEL_6_1:
        return VideoCapabilitiesResponse::AV1_LEVEL_6_1;
    case firebolt::rialto::Av1Level::AV1_LEVEL_6_2:
        return VideoCapabilitiesResponse::AV1_LEVEL_6_2;
    }
    return VideoCapabilitiesResponse::AV1_LEVEL_4_0;
}

void convertVideoCodecCapabilities(const firebolt::rialto::VideoCodecCapabilities &src,
                                   VideoCapabilitiesResponse::VideoCodecCapabilities *dst)
{
    for (const auto &p : src.mpeg2Profiles)
    {
        auto *proto = dst->add_mpeg2_profiles();
        proto->set_type(convertMpeg2ProfileType(p.type));
        proto->set_max_level(convertMpeg2Level(p.maxLevel));
        proto->set_max_bitrate_in_bps(p.maxBitrateInBps);
    }
    for (const auto &p : src.h264Profiles)
    {
        auto *proto = dst->add_h264_profiles();
        proto->set_type(convertH264ProfileType(p.type));
        proto->set_max_level(convertH264Level(p.maxLevel));
        proto->set_max_bitrate_in_bps(p.maxBitrateInBps);
    }
    for (const auto &p : src.h265Profiles)
    {
        auto *proto = dst->add_h265_profiles();
        proto->set_type(convertH265ProfileType(p.type));
        proto->set_max_level(convertH265Level(p.maxLevel));
        proto->set_max_bitrate_in_bps(p.maxBitrateInBps);
    }
    for (const auto &p : src.vp9Profiles)
    {
        auto *proto = dst->add_vp9_profiles();
        proto->set_type(convertVp9ProfileType(p.type));
        proto->set_max_level(convertVp9Level(p.maxLevel));
        proto->set_max_bitrate_in_bps(p.maxBitrateInBps);
    }
    for (const auto &p : src.av1Profiles)
    {
        auto *proto = dst->add_av1_profiles();
        proto->set_type(convertAv1ProfileType(p.type));
        proto->set_max_level(convertAv1Level(p.maxLevel));
        proto->set_max_bitrate_in_bps(p.maxBitrateInBps);
    }
}

void convertVideoDecoderCapability(const firebolt::rialto::VideoDecoderCapability &src,
                                   VideoCapabilitiesResponse::VideoDecoderCapability *dst)
{
    convertVideoCodecCapabilities(src.codecCapabilities, dst->mutable_codec_capabilities());
    for (const auto &dr : src.dynamicRanges)
    {
        dst->add_dynamic_ranges(convertDynamicRange(dr));
    }
}

void convertVideoDecoderCapabilities(const firebolt::rialto::VideoDecoderCapabilities &src, VideoCapabilitiesResponse *dst)
{
    dst->set_interface_version(src.interfaceVersion);
    dst->set_schema_version(src.schemaVersion);
    for (const auto &cap : src.capabilities)
    {
        convertVideoDecoderCapability(cap, dst->add_capabilities());
    }
}
} // namespace

namespace firebolt::rialto::server::ipc
{
std::shared_ptr<IMediaPipelineCapabilitiesModuleServiceFactory>
IMediaPipelineCapabilitiesModuleServiceFactory::createFactory()
{
    std::shared_ptr<IMediaPipelineCapabilitiesModuleServiceFactory> factory;

    try
    {
        factory = std::make_shared<MediaPipelineCapabilitiesModuleServiceFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media player capabilities module service factory, reason: %s",
                                e.what());
    }

    return factory;
}

std::shared_ptr<IMediaPipelineCapabilitiesModuleService>
MediaPipelineCapabilitiesModuleServiceFactory::create(service::IMediaPipelineService &mediaPipelineService) const
{
    std::shared_ptr<IMediaPipelineCapabilitiesModuleService> mediaPipelineCapabilitiesModule;

    try
    {
        mediaPipelineCapabilitiesModule = std::make_shared<MediaPipelineCapabilitiesModuleService>(mediaPipelineService);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media player module service, reason: %s", e.what());
    }

    return mediaPipelineCapabilitiesModule;
}

MediaPipelineCapabilitiesModuleService::MediaPipelineCapabilitiesModuleService(
    service::IMediaPipelineService &mediaPipelineService)
    : m_mediaPipelineService{mediaPipelineService}
{
}

MediaPipelineCapabilitiesModuleService::~MediaPipelineCapabilitiesModuleService() {}

void MediaPipelineCapabilitiesModuleService::clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client Connected!");
    ipcClient->exportService(shared_from_this());
}

void MediaPipelineCapabilitiesModuleService::clientDisconnected(
    const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client disconnected!");
}

void MediaPipelineCapabilitiesModuleService::getSupportedMimeTypes(
    ::google::protobuf::RpcController *controller, const ::firebolt::rialto::GetSupportedMimeTypesRequest *request,
    ::firebolt::rialto::GetSupportedMimeTypesResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }

    firebolt::rialto::MediaSourceType sourceType = convertMediaSourceType(request->media_type());
    std::vector<std::string> supportedMimeTypes = m_mediaPipelineService.getSupportedMimeTypes(sourceType);

    for (std::string &mimeType : supportedMimeTypes)
    {
        response->add_mime_types(mimeType);
    }

    done->Run();
}

void MediaPipelineCapabilitiesModuleService::isMimeTypeSupported(
    ::google::protobuf::RpcController *controller, const ::firebolt::rialto::IsMimeTypeSupportedRequest *request,
    ::firebolt::rialto::IsMimeTypeSupportedResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }

    bool isSupported = m_mediaPipelineService.isMimeTypeSupported(request->mime_type());
    response->set_is_supported(isSupported);

    done->Run();
}

void MediaPipelineCapabilitiesModuleService::getSupportedProperties(
    ::google::protobuf::RpcController *controller, const ::firebolt::rialto::GetSupportedPropertiesRequest *request,
    ::firebolt::rialto::GetSupportedPropertiesResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }

    firebolt::rialto::MediaSourceType mediaType = convertMediaSourceType(request->media_type());
    std::vector<std::string> propertiesToSearch{request->property_names().begin(), request->property_names().end()};

    std::vector<std::string> supportedProperties{
        m_mediaPipelineService.getSupportedProperties(mediaType, propertiesToSearch)};

    for (const std::string &property : supportedProperties)
    {
        response->add_supported_properties(property.c_str());
    }

    done->Run();
}

void MediaPipelineCapabilitiesModuleService::isVideoMaster(::google::protobuf::RpcController *controller,
                                                           const ::firebolt::rialto::IsVideoMasterRequest *request,
                                                           ::firebolt::rialto::IsVideoMasterResponse *response,
                                                           ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }

    bool isMaster{false};
    if (!m_mediaPipelineService.isVideoMaster(isMaster))
    {
        RIALTO_SERVER_LOG_ERROR("isVideoMaster check failed");
        controller->SetFailed("isVideoMaster check failed");
        done->Run();
        return;
    }

    response->set_is_video_master(isMaster);

    done->Run();
}

void MediaPipelineCapabilitiesModuleService::getSupportedAudioCapabilities(
    ::google::protobuf::RpcController *controller,
    const ::firebolt::rialto::GetSupportedAudioCapabilitiesRequest *request,
    ::firebolt::rialto::GetSupportedAudioCapabilitiesResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }

    const firebolt::rialto::AudioDecoderCapabilities kAudioCapabilities =
        m_mediaPipelineService.getSupportedAudioCapabilities();

    convertAudioDecoderCapabilities(kAudioCapabilities, response);

    done->Run();
}

void MediaPipelineCapabilitiesModuleService::getSupportedVideoCapabilities(
    ::google::protobuf::RpcController *controller,
    const ::firebolt::rialto::GetSupportedVideoCapabilitiesRequest *request,
    ::firebolt::rialto::GetSupportedVideoCapabilitiesResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }

    const firebolt::rialto::VideoDecoderCapabilities kVideoCapabilities =
        m_mediaPipelineService.getSupportedVideoCapabilities();

    convertVideoDecoderCapabilities(kVideoCapabilities, response);

    done->Run();
}
} // namespace firebolt::rialto::server::ipc
