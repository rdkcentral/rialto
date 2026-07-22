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
    case firebolt::rialto::AacProfile::LC:    return AudioCapabilitiesResponse::AAC_PROFILE_LC;
    case firebolt::rialto::AacProfile::HE_V1: return AudioCapabilitiesResponse::AAC_PROFILE_HE_V1;
    case firebolt::rialto::AacProfile::HE_V2: return AudioCapabilitiesResponse::AAC_PROFILE_HE_V2;
    case firebolt::rialto::AacProfile::ELD:   return AudioCapabilitiesResponse::AAC_PROFILE_ELD;
    case firebolt::rialto::AacProfile::X_HE:  return AudioCapabilitiesResponse::AAC_PROFILE_X_HE;
    }
    return AudioCapabilitiesResponse::AAC_PROFILE_LC;
}

AudioCapabilitiesResponse::DolbyAc3Profile convertDolbyAc3Profile(firebolt::rialto::DolbyAc3Profile profile)
{
    switch (profile)
    {
    case firebolt::rialto::DolbyAc3Profile::STANDARD: return AudioCapabilitiesResponse::DOLBY_AC3_PROFILE_STANDARD;
    }
    return AudioCapabilitiesResponse::DOLBY_AC3_PROFILE_STANDARD;
}

AudioCapabilitiesResponse::DolbyEac3Profile convertDolbyEac3Profile(firebolt::rialto::DolbyEac3Profile profile)
{
    switch (profile)
    {
    case firebolt::rialto::DolbyEac3Profile::PLUS:     return AudioCapabilitiesResponse::DOLBY_EAC3_PROFILE_PLUS;
    case firebolt::rialto::DolbyEac3Profile::PLUS_JOC: return AudioCapabilitiesResponse::DOLBY_EAC3_PROFILE_PLUS_JOC;
    }
    return AudioCapabilitiesResponse::DOLBY_EAC3_PROFILE_PLUS;
}

AudioCapabilitiesResponse::MpegAudioProfile convertMpegAudioProfile(firebolt::rialto::MpegAudioProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::MpegAudioProfile::LAYER_1: return AudioCapabilitiesResponse::MPEG_AUDIO_PROFILE_LAYER_1;
    case firebolt::rialto::MpegAudioProfile::LAYER_2: return AudioCapabilitiesResponse::MPEG_AUDIO_PROFILE_LAYER_2;
    }
    return AudioCapabilitiesResponse::MPEG_AUDIO_PROFILE_LAYER_1;
}

AudioCapabilitiesResponse::RealAudioProfile convertRealAudioProfile(firebolt::rialto::RealAudioProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::RealAudioProfile::RA8:  return AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA8;
    case firebolt::rialto::RealAudioProfile::RA10: return AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA10;
    }
    return AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA8;
}

AudioCapabilitiesResponse::UsacProfile convertUsacProfile(firebolt::rialto::UsacProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::UsacProfile::BASELINE:          return AudioCapabilitiesResponse::USAC_PROFILE_BASELINE;
    case firebolt::rialto::UsacProfile::EXTENDED_HE_AAC:   return AudioCapabilitiesResponse::USAC_PROFILE_EXTENDED_HE_AAC;
    }
    return AudioCapabilitiesResponse::USAC_PROFILE_BASELINE;
}

AudioCapabilitiesResponse::DtsProfile convertDtsProfile(firebolt::rialto::DtsProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::DtsProfile::CORE:   return AudioCapabilitiesResponse::DTS_PROFILE_CORE;
    case firebolt::rialto::DtsProfile::HD_HRA: return AudioCapabilitiesResponse::DTS_PROFILE_HD_HRA;
    case firebolt::rialto::DtsProfile::HD_MA:  return AudioCapabilitiesResponse::DTS_PROFILE_HD_MA;
    }
    return AudioCapabilitiesResponse::DTS_PROFILE_CORE;
}

AudioCapabilitiesResponse::AvsProfile convertAvsProfile(firebolt::rialto::AvsProfile profile)
{
    switch (profile)
    {
    case firebolt::rialto::AvsProfile::AVS1_PART2: return AudioCapabilitiesResponse::AVS_PROFILE_AVS1_PART2;
    case firebolt::rialto::AvsProfile::AVS2:       return AudioCapabilitiesResponse::AVS_PROFILE_AVS2;
    case firebolt::rialto::AvsProfile::AVS3:       return AudioCapabilitiesResponse::AVS_PROFILE_AVS3;
    }
    return AudioCapabilitiesResponse::AVS_PROFILE_AVS1_PART2;
}

