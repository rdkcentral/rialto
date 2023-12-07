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

#ifndef FIREBOLT_RIALTO_SERVER_CT_MESSAGE_BUILDERS_H_
#define FIREBOLT_RIALTO_SERVER_CT_MESSAGE_BUILDERS_H_

#include "MediaCommon.h"
#include "mediakeysmodule.pb.h"
#include "mediapipelinemodule.pb.h"
#include "servermanagermodule.pb.h"

namespace firebolt::rialto::server::ct
{
// server manager module
::rialto::SetConfigurationRequest createGenericSetConfigurationReq();
::rialto::SetStateRequest createSetStateRequest(::rialto::SessionServerState value);
::rialto::SetLogLevelsRequest createSetLogLevelsRequest();
::rialto::PingRequest createPingRequest(::google::protobuf::int32 id);

// media pipeline module
::firebolt::rialto::CreateSessionRequest createCreateSessionRequest(const VideoRequirements &requirements);
::firebolt::rialto::LoadRequest createLoadRequest(int sessionId);

// media keys module
::firebolt::rialto::CreateMediaKeysRequest createCreateMediaKeysRequest();
::firebolt::rialto::CreateKeySessionRequest createCreateKeySessionRequest(int mediaKeysHandle);
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_MESSAGE_BUILDERS_H_
