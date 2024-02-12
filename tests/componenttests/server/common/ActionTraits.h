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

#ifndef FIREBOLT_RIALTO_SERVER_CT_ACTION_TRAITS_H_
#define FIREBOLT_RIALTO_SERVER_CT_ACTION_TRAITS_H_

#include "controlmodule.pb.h"
#include "mediakeyscapabilitiesmodule.pb.h"
#include "mediakeysmodule.pb.h"
#include "mediapipelinecapabilitiesmodule.pb.h"
#include "mediapipelinemodule.pb.h"
#include "servermanagermodule.pb.h"
#include "webaudioplayermodule.pb.h"

namespace firebolt::rialto::server::ct
{
// servermanager module
struct SetConfiguration
{
    using RequestType = ::rialto::SetConfigurationRequest;
    using ResponseType = ::rialto::SetConfigurationResponse;
    using Stub = ::rialto::ServerManagerModule_Stub;
    static constexpr auto m_kFunction{&Stub::setConfiguration};
};

struct SetState
{
    using RequestType = ::rialto::SetStateRequest;
    using ResponseType = ::rialto::SetStateResponse;
    using Stub = ::rialto::ServerManagerModule_Stub;
    static constexpr auto m_kFunction{&Stub::setState};
};

struct SetLogLevels
{
    using RequestType = ::rialto::SetLogLevelsRequest;
    using ResponseType = ::rialto::SetLogLevelsResponse;
    using Stub = ::rialto::ServerManagerModule_Stub;
    static constexpr auto m_kFunction{&Stub::setLogLevels};
};

struct Ping
{
    using RequestType = ::rialto::PingRequest;
    using ResponseType = ::rialto::PingResponse;
    using Stub = ::rialto::ServerManagerModule_Stub;
    static constexpr auto m_kFunction{&Stub::ping};
};

// mediapipeline module
struct CreateSession
{
    using RequestType = ::firebolt::rialto::CreateSessionRequest;
    using ResponseType = ::firebolt::rialto::CreateSessionResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::createSession};
};

struct Load
{
    using RequestType = ::firebolt::rialto::LoadRequest;
    using ResponseType = ::firebolt::rialto::LoadResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::load};
};

struct AttachSource
{
    using RequestType = ::firebolt::rialto::AttachSourceRequest;
    using ResponseType = ::firebolt::rialto::AttachSourceResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::attachSource};
};

struct AllSourcesAttached
{
    using RequestType = ::firebolt::rialto::AllSourcesAttachedRequest;
    using ResponseType = ::firebolt::rialto::AllSourcesAttachedResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::allSourcesAttached};
};

struct HaveData
{
    using RequestType = ::firebolt::rialto::HaveDataRequest;
    using ResponseType = ::firebolt::rialto::HaveDataResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::haveData};
};

struct Pause
{
    using RequestType = ::firebolt::rialto::PauseRequest;
    using ResponseType = ::firebolt::rialto::PauseResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::pause};
};

struct Play
{
    using RequestType = ::firebolt::rialto::PlayRequest;
    using ResponseType = ::firebolt::rialto::PlayResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::play};
};

struct RemoveSource
{
    using RequestType = ::firebolt::rialto::RemoveSourceRequest;
    using ResponseType = ::firebolt::rialto::RemoveSourceResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::removeSource};
};

struct Stop
{
    using RequestType = ::firebolt::rialto::StopRequest;
    using ResponseType = ::firebolt::rialto::StopResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::stop};
};

struct DestroySession
{
    using RequestType = ::firebolt::rialto::DestroySessionRequest;
    using ResponseType = ::firebolt::rialto::DestroySessionResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::destroySession};
};

struct SetPlaybackRate
{
    using RequestType = ::firebolt::rialto::SetPlaybackRateRequest;
    using ResponseType = ::firebolt::rialto::SetPlaybackRateResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::setPlaybackRate};
};

struct SetPosition
{
    using RequestType = ::firebolt::rialto::SetPositionRequest;
    using ResponseType = ::firebolt::rialto::SetPositionResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::setPosition};
};

struct GetPosition
{
    using RequestType = ::firebolt::rialto::GetPositionRequest;
    using ResponseType = ::firebolt::rialto::GetPositionResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::getPosition};
};