void fillAudioProfileCapability(const firebolt::rialto::AudioProfileCapability &src,
                                 AudioCapabilitiesResponse::AudioProfileCapability *dst)
{
    dst->set_max_bitrate_in_bps(src.maxBitrateInBps);
    dst->set_max_channels(src.maxChannels);
    dst->set_max_sample_rate_in_hz(src.maxSampleRateInHz);
    dst->set_max_bit_depth(src.maxBitDepth);
}

template <typename MapType, typename ProtoCapType, typename ProtoProfileConverter>
void serializeNamedProfileMap(const MapType &srcMap, ProtoCapType *dst,
                               ProtoProfileConverter convertProfile)
{
    for (const auto &[profile, cap] : srcMap)
    {
        auto *entry = dst->add_profiles();
        entry->set_profile(convertProfile(profile));
        fillAudioProfileCapability(cap, entry->mutable_capability());
    }
}

void convertAudioDecoderCapability(const firebolt::rialto::AudioDecoderCapability &src,
                                   AudioCapabilitiesResponse::AudioDecoderCapability *dst)
{
    auto fillBase = [](const firebolt::rialto::AudioProfileCapability &cap, auto *proto) {
        fillAudioProfileCapability(cap, proto->mutable_base());
    };

    if (src.pcm)       fillBase(src.pcm->base,       dst->mutable_pcm());
    if (src.mp3)       fillBase(src.mp3->base,       dst->mutable_mp3());
    if (src.alac)      fillBase(src.alac->base,      dst->mutable_alac());
    if (src.sbc)       fillBase(src.sbc->base,       dst->mutable_sbc());
    if (src.dolbyAc4)  fillBase(src.dolbyAc4->base,  dst->mutable_dolby_ac4());
    if (src.dolbyTruehd) fillBase(src.dolbyTruehd->base, dst->mutable_dolby_truehd());
    if (src.flac)      fillBase(src.flac->base,      dst->mutable_flac());
    if (src.vorbis)    fillBase(src.vorbis->base,    dst->mutable_vorbis());
    if (src.opus)      fillBase(src.opus->base,      dst->mutable_opus());

    if (src.aac)       serializeNamedProfileMap(src.aac->profiles,       dst->mutable_aac(),       convertAacProfile);
    if (src.mpegAudio) serializeNamedProfileMap(src.mpegAudio->profiles, dst->mutable_mpeg_audio(), convertMpegAudioProfile);
    if (src.dolbyAc3)  serializeNamedProfileMap(src.dolbyAc3->profiles,  dst->mutable_dolby_ac3(), convertDolbyAc3Profile);
    if (src.dolbyEac3) serializeNamedProfileMap(src.dolbyEac3->profiles, dst->mutable_dolby_eac3(), convertDolbyEac3Profile);
    if (src.realAudio) serializeNamedProfileMap(src.realAudio->profiles, dst->mutable_real_audio(), convertRealAudioProfile);
    if (src.usac)      serializeNamedProfileMap(src.usac->profiles,      dst->mutable_usac(),      convertUsacProfile);
    if (src.dts)       serializeNamedProfileMap(src.dts->profiles,       dst->mutable_dts(),       convertDtsProfile);
    if (src.avs)       serializeNamedProfileMap(src.avs->profiles,       dst->mutable_avs(),       convertAvsProfile);
}

void convertAudioDecoderCapabilities(const firebolt::rialto::AudioDecoderCapabilities &src,
                                     AudioCapabilitiesResponse *dst)
{
    dst->set_interface_version(src.interfaceVersion);
    dst->set_schema_version(src.schemaVersion);
    for (const auto &cap : src.capabilities)
    {
        convertAudioDecoderCapability(cap, dst->add_capabilities());
    }
}


// ============================================================
// Video converter helpers
// ============================================================

VideoCapabilitiesResponse::DynamicRange convertDynamicRange(firebolt::rialto::DynamicRange dr)
{
    switch (dr)
    {
    case firebolt::rialto::DynamicRange::SDR:          return VideoCapabilitiesResponse::DYNAMIC_RANGE_SDR;
    case firebolt::rialto::DynamicRange::HLG:          return VideoCapabilitiesResponse::DYNAMIC_RANGE_HLG;
    case firebolt::rialto::DynamicRange::HDR10:        return VideoCapabilitiesResponse::DYNAMIC_RANGE_HDR10;
    case firebolt::rialto::DynamicRange::HDR10PLUS:    return VideoCapabilitiesResponse::DYNAMIC_RANGE_HDR10PLUS;
    case firebolt::rialto::DynamicRange::DOLBY_VISION: return VideoCapabilitiesResponse::DYNAMIC_RANGE_DOLBY_VISION;
    }
    return VideoCapabilitiesResponse::DYNAMIC_RANGE_SDR;
}

