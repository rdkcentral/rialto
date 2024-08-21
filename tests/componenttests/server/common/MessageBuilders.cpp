/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include <string>

#include "Constants.h"
#include "MessageBuilders.h"
#include "RialtoLogging.h"

using ::google::protobuf::int32;
using ::google::protobuf::uint32;

namespace firebolt::rialto::server::ct
{
::rialto::SetConfigurationRequest createGenericSetConfigurationReq()
{
    constexpr int kLogLevel{kAllLogs};
    constexpr int kMaxPlaybacks{2};
    const std::string kDisplayName{"waylanddisplay"};
    ::rialto::LogLevels logLevels;
    logLevels.set_defaultloglevels(kLogLevel);
    logLevels.set_clientloglevels(kLogLevel);
    logLevels.set_sessionserverloglevels(kLogLevel);
    logLevels.set_ipcloglevels(kLogLevel);
    logLevels.set_servermanagerloglevels(kLogLevel);
    logLevels.set_commonloglevels(kLogLevel);

    ::rialto::SetConfigurationRequest request;
    request.set_sessionmanagementsocketname(kSocketName);
    request.set_clientdisplayname(kDisplayName);
    request.mutable_resources()->set_maxplaybacks(kMaxPlaybacks);
    request.mutable_resources()->set_maxwebaudioplayers(kMaxPlaybacks);
    request.set_socketpermissions(kDefaultPermissions);
    request.set_socketowner(kOwnerName);
    request.set_socketgroup(kOwnerName);
    *(request.mutable_loglevels()) = logLevels;

    return request;
}

::firebolt::rialto::CreateSessionRequest createCreateSessionRequest(int width, int height)
{
    ::firebolt::rialto::CreateSessionRequest request;
    request.set_max_width(width);
    request.set_max_height(height);
    return request;
}

::rialto::SetStateRequest createSetStateRequest(::rialto::SessionServerState value)
{
    ::rialto::SetStateRequest request;
    request.set_sessionserverstate(value);
    return request;
}

::rialto::SetLogLevelsRequest createSetLogLevelsRequest()
{
    ::rialto::SetLogLevelsRequest request;
    auto levels = request.mutable_loglevels();

    levels->set_defaultloglevels(RIALTO_DEBUG_LEVEL_FATAL);
    levels->set_clientloglevels(RIALTO_DEBUG_LEVEL_ERROR);
    levels->set_sessionserverloglevels(RIALTO_DEBUG_LEVEL_WARNING);
    levels->set_ipcloglevels(RIALTO_DEBUG_LEVEL_MILESTONE);
    levels->set_servermanagerloglevels(RIALTO_DEBUG_LEVEL_INFO);
    levels->set_commonloglevels(RIALTO_DEBUG_LEVEL_DEBUG);

    return request;
}

::rialto::PingRequest createPingRequest(int32 id)
{
    ::rialto::PingRequest request;
    request.set_id(id);
    return request;
}

::firebolt::rialto::LoadRequest createLoadRequest(int sessionId)
{
    ::firebolt::rialto::LoadRequest request;
    request.set_session_id(sessionId);
    request.set_type(::firebolt::rialto::LoadRequest_MediaType_MSE);
    request.set_mime_type("mimetype");
    request.set_url("url");
    return request;
}

::firebolt::rialto::AttachSourceRequest createAttachAudioSourceRequest(int sessionId)
{
    ::firebolt::rialto::AttachSourceRequest request;
    request.set_session_id(sessionId);
    request.set_config_type(::firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_AUDIO);
    request.set_mime_type("audio/mp4");
    request.set_has_drm(false);
    request.set_segment_alignment(::firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_NAL);
    request.mutable_audio_config()->set_number_of_channels(kNumOfChannels);
    request.mutable_audio_config()->set_sample_rate(kSampleRate);
    request.set_stream_format(::firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW);
    return request;
}

::firebolt::rialto::AttachSourceRequest createAttachVideoSourceRequest(int sessionId)
{
    ::firebolt::rialto::AttachSourceRequest request;
    request.set_session_id(sessionId);
    request.set_config_type(::firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_VIDEO);
    request.set_mime_type("video/h264");
    request.set_has_drm(false);
    request.set_width(kWidth);
    request.set_height(kHeight);
    request.set_segment_alignment(::firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_NAL);
    request.set_stream_format(::firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW);
    return request;
}

::firebolt::rialto::AllSourcesAttachedRequest createAllSourcesAttachedRequest(int sessionId)
{
    ::firebolt::rialto::AllSourcesAttachedRequest request;
    request.set_session_id(sessionId);
    return request;
}

::firebolt::rialto::HaveDataRequest createHaveDataRequest(int sessionId, unsigned numOfFrames, unsigned requestId)
{
    ::firebolt::rialto::HaveDataRequest request;
    request.set_session_id(sessionId);
    request.set_status(::firebolt::rialto::HaveDataRequest_MediaSourceStatus_OK);
    request.set_num_frames(numOfFrames);
    request.set_request_id(requestId);
    return request;
}

::firebolt::rialto::PauseRequest createPauseRequest(int sessionId)
{
    ::firebolt::rialto::PauseRequest request;
    request.set_session_id(sessionId);
    return request;
}

::firebolt::rialto::PlayRequest createPlayRequest(int sessionId)
{
    ::firebolt::rialto::PlayRequest request;
    request.set_session_id(sessionId);
    return request;
}

::firebolt::rialto::RemoveSourceRequest createRemoveSourceRequest(int sessionId, int sourceId)
{
    ::firebolt::rialto::RemoveSourceRequest request;
    request.set_session_id(sessionId);
    request.set_source_id(sourceId);
    return request;
}

::firebolt::rialto::StopRequest createStopRequest(int sessionId)
{
    ::firebolt::rialto::StopRequest request;
    request.set_session_id(sessionId);
    return request;
}

::firebolt::rialto::DestroySessionRequest createDestroySessionRequest(int sessionId)
{
    ::firebolt::rialto::DestroySessionRequest request;
    request.set_session_id(sessionId);
    return request;
}

::firebolt::rialto::SetPlaybackRateRequest createSetPlaybackRateRequest(int sessionId)
{
    ::firebolt::rialto::SetPlaybackRateRequest request;
    request.set_session_id(sessionId);
    request.set_rate(kPlaybackRate);
    return request;
}

::firebolt::rialto::SetPositionRequest createSetPositionRequest(int sessionId, std::int64_t position)
{
    ::firebolt::rialto::SetPositionRequest request;
    request.set_session_id(sessionId);
    request.set_position(position);
    return request;
}

::firebolt::rialto::GetPositionRequest createGetPositionRequest(int sessionId)
{
    ::firebolt::rialto::GetPositionRequest request;
    request.set_session_id(sessionId);
    return request;
}

::firebolt::rialto::SetImmediateOutputRequest createSetImmediateOutputRequest(int sessionId, int sourceId,
                                                                              bool immediateOutput)
{
    ::firebolt::rialto::SetImmediateOutputRequest request;
    request.set_session_id(sessionId);
    request.set_source_id(sourceId);
    request.set_immediate_output(immediateOutput);
    return request;
}

::firebolt::rialto::GetImmediateOutputRequest createGetImmediateOutputRequest(int sessionId, int sourceId)
{
    ::firebolt::rialto::GetImmediateOutputRequest request;
    request.set_session_id(sessionId);
    request.set_source_id(sourceId);
    return request;
}

::firebolt::rialto::RenderFrameRequest createRenderFrameRequest(int sessionId)
{
    ::firebolt::rialto::RenderFrameRequest request;
    request.set_session_id(sessionId);
    return request;
}

::firebolt::rialto::SetVolumeRequest createSetVolumeRequest(int sessionId)
{
    ::firebolt::rialto::SetVolumeRequest request;
    request.set_session_id(sessionId);
    request.set_volume(kVolume);
    return request;
}

::firebolt::rialto::GetVolumeRequest createGetVolumeRequest(int sessionId)
{
    ::firebolt::rialto::GetVolumeRequest request;
    request.set_session_id(sessionId);
    return request;
}

::firebolt::rialto::SetMuteRequest createSetMuteRequest(int sessionId)
{
    ::firebolt::rialto::SetMuteRequest request;
    request.set_session_id(sessionId);
    request.set_mute(true);
    return request;
}

::firebolt::rialto::GetMuteRequest createGetMuteRequest(int sessionId)
{
    ::firebolt::rialto::GetMuteRequest request;
    request.set_session_id(sessionId);
    return request;
}

::firebolt::rialto::SetVideoWindowRequest createSetVideoWindowRequest(int sessionId)
{
    ::firebolt::rialto::SetVideoWindowRequest request;
    request.set_session_id(sessionId);
    request.set_x(kX);
    request.set_y(kY);
    request.set_width(kWidth);
    request.set_height(kHeight);
    return request;
}

::firebolt::rialto::FlushRequest createFlushRequest(int sessionId, int sourceId, bool resetTime)
{
    ::firebolt::rialto::FlushRequest request;
    request.set_session_id(sessionId);
    request.set_source_id(sourceId);
    request.set_reset_time(resetTime);
    return request;
}

::firebolt::rialto::SetSourcePositionRequest createSetSourcePositionRequest(int sessionId, int sourceId,
                                                                            std::int64_t position)
{
    ::firebolt::rialto::SetSourcePositionRequest request;
    request.set_session_id(sessionId);
    request.set_source_id(sourceId);
    request.set_position(position);
    return request;
}

::firebolt::rialto::ProcessAudioGapRequest createProcessAudioGapRequest(int sessionId, std::int64_t position,
                                                                        unsigned duration,
                                                                        std::int64_t discontinuityGap, bool audioAac)
{
    ::firebolt::rialto::ProcessAudioGapRequest request;
    request.set_session_id(sessionId);
    request.set_position(position);
    request.set_duration(duration);
    request.set_discontinuity_gap(discontinuityGap);
    request.set_audio_aac(audioAac);
    return request;
}

::firebolt::rialto::CreateMediaKeysRequest createCreateMediaKeysRequestWidevine()
{
    ::firebolt::rialto::CreateMediaKeysRequest request;
    request.set_key_system("com.widevine.alpha");
    return request;
}
::firebolt::rialto::CreateMediaKeysRequest createCreateMediaKeysRequestNetflix()
{
    ::firebolt::rialto::CreateMediaKeysRequest request;
    request.set_key_system("com.netflix.playready");
    return request;
}

::firebolt::rialto::CreateKeySessionRequest createCreateKeySessionRequest(int mediaKeysHandle)
{
    ::firebolt::rialto::CreateKeySessionRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    request.set_session_type(::firebolt::rialto::CreateKeySessionRequest_KeySessionType_TEMPORARY);
    request.set_is_ldl(false);
    return request;
}

::firebolt::rialto::GenerateRequestRequest createGenerateRequestRequest(int mediaKeysHandle, int keySessionId,
                                                                        const std::vector<unsigned char> &initData)
{
    ::firebolt::rialto::GenerateRequestRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    request.set_key_session_id(keySessionId);
    request.set_init_data_type(::firebolt::rialto::GenerateRequestRequest_InitDataType_CENC);
    for (auto i : initData)
    {
        request.add_init_data(i);
    }
    return request;
}

::firebolt::rialto::UpdateSessionRequest createUpdateSessionRequest(int mediaKeysHandle, int keySessionId,
                                                                    const std::vector<unsigned char> &response)
{
    ::firebolt::rialto::UpdateSessionRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    request.set_key_session_id(keySessionId);
    for (auto i : response)
    {
        request.add_response_data(i);
    }
    return request;
}

::firebolt::rialto::ContainsKeyRequest createContainsKeyRequest(int mediaKeysHandle, int keySessionId,
                                                                const std::vector<unsigned char> &keyId)
{
    ::firebolt::rialto::ContainsKeyRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    request.set_key_session_id(keySessionId);
    for (auto i : keyId)
    {
        request.add_key_id(i);
    }
    return request;
}

::firebolt::rialto::RemoveKeySessionRequest createRemoveKeySessionRequest(int mediaKeysHandle, int keySessionId)
{
    ::firebolt::rialto::RemoveKeySessionRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    request.set_key_session_id(keySessionId);
    return request;
}

::firebolt::rialto::LoadSessionRequest createLoadSessionRequest(int mediaKeysHandle, int keySessionId)
{
    ::firebolt::rialto::LoadSessionRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    request.set_key_session_id(keySessionId);
    return request;
}

::firebolt::rialto::CloseKeySessionRequest createCloseKeySessionRequest(int mediaKeysHandle, int keySessionId)
{
    ::firebolt::rialto::CloseKeySessionRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    request.set_key_session_id(keySessionId);
    return request;
}

::firebolt::rialto::SetDrmHeaderRequest createSetDrmHeaderRequest(int mediaKeysHandle, int keySessionId,
                                                                  const std::vector<unsigned char> &keyId)
{
    ::firebolt::rialto::SetDrmHeaderRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    request.set_key_session_id(keySessionId);
    for (auto i : keyId)
    {
        request.add_request_data(i);
    }
    return request;
}

::firebolt::rialto::GetLastDrmErrorRequest createGetLastDrmErrorRequest(int mediaKeysHandle, int keySessionId)
{
    ::firebolt::rialto::GetLastDrmErrorRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    request.set_key_session_id(keySessionId);
    return request;
}

::firebolt::rialto::GetCdmKeySessionIdRequest createGetCdmKeySessionIdRequest(int mediaKeysHandle, int keySessionId)
{
    ::firebolt::rialto::GetCdmKeySessionIdRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    request.set_key_session_id(keySessionId);
    return request;
}

::firebolt::rialto::DestroyMediaKeysRequest createDestroyMediaKeysRequest(int mediaKeysHandle)
{
    ::firebolt::rialto::DestroyMediaKeysRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    return request;
}

::firebolt::rialto::DeleteDrmStoreRequest createDeleteDrmStoreRequest(int mediaKeysHandle)
{
    ::firebolt::rialto::DeleteDrmStoreRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    return request;
}

::firebolt::rialto::DeleteKeyStoreRequest createDeleteKeyStoreRequest(int mediaKeysHandle)
{
    ::firebolt::rialto::DeleteKeyStoreRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    return request;
}

::firebolt::rialto::GetDrmStoreHashRequest createGetDrmStoreHashRequest(int mediaKeysHandle)
{
    ::firebolt::rialto::GetDrmStoreHashRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    return request;
}

::firebolt::rialto::GetKeyStoreHashRequest createGetKeyStoreHashRequest(int mediaKeysHandle)
{
    ::firebolt::rialto::GetKeyStoreHashRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    return request;
}

::firebolt::rialto::GetLdlSessionsLimitRequest createGetLdlSessionsLimitRequest(int mediaKeysHandle)
{
    ::firebolt::rialto::GetLdlSessionsLimitRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    return request;
}

::firebolt::rialto::GetDrmTimeRequest createGetDrmTimeRequest(int mediaKeysHandle)
{
    ::firebolt::rialto::GetDrmTimeRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    return request;
}

::firebolt::rialto::ReleaseKeySessionRequest createReleaseKeySessionRequest(int mediaKeysHandle, int keySessionId)
{
    ::firebolt::rialto::ReleaseKeySessionRequest request;
    request.set_media_keys_handle(mediaKeysHandle);
    request.set_key_session_id(keySessionId);
    return request;
}

::firebolt::rialto::GetSupportedKeySystemsRequest createGetSupportedKeySystemsRequest()
{
    ::firebolt::rialto::GetSupportedKeySystemsRequest request;
    return request;
}

::firebolt::rialto::SupportsKeySystemRequest createSupportsKeySystemRequest(const std::string &keySystem)
{
    ::firebolt::rialto::SupportsKeySystemRequest request;
    request.set_key_system(keySystem.c_str());
    return request;
}

::firebolt::rialto::GetSupportedKeySystemVersionRequest
createGetSupportedKeySystemVersionRequest(const std::string &keySystem)
{
    ::firebolt::rialto::GetSupportedKeySystemVersionRequest request;
    request.set_key_system(keySystem.c_str());
    return request;
}

::firebolt::rialto::IsServerCertificateSupportedRequest
createIsServerCertificateSupportedRequest(const std::string &keySystem)
{
    ::firebolt::rialto::IsServerCertificateSupportedRequest request;
    request.set_key_system(keySystem.c_str());
    return request;
}

::firebolt::rialto::GetSharedMemoryRequest createGetSharedMemoryRequest()
{
    return ::firebolt::rialto::GetSharedMemoryRequest();
}

::firebolt::rialto::RegisterClientRequest createRegisterClientRequest()
{
    ::firebolt::rialto::RegisterClientRequest request;
    request.mutable_client_schema_version()->set_major(std::stoul(PROJECT_VER_MAJOR));
    request.mutable_client_schema_version()->set_minor(std::stoul(PROJECT_VER_MINOR));
    request.mutable_client_schema_version()->set_patch(std::stoul(PROJECT_VER_PATCH));
    return request;
}

::firebolt::rialto::AckRequest createAckRequest(int controlHandle, int id)
{
    ::firebolt::rialto::AckRequest request;
    request.set_control_handle(controlHandle);
    request.set_id(id);
    return request;
}

::firebolt::rialto::GetSupportedMimeTypesRequest
createGetSupportedMimeTypesRequest(const ProtoMediaSourceType &mediaSourceType)
{
    ::firebolt::rialto::GetSupportedMimeTypesRequest request;
    request.set_media_type(mediaSourceType);
    return request;
}

::firebolt::rialto::IsMimeTypeSupportedRequest createIsMimeTypeSupportedRequest(const std::string &mimeType)
{
    ::firebolt::rialto::IsMimeTypeSupportedRequest request;
    request.set_mime_type(mimeType);
    return request;
}

::firebolt::rialto::CreateWebAudioPlayerRequest
createCreateWebAudioPlayerRequest(uint32 pcmRate, uint32 pcmChannels, uint32 pcmSampleSize, bool pcmIsBigEndian,
                                  bool pcmIsSigned, bool pcmIsFloat, const std::string &audioMimeType, uint32 priority)
{
    ::firebolt::rialto::CreateWebAudioPlayerRequest request;
    auto configPcm = request.mutable_config()->mutable_pcm();
    configPcm->set_rate(pcmRate);
    configPcm->set_channels(pcmChannels);
    configPcm->set_sample_size(pcmSampleSize);
    configPcm->set_is_big_endian(pcmIsBigEndian);
    configPcm->set_is_signed(pcmIsSigned);
    configPcm->set_is_float(pcmIsFloat);
    request.set_audio_mime_type(audioMimeType);
    request.set_priority(priority);
    return request;
}

::firebolt::rialto::DestroyWebAudioPlayerRequest createDestroyWebAudioPlayerRequest(int32 webAudioPlayerHandle)
{
    ::firebolt::rialto::DestroyWebAudioPlayerRequest request;
    request.set_web_audio_player_handle(webAudioPlayerHandle);
    return request;
}

::firebolt::rialto::WebAudioPlayRequest createWebAudioPlayRequest(int32 webAudioPlayerHandle)
{
    ::firebolt::rialto::WebAudioPlayRequest request;
    request.set_web_audio_player_handle(webAudioPlayerHandle);
    return request;
}

::firebolt::rialto::WebAudioPauseRequest createWebAudioPauseRequest(int32 webAudioPlayerHandle)
{
    ::firebolt::rialto::WebAudioPauseRequest request;
    request.set_web_audio_player_handle(webAudioPlayerHandle);
    return request;
}

::firebolt::rialto::WebAudioSetEosRequest createWebAudioSetEosRequest(int32 webAudioPlayerHandle)
{
    ::firebolt::rialto::WebAudioSetEosRequest request;
    request.set_web_audio_player_handle(webAudioPlayerHandle);
    return request;
}

::firebolt::rialto::WebAudioGetBufferAvailableRequest createWebAudioGetBufferAvailableRequest(int32 webAudioPlayerHandle)
{
    ::firebolt::rialto::WebAudioGetBufferAvailableRequest request;
    request.set_web_audio_player_handle(webAudioPlayerHandle);
    return request;
}

::firebolt::rialto::WebAudioGetBufferDelayRequest createWebAudioGetBufferDelayRequest(int32 webAudioPlayerHandle)
{
    ::firebolt::rialto::WebAudioGetBufferDelayRequest request;
    request.set_web_audio_player_handle(webAudioPlayerHandle);
    return request;
}

::firebolt::rialto::WebAudioWriteBufferRequest createWebAudioWriteBufferRequest(int32 webAudioPlayerHandle,
                                                                                uint32 numberOfFrames)
{
    ::firebolt::rialto::WebAudioWriteBufferRequest request;
    request.set_web_audio_player_handle(webAudioPlayerHandle);
    request.set_number_of_frames(numberOfFrames);
    return request;
}

::firebolt::rialto::WebAudioGetDeviceInfoRequest createWebAudioGetDeviceInfoRequest(int32 webAudioPlayerHandle)
{
    ::firebolt::rialto::WebAudioGetDeviceInfoRequest request;
    request.set_web_audio_player_handle(webAudioPlayerHandle);
    return request;
}

::firebolt::rialto::WebAudioSetVolumeRequest createWebAudioSetVolumeRequest(int32 webAudioPlayerHandle, double volume)
{
    ::firebolt::rialto::WebAudioSetVolumeRequest request;
    request.set_web_audio_player_handle(webAudioPlayerHandle);
    request.set_volume(volume);
    return request;
}

::firebolt::rialto::WebAudioGetVolumeRequest createWebAudioGetVolumeRequest(int32 webAudioPlayerHandle)
{
    ::firebolt::rialto::WebAudioGetVolumeRequest request;
    request.set_web_audio_player_handle(webAudioPlayerHandle);
    return request;
}

} // namespace firebolt::rialto::server::ct
