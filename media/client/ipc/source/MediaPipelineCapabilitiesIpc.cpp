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

firebolt::rialto::AudioProfileCapability convertAudioProfileCapability(
    const AudioCapabilitiesResponse::AudioProfileCapability &proto)
{
    firebolt::rialto::AudioProfileCapability cap{};
    if (proto.has_max_bitrate_in_bps())    cap.maxBitrateInBps   = proto.max_bitrate_in_bps();
    if (proto.has_max_channels())          cap.maxChannels        = proto.max_channels();
    if (proto.has_max_sample_rate_in_hz()) cap.maxSampleRateInHz  = proto.max_sample_rate_in_hz();
    if (proto.has_max_bit_depth())         cap.maxBitDepth        = proto.max_bit_depth();
    return cap;
}

firebolt::rialto::AacProfile convertAacProfile(AudioCapabilitiesResponse::AacProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::AAC_PROFILE_LC:    return firebolt::rialto::AacProfile::LC;
    case AudioCapabilitiesResponse::AAC_PROFILE_HE_V1: return firebolt::rialto::AacProfile::HE_V1;
    case AudioCapabilitiesResponse::AAC_PROFILE_HE_V2: return firebolt::rialto::AacProfile::HE_V2;
    case AudioCapabilitiesResponse::AAC_PROFILE_ELD:   return firebolt::rialto::AacProfile::ELD;
    case AudioCapabilitiesResponse::AAC_PROFILE_X_HE:  return firebolt::rialto::AacProfile::X_HE;
    }
    return firebolt::rialto::AacProfile::LC;
}

firebolt::rialto::DolbyAc3Profile convertDolbyAc3Profile(AudioCapabilitiesResponse::DolbyAc3Profile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::DOLBY_AC3_PROFILE_STANDARD: return firebolt::rialto::DolbyAc3Profile::STANDARD;
    }
    return firebolt::rialto::DolbyAc3Profile::STANDARD;
}

firebolt::rialto::DolbyEac3Profile convertDolbyEac3Profile(AudioCapabilitiesResponse::DolbyEac3Profile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::DOLBY_EAC3_PROFILE_PLUS:     return firebolt::rialto::DolbyEac3Profile::PLUS;
    case AudioCapabilitiesResponse::DOLBY_EAC3_PROFILE_PLUS_JOC: return firebolt::rialto::DolbyEac3Profile::PLUS_JOC;
    }
    return firebolt::rialto::DolbyEac3Profile::PLUS;
}

firebolt::rialto::MpegAudioProfile convertMpegAudioProfile(AudioCapabilitiesResponse::MpegAudioProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::MPEG_AUDIO_PROFILE_LAYER_1: return firebolt::rialto::MpegAudioProfile::LAYER_1;
    case AudioCapabilitiesResponse::MPEG_AUDIO_PROFILE_LAYER_2: return firebolt::rialto::MpegAudioProfile::LAYER_2;
    }
    return firebolt::rialto::MpegAudioProfile::LAYER_1;
}

firebolt::rialto::RealAudioProfile convertRealAudioProfile(AudioCapabilitiesResponse::RealAudioProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA8:  return firebolt::rialto::RealAudioProfile::RA8;
    case AudioCapabilitiesResponse::REALAUDIO_PROFILE_RA10: return firebolt::rialto::RealAudioProfile::RA10;
    }
    return firebolt::rialto::RealAudioProfile::RA8;
}

firebolt::rialto::UsacProfile convertUsacProfile(AudioCapabilitiesResponse::UsacProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::USAC_PROFILE_BASELINE:        return firebolt::rialto::UsacProfile::BASELINE;
    case AudioCapabilitiesResponse::USAC_PROFILE_EXTENDED_HE_AAC: return firebolt::rialto::UsacProfile::EXTENDED_HE_AAC;
    }
    return firebolt::rialto::UsacProfile::BASELINE;
}

firebolt::rialto::DtsProfile convertDtsProfile(AudioCapabilitiesResponse::DtsProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::DTS_PROFILE_CORE:   return firebolt::rialto::DtsProfile::CORE;
    case AudioCapabilitiesResponse::DTS_PROFILE_HD_HRA: return firebolt::rialto::DtsProfile::HD_HRA;
    case AudioCapabilitiesResponse::DTS_PROFILE_HD_MA:  return firebolt::rialto::DtsProfile::HD_MA;
    }
    return firebolt::rialto::DtsProfile::CORE;
}