VideoCapabilitiesResponse::Mpeg2ProfileType convertMpeg2ProfileType(firebolt::rialto::Mpeg2ProfileType t)
{
    switch (t)
    {
    case firebolt::rialto::Mpeg2ProfileType::MPEG2_MAIN:   return VideoCapabilitiesResponse::MPEG2_PROFILE_MAIN;
    case firebolt::rialto::Mpeg2ProfileType::MPEG2_SIMPLE: return VideoCapabilitiesResponse::MPEG2_PROFILE_SIMPLE;
    }
    return VideoCapabilitiesResponse::MPEG2_PROFILE_MAIN;
}
VideoCapabilitiesResponse::Mpeg2Level convertMpeg2Level(firebolt::rialto::Mpeg2Level l)
{
    switch (l)
    {
    case firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_LOW:  return VideoCapabilitiesResponse::MPEG2_LEVEL_LOW;
    case firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_MAIN: return VideoCapabilitiesResponse::MPEG2_LEVEL_MAIN;
    case firebolt::rialto::Mpeg2Level::MPEG2_LEVEL_HIGH: return VideoCapabilitiesResponse::MPEG2_LEVEL_HIGH;
    }
    return VideoCapabilitiesResponse::MPEG2_LEVEL_MAIN;
}

VideoCapabilitiesResponse::H264ProfileType convertH264ProfileType(firebolt::rialto::H264ProfileType t)
{
    switch (t)
    {
    case firebolt::rialto::H264ProfileType::H264_BASELINE: return VideoCapabilitiesResponse::H264_PROFILE_BASELINE;
    case firebolt::rialto::H264ProfileType::H264_MAIN:     return VideoCapabilitiesResponse::H264_PROFILE_MAIN;
    case firebolt::rialto::H264ProfileType::H264_HIGH:     return VideoCapabilitiesResponse::H264_PROFILE_HIGH;
    }
    return VideoCapabilitiesResponse::H264_PROFILE_MAIN;
}
VideoCapabilitiesResponse::H264Level convertH264Level(firebolt::rialto::H264Level l)
{
    switch (l)
    {
    case firebolt::rialto::H264Level::H264_LEVEL_3:   return VideoCapabilitiesResponse::H264_LEVEL_3;
    case firebolt::rialto::H264Level::H264_LEVEL_3_1: return VideoCapabilitiesResponse::H264_LEVEL_3_1;
    case firebolt::rialto::H264Level::H264_LEVEL_4:   return VideoCapabilitiesResponse::H264_LEVEL_4;
    case firebolt::rialto::H264Level::H264_LEVEL_4_1: return VideoCapabilitiesResponse::H264_LEVEL_4_1;
    case firebolt::rialto::H264Level::H264_LEVEL_5:   return VideoCapabilitiesResponse::H264_LEVEL_5;
    case firebolt::rialto::H264Level::H264_LEVEL_5_1: return VideoCapabilitiesResponse::H264_LEVEL_5_1;
    case firebolt::rialto::H264Level::H264_LEVEL_5_2: return VideoCapabilitiesResponse::H264_LEVEL_5_2;
    }
    return VideoCapabilitiesResponse::H264_LEVEL_5;
}

VideoCapabilitiesResponse::H265ProfileType convertH265ProfileType(firebolt::rialto::H265ProfileType t)
{
    switch (t)
    {
    case firebolt::rialto::H265ProfileType::H265_MAIN:          return VideoCapabilitiesResponse::H265_PROFILE_MAIN;
    case firebolt::rialto::H265ProfileType::H265_MAIN_10:       return VideoCapabilitiesResponse::H265_PROFILE_MAIN_10;
    case firebolt::rialto::H265ProfileType::H265_MAIN_10_HDR10: return VideoCapabilitiesResponse::H265_PROFILE_MAIN_10_HDR10;
    }
    return VideoCapabilitiesResponse::H265_PROFILE_MAIN;
}
VideoCapabilitiesResponse::H265Level convertH265Level(firebolt::rialto::H265Level l)
{
    switch (l)
    {
    case firebolt::rialto::H265Level::H265_LEVEL_4:   return VideoCapabilitiesResponse::H265_LEVEL_4;
    case firebolt::rialto::H265Level::H265_LEVEL_4_1: return VideoCapabilitiesResponse::H265_LEVEL_4_1;
    case firebolt::rialto::H265Level::H265_LEVEL_5:   return VideoCapabilitiesResponse::H265_LEVEL_5;
    case firebolt::rialto::H265Level::H265_LEVEL_5_1: return VideoCapabilitiesResponse::H265_LEVEL_5_1;
    case firebolt::rialto::H265Level::H265_LEVEL_5_2: return VideoCapabilitiesResponse::H265_LEVEL_5_2;
    case firebolt::rialto::H265Level::H265_LEVEL_6:   return VideoCapabilitiesResponse::H265_LEVEL_6;
    case firebolt::rialto::H265Level::H265_LEVEL_6_1: return VideoCapabilitiesResponse::H265_LEVEL_6_1;
    case firebolt::rialto::H265Level::H265_LEVEL_6_2: return VideoCapabilitiesResponse::H265_LEVEL_6_2;
    }
    return VideoCapabilitiesResponse::H265_LEVEL_5;
}

