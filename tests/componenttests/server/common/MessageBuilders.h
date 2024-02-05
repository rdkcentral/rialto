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

#include <string>
#include <vector>

#include "controlmodule.pb.h"
#include "mediakeyscapabilitiesmodule.pb.h"
#include "mediakeysmodule.pb.h"
#include "mediapipelinecapabilitiesmodule.pb.h"
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
::firebolt::rialto::CreateSessionRequest createCreateSessionRequest();
::firebolt::rialto::LoadRequest createLoadRequest(int sessionId);
::firebolt::rialto::AttachSourceRequest createAttachAudioSourceRequest(int sessionId);
::firebolt::rialto::AttachSourceRequest createAttachVideoSourceRequest(int sessionId);
::firebolt::rialto::AllSourcesAttachedRequest createAllSourcesAttachedRequest(int sessionId);
::firebolt::rialto::HaveDataRequest createHaveDataRequest(int sessionId, unsigned numOfFrames, unsigned requestId);
::firebolt::rialto::PauseRequest createPauseRequest(int sessionId);
::firebolt::rialto::PlayRequest createPlayRequest(int sessionId);
::firebolt::rialto::RemoveSourceRequest createRemoveSourceRequest(int sessionId, int sourceId);
::firebolt::rialto::StopRequest createStopRequest(int sessionId);
::firebolt::rialto::DestroySessionRequest createDestroySessionRequest(int sessionId);
::firebolt::rialto::SetPlaybackRateRequest createSetPlaybackRateRequest(int sessionId);
::firebolt::rialto::SetPositionRequest createSetPositionRequest(int sessionId, std::int64_t position);
::firebolt::rialto::GetPositionRequest createGetPositionRequest(int sessionId);
::firebolt::rialto::RenderFrameRequest createRenderFrameRequest(int sessionId);
::firebolt::rialto::SetVolumeRequest createSetVolumeRequest(int sessionId);
::firebolt::rialto::GetVolumeRequest createGetVolumeRequest(int sessionId);
::firebolt::rialto::SetMuteRequest createSetMuteRequest(int sessionId);
::firebolt::rialto::GetMuteRequest createGetMuteRequest(int sessionId);
::firebolt::rialto::SetVideoWindowRequest createSetVideoWindowRequest(int sessionId);

// media keys module
::firebolt::rialto::CreateMediaKeysRequest createCreateMediaKeysRequestWidevine();
::firebolt::rialto::CreateMediaKeysRequest createCreateMediaKeysRequestNetflix();
::firebolt::rialto::CreateKeySessionRequest createCreateKeySessionRequest(int mediaKeysHandle);
::firebolt::rialto::GenerateRequestRequest createGenerateRequestRequest(int mediaKeysHandle, int keySessionId,
                                                                        const std::vector<unsigned char> &initData);
::firebolt::rialto::UpdateSessionRequest createUpdateSessionRequest(int mediaKeysHandle, int keySessionId,
                                                                    const std::vector<unsigned char> &response);
::firebolt::rialto::ContainsKeyRequest createContainsKeyRequest(int mediaKeysHandle, int keySessionId,
                                                                const std::vector<unsigned char> &keyId);
::firebolt::rialto::RemoveKeySessionRequest createRemoveKeySessionRequest(int mediaKeysHandle, int keySessionId);
::firebolt::rialto::LoadSessionRequest createLoadSessionRequest(int mediaKeysHandle, int keySessionId);
::firebolt::rialto::CloseKeySessionRequest createCloseKeySessionRequest(int mediaKeysHandle, int keySessionId);
::firebolt::rialto::SetDrmHeaderRequest createSetDrmHeaderRequest(int mediaKeysHandle, int keySessionId,
                                                                  const std::vector<unsigned char> &keyId);
::firebolt::rialto::GetLastDrmErrorRequest createGetLastDrmErrorRequest(int mediaKeysHandle, int keySessionId);
::firebolt::rialto::GetCdmKeySessionIdRequest createGetCdmKeySessionIdRequest(int mediaKeysHandle, int keySessionId);
::firebolt::rialto::DestroyMediaKeysRequest createDestroyMediaKeysRequest(int mediaKeysHandle);
::firebolt::rialto::DeleteDrmStoreRequest createDeleteDrmStoreRequest(int mediaKeysHandle);
::firebolt::rialto::DeleteKeyStoreRequest createDeleteKeyStoreRequest(int mediaKeysHandle);
::firebolt::rialto::GetDrmStoreHashRequest createGetDrmStoreHashRequest(int mediaKeysHandle);
::firebolt::rialto::GetKeyStoreHashRequest createGetKeyStoreHashRequest(int mediaKeysHandle);
::firebolt::rialto::GetLdlSessionsLimitRequest createGetLdlSessionsLimitRequest(int mediaKeysHandle);
::firebolt::rialto::GetDrmTimeRequest createGetDrmTimeRequest(int mediaKeysHandle);

// media keys capabilities module
::firebolt::rialto::GetSupportedKeySystemsRequest createGetSupportedKeySystemsRequest();
::firebolt::rialto::SupportsKeySystemRequest createSupportsKeySystemRequest(const std::string &keySystem);
::firebolt::rialto::GetSupportedKeySystemVersionRequest
createGetSupportedKeySystemVersionRequest(const std::string &keySystem);

// control module
::firebolt::rialto::GetSharedMemoryRequest createGetSharedMemoryRequest();
::firebolt::rialto::RegisterClientRequest createRegisterClientRequest();
::firebolt::rialto::AckRequest createAckRequest(int controlHandle, int id);

// media pipeline capabilities module
::firebolt::rialto::GetSupportedMimeTypesRequest
createGetSupportedMimeTypesRequest(const ProtoMediaSourceType &mediaSourceType);
::firebolt::rialto::IsMimeTypeSupportedRequest createIsMimeTypeSupportedRequest(const std::string &mimeType);
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_MESSAGE_BUILDERS_H_
