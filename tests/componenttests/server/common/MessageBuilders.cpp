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

::firebolt::rialto::CreateSessionRequest createCreateSessionRequest()
{
    ::firebolt::rialto::CreateSessionRequest request;
    request.set_max_width(kWidth);
    request.set_max_height(kHeight);
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

::rialto::PingRequest createPingRequest(::google::protobuf::int32 id)
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

::firebolt::rialto::CreateMediaKeysRequest createCreateMediaKeysRequest()
{
    ::firebolt::rialto::CreateMediaKeysRequest request;
    request.set_key_system("com.widevine.alpha");
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

::firebolt::rialto::GetSharedMemoryRequest createGetSharedMemoryRequest()
{
    return ::firebolt::rialto::GetSharedMemoryRequest();
}
} // namespace firebolt::rialto::server::ct