VideoCapabilitiesResponse::Vp9ProfileType convertVp9ProfileType(firebolt::rialto::Vp9ProfileType t)
{
    switch (t)
    {
    case firebolt::rialto::Vp9ProfileType::VP9_PROFILE_0: return VideoCapabilitiesResponse::VP9_PROFILE_0;
    case firebolt::rialto::Vp9ProfileType::VP9_PROFILE_1: return VideoCapabilitiesResponse::VP9_PROFILE_1;
    case firebolt::rialto::Vp9ProfileType::VP9_PROFILE_2: return VideoCapabilitiesResponse::VP9_PROFILE_2;
    case firebolt::rialto::Vp9ProfileType::VP9_PROFILE_3: return VideoCapabilitiesResponse::VP9_PROFILE_3;
    }
    return VideoCapabilitiesResponse::VP9_PROFILE_0;
}
VideoCapabilitiesResponse::Vp9Level convertVp9Level(firebolt::rialto::Vp9Level l)
{
    switch (l)
    {
    case firebolt::rialto::Vp9Level::VP9_LEVEL_1:   return VideoCapabilitiesResponse::VP9_LEVEL_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_1_1: return VideoCapabilitiesResponse::VP9_LEVEL_1_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_2:   return VideoCapabilitiesResponse::VP9_LEVEL_2;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_2_1: return VideoCapabilitiesResponse::VP9_LEVEL_2_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_3:   return VideoCapabilitiesResponse::VP9_LEVEL_3;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_3_1: return VideoCapabilitiesResponse::VP9_LEVEL_3_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_4:   return VideoCapabilitiesResponse::VP9_LEVEL_4;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_4_1: return VideoCapabilitiesResponse::VP9_LEVEL_4_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_5:   return VideoCapabilitiesResponse::VP9_LEVEL_5;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_5_1: return VideoCapabilitiesResponse::VP9_LEVEL_5_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_5_2: return VideoCapabilitiesResponse::VP9_LEVEL_5_2;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_6:   return VideoCapabilitiesResponse::VP9_LEVEL_6;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_6_1: return VideoCapabilitiesResponse::VP9_LEVEL_6_1;
    case firebolt::rialto::Vp9Level::VP9_LEVEL_6_2: return VideoCapabilitiesResponse::VP9_LEVEL_6_2;
    }
    return VideoCapabilitiesResponse::VP9_LEVEL_5;
}

VideoCapabilitiesResponse::Av1ProfileType convertAv1ProfileType(firebolt::rialto::Av1ProfileType t)
{
    switch (t)
    {
    case firebolt::rialto::Av1ProfileType::AV1_MAIN: return VideoCapabilitiesResponse::AV1_PROFILE_MAIN;
    case firebolt::rialto::Av1ProfileType::AV1_HIGH: return VideoCapabilitiesResponse::AV1_PROFILE_HIGH;
    }
    return VideoCapabilitiesResponse::AV1_PROFILE_MAIN;
}
VideoCapabilitiesResponse::Av1Level convertAv1Level(firebolt::rialto::Av1Level l)
{
    switch (l)
    {
    case firebolt::rialto::Av1Level::AV1_LEVEL_4_0: return VideoCapabilitiesResponse::AV1_LEVEL_4_0;
    case firebolt::rialto::Av1Level::AV1_LEVEL_4_1: return VideoCapabilitiesResponse::AV1_LEVEL_4_1;
    case firebolt::rialto::Av1Level::AV1_LEVEL_5_0: return VideoCapabilitiesResponse::AV1_LEVEL_5_0;
    case firebolt::rialto::Av1Level::AV1_LEVEL_5_1: return VideoCapabilitiesResponse::AV1_LEVEL_5_1;
    case firebolt::rialto::Av1Level::AV1_LEVEL_5_2: return VideoCapabilitiesResponse::AV1_LEVEL_5_2;
    case firebolt::rialto::Av1Level::AV1_LEVEL_6_0: return VideoCapabilitiesResponse::AV1_LEVEL_6_0;
    case firebolt::rialto::Av1Level::AV1_LEVEL_6_1: return VideoCapabilitiesResponse::AV1_LEVEL_6_1;
    case firebolt::rialto::Av1Level::AV1_LEVEL_6_2: return VideoCapabilitiesResponse::AV1_LEVEL_6_2;
    }
    return VideoCapabilitiesResponse::AV1_LEVEL_5_0;
}

