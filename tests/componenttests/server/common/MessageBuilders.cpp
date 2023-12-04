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

#include "MessageBuilders.h"
#include "Constants.h"

namespace firebolt::rialto::server::ct
{
::rialto::SetConfigurationRequest createGenericSetConfigurationReq()
{
    constexpr int kLogLevel{63};
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
    request.set_clientdisplayname("kDisplayName");
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
    request.mutable_audio_config()->set_number_of_channels(2);
    request.mutable_audio_config()->set_sample_rate(48000);
    // request.set_codec_data();
    request.set_stream_format(::firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW);
    // request.set_dolby_vision_profile();
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
    // request.set_codec_data();
    request.set_stream_format(::firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW);
    // request.set_dolby_vision_profile();
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
} // namespace firebolt::rialto::server::ct