struct RenderFrame
{
    using RequestType = ::firebolt::rialto::RenderFrameRequest;
    using ResponseType = ::firebolt::rialto::RenderFrameResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::renderFrame};
};

struct SetVolume
{
    using RequestType = ::firebolt::rialto::SetVolumeRequest;
    using ResponseType = ::firebolt::rialto::SetVolumeResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::setVolume};
};

struct GetVolume
{
    using RequestType = ::firebolt::rialto::GetVolumeRequest;
    using ResponseType = ::firebolt::rialto::GetVolumeResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::getVolume};
};

struct SetMute
{
    using RequestType = ::firebolt::rialto::SetMuteRequest;
    using ResponseType = ::firebolt::rialto::SetMuteResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::setMute};
};

struct GetMute
{
    using RequestType = ::firebolt::rialto::GetMuteRequest;
    using ResponseType = ::firebolt::rialto::GetMuteResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::getMute};
};

struct SetVideoWindow
{
    using RequestType = ::firebolt::rialto::SetVideoWindowRequest;
    using ResponseType = ::firebolt::rialto::SetVideoWindowResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    static constexpr auto m_kFunction{&Stub::setVideoWindow};
};

// mediakeys module
struct CreateMediaKeys
{
    using RequestType = ::firebolt::rialto::CreateMediaKeysRequest;
    using ResponseType = ::firebolt::rialto::CreateMediaKeysResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::createMediaKeys};
};

struct CreateKeySession
{
    using RequestType = ::firebolt::rialto::CreateKeySessionRequest;
    using ResponseType = ::firebolt::rialto::CreateKeySessionResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::createKeySession};
};

struct GenerateRequest
{
    using RequestType = ::firebolt::rialto::GenerateRequestRequest;
    using ResponseType = ::firebolt::rialto::GenerateRequestResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::generateRequest};
};

struct UpdateSession
{
    using RequestType = ::firebolt::rialto::UpdateSessionRequest;
    using ResponseType = ::firebolt::rialto::UpdateSessionResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::updateSession};
};

struct ContainsKey
{
    using RequestType = ::firebolt::rialto::ContainsKeyRequest;
    using ResponseType = ::firebolt::rialto::ContainsKeyResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::containsKey};
};

struct RemoveKeySession
{
    using RequestType = ::firebolt::rialto::RemoveKeySessionRequest;
    using ResponseType = ::firebolt::rialto::RemoveKeySessionResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::removeKeySession};
};

struct LoadSession
{
    using RequestType = ::firebolt::rialto::LoadSessionRequest;
    using ResponseType = ::firebolt::rialto::LoadSessionResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::loadSession};
};

struct CloseKeySession
{
    using RequestType = ::firebolt::rialto::CloseKeySessionRequest;
    using ResponseType = ::firebolt::rialto::CloseKeySessionResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::closeKeySession};
};

struct SetDrmHeader
{
    using RequestType = ::firebolt::rialto::SetDrmHeaderRequest;
    using ResponseType = ::firebolt::rialto::SetDrmHeaderResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::setDrmHeader};
};

struct GetLastDrmError
{
    using RequestType = ::firebolt::rialto::GetLastDrmErrorRequest;
    using ResponseType = ::firebolt::rialto::GetLastDrmErrorResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::getLastDrmError};
};

struct GetCdmKeySessionId
{
    using RequestType = ::firebolt::rialto::GetCdmKeySessionIdRequest;
    using ResponseType = ::firebolt::rialto::GetCdmKeySessionIdResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::getCdmKeySessionId};
};

struct DestroyMediaKeys
{
    using RequestType = ::firebolt::rialto::DestroyMediaKeysRequest;
    using ResponseType = ::firebolt::rialto::DestroyMediaKeysResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::destroyMediaKeys};
};

struct DeleteDrmStore
{
    using RequestType = ::firebolt::rialto::DeleteDrmStoreRequest;
    using ResponseType = ::firebolt::rialto::DeleteDrmStoreResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::deleteDrmStore};
};

