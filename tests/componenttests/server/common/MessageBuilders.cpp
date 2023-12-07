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

::firebolt::rialto::CreateSessionRequest createCreateSessionRequest(const VideoRequirements &requirements)
{
    ::firebolt::rialto::CreateSessionRequest request;
    request.set_max_width(requirements.maxWidth);
    request.set_max_height(requirements.maxHeight);
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
    const ::google::protobuf::uint32 kLogLevel = 2;

    ::rialto::LogLevels *levels = new ::rialto::LogLevels;
    levels->set_defaultloglevels(kLogLevel);
    levels->set_clientloglevels(kLogLevel);
    levels->set_sessionserverloglevels(kLogLevel);
    levels->set_ipcloglevels(kLogLevel);
    levels->set_servermanagerloglevels(kLogLevel);
    levels->set_commonloglevels(kLogLevel);

    ::rialto::SetLogLevelsRequest request;
    request.set_allocated_loglevels(levels);

    return request;
}

void deleteSetLogLevelsRequest(::rialto::SetLogLevelsRequest request)
{
    delete request.release_loglevels();
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