firebolt::rialto::AvsProfile convertAvsProfile(AudioCapabilitiesResponse::AvsProfile proto)
{
    switch (proto)
    {
    case AudioCapabilitiesResponse::AVS_PROFILE_AVS1_PART2: return firebolt::rialto::AvsProfile::AVS1_PART2;
    case AudioCapabilitiesResponse::AVS_PROFILE_AVS2:       return firebolt::rialto::AvsProfile::AVS2;
    case AudioCapabilitiesResponse::AVS_PROFILE_AVS3:       return firebolt::rialto::AvsProfile::AVS3;
    }
    return firebolt::rialto::AvsProfile::AVS1_PART2;
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

firebolt::rialto::AudioDecoderCapability convertAudioDecoderCapability(
    const AudioCapabilitiesResponse::AudioDecoderCapability &proto)
{
    firebolt::rialto::AudioDecoderCapability result;

    auto fromBase = [](const auto &protoCap) {
        return protoCap.has_base() ? std::optional{convertAudioProfileCapability(protoCap.base())} : std::nullopt;
    };

    if (proto.has_pcm())         result.pcm         = firebolt::rialto::PcmCapability{convertAudioProfileCapability(proto.pcm().base())};
    if (proto.has_mp3())         result.mp3         = firebolt::rialto::Mp3Capability{convertAudioProfileCapability(proto.mp3().base())};
    if (proto.has_alac())        result.alac        = firebolt::rialto::AlacCapability{convertAudioProfileCapability(proto.alac().base())};
    if (proto.has_sbc())         result.sbc         = firebolt::rialto::SbcCapability{convertAudioProfileCapability(proto.sbc().base())};
    if (proto.has_dolby_ac4())   result.dolbyAc4    = firebolt::rialto::DolbyAc4Capability{convertAudioProfileCapability(proto.dolby_ac4().base())};
    if (proto.has_dolby_truehd())result.dolbyTruehd = firebolt::rialto::DolbyTruehdCapability{convertAudioProfileCapability(proto.dolby_truehd().base())};
    if (proto.has_flac())        result.flac        = firebolt::rialto::FlacCapability{convertAudioProfileCapability(proto.flac().base())};
    if (proto.has_vorbis())      result.vorbis      = firebolt::rialto::VorbisCapability{convertAudioProfileCapability(proto.vorbis().base())};
    if (proto.has_opus())        result.opus        = firebolt::rialto::OpusCapability{convertAudioProfileCapability(proto.opus().base())};

    if (proto.has_aac())
        result.aac = firebolt::rialto::AacCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::AacProfile, firebolt::rialto::AudioProfileCapability>>(
                proto.aac(), convertAacProfile)};
    if (proto.has_mpeg_audio())
        result.mpegAudio = firebolt::rialto::MpegAudioCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::MpegAudioProfile, firebolt::rialto::AudioProfileCapability>>(
                proto.mpeg_audio(), convertMpegAudioProfile)};
    if (proto.has_dolby_ac3())
        result.dolbyAc3 = firebolt::rialto::DolbyAc3Capability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::DolbyAc3Profile, firebolt::rialto::AudioProfileCapability>>(
                proto.dolby_ac3(), convertDolbyAc3Profile)};
    if (proto.has_dolby_eac3())
        result.dolbyEac3 = firebolt::rialto::DolbyEac3Capability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::DolbyEac3Profile, firebolt::rialto::AudioProfileCapability>>(
                proto.dolby_eac3(), convertDolbyEac3Profile)};
    if (proto.has_real_audio())
        result.realAudio = firebolt::rialto::RealAudioCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::RealAudioProfile, firebolt::rialto::AudioProfileCapability>>(
                proto.real_audio(), convertRealAudioProfile)};
    if (proto.has_usac())
        result.usac = firebolt::rialto::UsacCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::UsacProfile, firebolt::rialto::AudioProfileCapability>>(
                proto.usac(), convertUsacProfile)};
    if (proto.has_dts())
        result.dts = firebolt::rialto::DtsCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::DtsProfile, firebolt::rialto::AudioProfileCapability>>(
                proto.dts(), convertDtsProfile)};
    if (proto.has_avs())
        result.avs = firebolt::rialto::AvsCapability{
            deserializeNamedProfileMap<std::map<firebolt::rialto::AvsProfile, firebolt::rialto::AudioProfileCapability>>(
                proto.avs(), convertAvsProfile)};

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


firebolt::rialto::VideoCodecCapabilities
convertVideoCodecCapabilities(const VideoCapabilitiesResponse::VideoCodecCapabilities &proto)
{
    firebolt::rialto::VideoCodecCapabilities result;

    auto fillDr = [](const auto &protoCodec, std::vector<firebolt::rialto::DynamicRange> &out) {
        for (int i = 0; i < protoCodec.dynamic_ranges_size(); ++i)
            out.push_back(convertDynamicRange(protoCodec.dynamic_ranges(i)));
    };

    if (proto.has_mpeg2())
    {
        firebolt::rialto::Mpeg2CodecCapability c;
        for (const auto &p : proto.mpeg2().profiles()) c.profiles.push_back(convertMpeg2Profile(p));
        fillDr(proto.mpeg2(), c.dynamicRanges);
        result.mpeg2 = std::move(c);
    }
    if (proto.has_h264())
    {
        firebolt::rialto::H264CodecCapability c;
        for (const auto &p : proto.h264().profiles()) c.profiles.push_back(convertH264Profile(p));
        fillDr(proto.h264(), c.dynamicRanges);
        result.h264 = std::move(c);
    }
    if (proto.has_h265())
    {
        firebolt::rialto::H265CodecCapability c;
        for (const auto &p : proto.h265().profiles()) c.profiles.push_back(convertH265Profile(p));
        fillDr(proto.h265(), c.dynamicRanges);
        result.h265 = std::move(c);
    }
    if (proto.has_vp9())
    {
        firebolt::rialto::Vp9CodecCapability c;
        for (const auto &p : proto.vp9().profiles()) c.profiles.push_back(convertVp9Profile(p));
        fillDr(proto.vp9(), c.dynamicRanges);
        result.vp9 = std::move(c);
    }
    if (proto.has_av1())
    {
        firebolt::rialto::Av1CodecCapability c;
        for (const auto &p : proto.av1().profiles()) c.profiles.push_back(convertAv1Profile(p));
        fillDr(proto.av1(), c.dynamicRanges);
        result.av1 = std::move(c);
    }
    return result;
}

firebolt::rialto::VideoDecoderCapability
convertVideoDecoderCapability(const VideoCapabilitiesResponse::VideoDecoderCapability &proto)
{
    firebolt::rialto::VideoDecoderCapability result;
    if (proto.has_codec_capabilities())
        result.codecCapabilities = convertVideoCodecCapabilities(proto.codec_capabilities());
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