struct DeleteKeyStore
{
    using RequestType = ::firebolt::rialto::DeleteKeyStoreRequest;
    using ResponseType = ::firebolt::rialto::DeleteKeyStoreResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::deleteKeyStore};
};

struct GetDrmStoreHash
{
    using RequestType = ::firebolt::rialto::GetDrmStoreHashRequest;
    using ResponseType = ::firebolt::rialto::GetDrmStoreHashResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::getDrmStoreHash};
};

struct GetKeyStoreHash
{
    using RequestType = ::firebolt::rialto::GetKeyStoreHashRequest;
    using ResponseType = ::firebolt::rialto::GetKeyStoreHashResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::getKeyStoreHash};
};

struct GetLdlSessionsLimit
{
    using RequestType = ::firebolt::rialto::GetLdlSessionsLimitRequest;
    using ResponseType = ::firebolt::rialto::GetLdlSessionsLimitResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::getLdlSessionsLimit};
};

struct GetDrmTime
{
    using RequestType = ::firebolt::rialto::GetDrmTimeRequest;
    using ResponseType = ::firebolt::rialto::GetDrmTimeResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    static constexpr auto m_kFunction{&Stub::getDrmTime};
};

// mediakeyscapabilities module
struct GetSupportedKeySystems
{
    using RequestType = ::firebolt::rialto::GetSupportedKeySystemsRequest;
    using ResponseType = ::firebolt::rialto::GetSupportedKeySystemsResponse;
    using Stub = ::firebolt::rialto::MediaKeysCapabilitiesModule_Stub;
    static constexpr auto m_kFunction{&Stub::getSupportedKeySystems};
};

struct SupportsKeySystem
{
    using RequestType = ::firebolt::rialto::SupportsKeySystemRequest;
    using ResponseType = ::firebolt::rialto::SupportsKeySystemResponse;
    using Stub = ::firebolt::rialto::MediaKeysCapabilitiesModule_Stub;
    static constexpr auto m_kFunction{&Stub::supportsKeySystem};
};

struct GetSupportedKeySystemVersion
{
    using RequestType = ::firebolt::rialto::GetSupportedKeySystemVersionRequest;
    using ResponseType = ::firebolt::rialto::GetSupportedKeySystemVersionResponse;
    using Stub = ::firebolt::rialto::MediaKeysCapabilitiesModule_Stub;
    static constexpr auto m_kFunction{&Stub::getSupportedKeySystemVersion};
};

// control module
struct GetSharedMemory
{
    using RequestType = ::firebolt::rialto::GetSharedMemoryRequest;
    using ResponseType = ::firebolt::rialto::GetSharedMemoryResponse;
    using Stub = ::firebolt::rialto::ControlModule_Stub;
    static constexpr auto m_kFunction{&Stub::getSharedMemory};
};

struct RegisterClient
{
    using RequestType = ::firebolt::rialto::RegisterClientRequest;
    using ResponseType = ::firebolt::rialto::RegisterClientResponse;
    using Stub = ::firebolt::rialto::ControlModule_Stub;
    static constexpr auto m_kFunction{&Stub::registerClient};
};

struct Ack
{
    using RequestType = ::firebolt::rialto::AckRequest;
    using ResponseType = ::firebolt::rialto::AckResponse;
    using Stub = ::firebolt::rialto::ControlModule_Stub;
    static constexpr auto m_kFunction{&Stub::ack};
};

// media pipeline capabilities module
struct GetSupportedMimeTypes
{
    using RequestType = ::firebolt::rialto::GetSupportedMimeTypesRequest;
    using ResponseType = ::firebolt::rialto::GetSupportedMimeTypesResponse;
    using Stub = ::firebolt::rialto::MediaPipelineCapabilitiesModule_Stub;
    static constexpr auto m_kFunction{&Stub::getSupportedMimeTypes};
};

struct IsMimeTypeSupported
{
    using RequestType = ::firebolt::rialto::IsMimeTypeSupportedRequest;
    using ResponseType = ::firebolt::rialto::IsMimeTypeSupportedResponse;
    using Stub = ::firebolt::rialto::MediaPipelineCapabilitiesModule_Stub;
    static constexpr auto m_kFunction{&Stub::isMimeTypeSupported};
};

