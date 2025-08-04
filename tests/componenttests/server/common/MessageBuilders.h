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
#include "webaudioplayermodule.pb.h"

namespace firebolt::rialto::server::ct
{
// server manager module
::rialto::SetConfigurationRequest createGenericSetConfigurationReq();
::rialto::SetStateRequest createSetStateRequest(::rialto::SessionServerState value);
::rialto::SetLogLevelsRequest createSetLogLevelsRequest();
::rialto::PingRequest createPingRequest(::google::protobuf::int32 id);

// media pipeline module
::firebolt::rialto::CreateSessionRequest createCreateSessionRequest(int width, int height);
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
::firebolt::rialto::SetImmediateOutputRequest createSetImmediateOutputRequest(int sessionId, int sourceId,
                                                                              bool immediateOutput);
::firebolt::rialto::GetImmediateOutputRequest createGetImmediateOutputRequest(int sessionId, int sourceId);
::firebolt::rialto::GetStatsRequest createGetStatsRequest(int sessionId, int sourceId);
::firebolt::rialto::RenderFrameRequest createRenderFrameRequest(int sessionId);
::firebolt::rialto::SetVolumeRequest createSetVolumeNormalRequest(int sessionId);
::firebolt::rialto::SetVolumeRequest createSetVolumeWithFadeRequest(int sessionId);
::firebolt::rialto::GetVolumeRequest createGetVolumeRequest(int sessionId);
::firebolt::rialto::SetMuteRequest createSetMuteRequest(int sessionId, int sourceId);
::firebolt::rialto::GetMuteRequest createGetMuteRequest(int sessionId, int sourceId);
::firebolt::rialto::SetLowLatencyRequest createSetLowLatencyRequest(int sessionId, bool lowLatency);
::firebolt::rialto::SetSyncRequest createSetSyncRequest(int sessionId, bool sync);
::firebolt::rialto::GetSyncRequest createGetSyncRequest(int sessionId);
::firebolt::rialto::SetSyncOffRequest createSetSyncOffRequest(int sessionId, bool syncOff);
::firebolt::rialto::SetStreamSyncModeRequest createSetStreamSyncModeRequest(int sessionId, int sourceId,
                                                                            bool streamSyncMode);
::firebolt::rialto::GetStreamSyncModeRequest createGetStreamSyncModeRequest(int sessionId);
::firebolt::rialto::SetBufferingLimitRequest createSetBufferingLimitRequest(int sessionId, uint32_t bufferingLimit);
::firebolt::rialto::GetBufferingLimitRequest createGetBufferingLimitRequest(int sessionId);
::firebolt::rialto::SetUseBufferingRequest createSetUseBufferingRequest(int sessionId, bool useBuffering);
::firebolt::rialto::GetUseBufferingRequest createGetUseBufferingRequest(int sessionId);
::firebolt::rialto::SetVideoWindowRequest createSetVideoWindowRequest(int sessionId);
::firebolt::rialto::FlushRequest createFlushRequest(int sessionId, int sourceId, bool resetTime);
::firebolt::rialto::SetSourcePositionRequest createSetSourcePositionRequest(int sessionId, int sourceId,
                                                                            std::int64_t position, bool resetTime,
                                                                            double appliedRate, uint64_t stopPosition);
::firebolt::rialto::ProcessAudioGapRequest createProcessAudioGapRequest(int sessionId, std::int64_t position,
                                                                        unsigned duration,
                                                                        std::int64_t discontinuityGap, bool audioAac);

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
::firebolt::rialto::ReleaseKeySessionRequest createReleaseKeySessionRequest(int mediaKeysHandle, int keySessionId);
::firebolt::rialto::GetMetricSystemDataRequest createGetMetricSystemDataRequest(int mediaKeysHandle);

// media keys capabilities module
::firebolt::rialto::GetSupportedKeySystemsRequest createGetSupportedKeySystemsRequest();
::firebolt::rialto::SupportsKeySystemRequest createSupportsKeySystemRequest(const std::string &keySystem);
::firebolt::rialto::GetSupportedKeySystemVersionRequest
createGetSupportedKeySystemVersionRequest(const std::string &keySystem);
::firebolt::rialto::IsServerCertificateSupportedRequest
createIsServerCertificateSupportedRequest(const std::string &keySystem);

// control module
::firebolt::rialto::GetSharedMemoryRequest createGetSharedMemoryRequest();
::firebolt::rialto::RegisterClientRequest createRegisterClientRequest();
::firebolt::rialto::AckRequest createAckRequest(int controlHandle, int id);

// media pipeline capabilities module
::firebolt::rialto::GetSupportedMimeTypesRequest
createGetSupportedMimeTypesRequest(const ProtoMediaSourceType &mediaSourceType);
::firebolt::rialto::IsMimeTypeSupportedRequest createIsMimeTypeSupportedRequest(const std::string &mimeType);
::firebolt::rialto::GetSupportedPropertiesRequest
createGetSupportedPropertiesRequest(const ProtoMediaSourceType &mediaType, const std::vector<std::string> &propertyNames);
::firebolt::rialto::IsVideoMasterRequest createIsVideoMasterRequest();

// web audio player module
::firebolt::rialto::CreateWebAudioPlayerRequest
createCreateWebAudioPlayerRequest(::google::protobuf::uint32 pcmRate, ::google::protobuf::uint32 pcmChannels,
                                  ::google::protobuf::uint32 pcmSampleSize, bool pcmIsBigEndian, bool pcmIsSigned,
                                  bool pcmIsFloat, const std::string &audioMimeType, ::google::protobuf::uint32 priority);
::firebolt::rialto::DestroyWebAudioPlayerRequest
createDestroyWebAudioPlayerRequest(::google::protobuf::int32 webAudioPlayerHandle);
::firebolt::rialto::WebAudioPlayRequest createWebAudioPlayRequest(::google::protobuf::int32 webAudioPlayerHandle);
::firebolt::rialto::WebAudioPauseRequest createWebAudioPauseRequest(::google::protobuf::int32 webAudioPlayerHandle);
::firebolt::rialto::WebAudioSetEosRequest createWebAudioSetEosRequest(::google::protobuf::int32 webAudioPlayerHandle);
::firebolt::rialto::WebAudioGetBufferAvailableRequest
createWebAudioGetBufferAvailableRequest(::google::protobuf::int32 webAudioPlayerHandle);
::firebolt::rialto::WebAudioGetBufferDelayRequest
createWebAudioGetBufferDelayRequest(::google::protobuf::int32 webAudioPlayerHandle);
::firebolt::rialto::WebAudioWriteBufferRequest
createWebAudioWriteBufferRequest(::google::protobuf::int32 webAudioPlayerHandle,
                                 ::google::protobuf::uint32 numberOfFrames);
::firebolt::rialto::WebAudioGetDeviceInfoRequest
createWebAudioGetDeviceInfoRequest(::google::protobuf::int32 webAudioPlayerHandle);
::firebolt::rialto::WebAudioSetVolumeRequest
createWebAudioSetVolumeRequest(::google::protobuf::int32 webAudioPlayerHandle, double volume);
::firebolt::rialto::WebAudioGetVolumeRequest createWebAudioGetVolumeRequest(::google::protobuf::int32 webAudioPlayerHandle);

} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_MESSAGE_BUILDERS_H_