void fillCodecProfiles(const std::vector<firebolt::rialto::Mpeg2Profile> &profiles, VideoCapabilitiesResponse::Mpeg2CodecCapability *dst)
{
    for (const auto &p : profiles)
    {
        auto *proto = dst->add_profiles();
        proto->set_type(convertMpeg2ProfileType(p.type));
        proto->set_max_level(convertMpeg2Level(p.maxLevel));
        proto->set_max_bitrate_in_bps(p.maxBitrateInBps);
    }
}
void fillCodecProfiles(const std::vector<firebolt::rialto::H264Profile> &profiles, VideoCapabilitiesResponse::H264CodecCapability *dst)
{
    for (const auto &p : profiles)
    {
        auto *proto = dst->add_profiles();
        proto->set_type(convertH264ProfileType(p.type));
        proto->set_max_level(convertH264Level(p.maxLevel));
        proto->set_max_bitrate_in_bps(p.maxBitrateInBps);
    }
}
void fillCodecProfiles(const std::vector<firebolt::rialto::H265Profile> &profiles, VideoCapabilitiesResponse::H265CodecCapability *dst)
{
    for (const auto &p : profiles)
    {
        auto *proto = dst->add_profiles();
        proto->set_type(convertH265ProfileType(p.type));
        proto->set_max_level(convertH265Level(p.maxLevel));
        proto->set_max_bitrate_in_bps(p.maxBitrateInBps);
    }
}
void fillCodecProfiles(const std::vector<firebolt::rialto::Vp9Profile> &profiles, VideoCapabilitiesResponse::Vp9CodecCapability *dst)
{
    for (const auto &p : profiles)
    {
        auto *proto = dst->add_profiles();
        proto->set_type(convertVp9ProfileType(p.type));
        proto->set_max_level(convertVp9Level(p.maxLevel));
        proto->set_max_bitrate_in_bps(p.maxBitrateInBps);
    }
}
void fillCodecProfiles(const std::vector<firebolt::rialto::Av1Profile> &profiles, VideoCapabilitiesResponse::Av1CodecCapability *dst)
{
    for (const auto &p : profiles)
    {
        auto *proto = dst->add_profiles();
        proto->set_type(convertAv1ProfileType(p.type));
        proto->set_max_level(convertAv1Level(p.maxLevel));
        proto->set_max_bitrate_in_bps(p.maxBitrateInBps);
    }
}

template <typename CodecCap, typename ProtoCap>
void fillPerCodecCapability(const CodecCap &src, ProtoCap *dst)
{
    fillCodecProfiles(src.profiles, dst);
    for (const auto &dr : src.dynamicRanges)
        dst->add_dynamic_ranges(convertDynamicRange(dr));
}


void convertVideoCodecCapabilities(const firebolt::rialto::VideoCodecCapabilities &src,
                                   VideoCapabilitiesResponse::VideoCodecCapabilities *dst)
{
    if (src.mpeg2) fillPerCodecCapability(*src.mpeg2, dst->mutable_mpeg2());
    if (src.h264)  fillPerCodecCapability(*src.h264,  dst->mutable_h264());
    if (src.h265)  fillPerCodecCapability(*src.h265,  dst->mutable_h265());
    if (src.vp9)   fillPerCodecCapability(*src.vp9,   dst->mutable_vp9());
    if (src.av1)   fillPerCodecCapability(*src.av1,   dst->mutable_av1());
}

void convertVideoDecoderCapability(const firebolt::rialto::VideoDecoderCapability &src,
                                   VideoCapabilitiesResponse::VideoDecoderCapability *dst)
{
    convertVideoCodecCapabilities(src.codecCapabilities, dst->mutable_codec_capabilities());
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