// web audio player module
struct CreateWebAudioPlayer
{
    using RequestType = ::firebolt::rialto::CreateWebAudioPlayerRequest;
    using ResponseType = ::firebolt::rialto::CreateWebAudioPlayerResponse;
    using Stub = ::firebolt::rialto::WebAudioPlayerModule_Stub;
    static constexpr auto m_kFunction{&Stub::createWebAudioPlayer};
};

struct DestroyWebAudioPlayer
{
    using RequestType = ::firebolt::rialto::DestroyWebAudioPlayerRequest;
    using ResponseType = ::firebolt::rialto::DestroyWebAudioPlayerResponse;
    using Stub = ::firebolt::rialto::WebAudioPlayerModule_Stub;
    static constexpr auto m_kFunction{&Stub::destroyWebAudioPlayer};
};

struct WebAudioPlayWebAudioPlayer
{
    using RequestType = ::firebolt::rialto::WebAudioPlayRequest;
    using ResponseType = ::firebolt::rialto::WebAudioPlayResponse;
    using Stub = ::firebolt::rialto::WebAudioPlayerModule_Stub;
    static constexpr auto m_kFunction{&Stub::play};
};

struct WebAudioPause
{
    using RequestType = ::firebolt::rialto::WebAudioPauseRequest;
    using ResponseType = ::firebolt::rialto::WebAudioPauseResponse;
    using Stub = ::firebolt::rialto::WebAudioPlayerModule_Stub;
    static constexpr auto m_kFunction{&Stub::pause};
};

struct WebAudioSetEos
{
    using RequestType = ::firebolt::rialto::WebAudioSetEosRequest;
    using ResponseType = ::firebolt::rialto::WebAudioSetEosResponse;
    using Stub = ::firebolt::rialto::WebAudioPlayerModule_Stub;
    static constexpr auto m_kFunction{&Stub::setEos};
};

struct WebAudioGetBufferAvailable
{
    using RequestType = ::firebolt::rialto::WebAudioGetBufferAvailableRequest;
    using ResponseType = ::firebolt::rialto::WebAudioGetBufferAvailableResponse;
    using Stub = ::firebolt::rialto::WebAudioPlayerModule_Stub;
    static constexpr auto m_kFunction{&Stub::getBufferAvailable};
};

struct WebAudioGetBufferDelay
{
    using RequestType = ::firebolt::rialto::WebAudioGetBufferDelayRequest;
    using ResponseType = ::firebolt::rialto::WebAudioGetBufferDelayResponse;
    using Stub = ::firebolt::rialto::WebAudioPlayerModule_Stub;
    static constexpr auto m_kFunction{&Stub::getBufferDelay};
};

struct WebAudioWriteBuffer
{
    using RequestType = ::firebolt::rialto::WebAudioWriteBufferRequest;
    using ResponseType = ::firebolt::rialto::WebAudioWriteBufferResponse;
    using Stub = ::firebolt::rialto::WebAudioPlayerModule_Stub;
    static constexpr auto m_kFunction{&Stub::writeBuffer};
};

struct WebAudioGetDeviceInfo
{
    using RequestType = ::firebolt::rialto::WebAudioGetDeviceInfoRequest;
    using ResponseType = ::firebolt::rialto::WebAudioGetDeviceInfoResponse;
    using Stub = ::firebolt::rialto::WebAudioPlayerModule_Stub;
    static constexpr auto m_kFunction{&Stub::getDeviceInfo};
};

struct WebAudioSetVolume
{
    using RequestType = ::firebolt::rialto::WebAudioSetVolumeRequest;
    using ResponseType = ::firebolt::rialto::WebAudioSetVolumeResponse;
    using Stub = ::firebolt::rialto::WebAudioPlayerModule_Stub;
    static constexpr auto m_kFunction{&Stub::setVolume};
};

struct WebAudioGetVolume
{
    using RequestType = ::firebolt::rialto::WebAudioGetVolumeRequest;
    using ResponseType = ::firebolt::rialto::WebAudioGetVolumeResponse;
    using Stub = ::firebolt::rialto::WebAudioPlayerModule_Stub;
    static constexpr auto m_kFunction{&Stub::getVolume};
};

} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_ACTION_TRAITS_H_
