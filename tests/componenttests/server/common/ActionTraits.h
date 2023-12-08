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
#include "mediakeysmodule.pb.h"
#include "mediapipelinemodule.pb.h"
#include "servermanagermodule.pb.h"

namespace firebolt::rialto::server::ct
{
// servermanager module
struct SetConfiguration
{
    using RequestType = ::rialto::SetConfigurationRequest;
    using ResponseType = ::rialto::SetConfigurationResponse;
    using Stub = ::rialto::ServerManagerModule_Stub;
    using FunctionType = void (Stub::*)(google::protobuf::RpcController *, const RequestType *, ResponseType *,
                                        google::protobuf::Closure *);
    static constexpr FunctionType m_kFunction{&Stub::setConfiguration};
};

// mediapipeline module
struct CreateSession
{
    using RequestType = ::firebolt::rialto::CreateSessionRequest;
    using ResponseType = ::firebolt::rialto::CreateSessionResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    using FunctionType = void (Stub::*)(google::protobuf::RpcController *, const RequestType *, ResponseType *,
                                        google::protobuf::Closure *);
    static constexpr FunctionType m_kFunction{&Stub::createSession};
};

struct Load
{
    using RequestType = ::firebolt::rialto::LoadRequest;
    using ResponseType = ::firebolt::rialto::LoadResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    using FunctionType = void (Stub::*)(google::protobuf::RpcController *, const RequestType *, ResponseType *,
                                        google::protobuf::Closure *);
    static constexpr FunctionType m_kFunction{&Stub::load};
};

struct AttachSource
{
    using RequestType = ::firebolt::rialto::AttachSourceRequest;
    using ResponseType = ::firebolt::rialto::AttachSourceResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    using FunctionType = void (Stub::*)(google::protobuf::RpcController *, const RequestType *, ResponseType *,
                                        google::protobuf::Closure *);
    static constexpr FunctionType m_kFunction{&Stub::attachSource};
};

struct AllSourcesAttached
{
    using RequestType = ::firebolt::rialto::AllSourcesAttachedRequest;
    using ResponseType = ::firebolt::rialto::AllSourcesAttachedResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    using FunctionType = void (Stub::*)(google::protobuf::RpcController *, const RequestType *, ResponseType *,
                                        google::protobuf::Closure *);
    static constexpr FunctionType m_kFunction{&Stub::allSourcesAttached};
};

struct HaveData
{
    using RequestType = ::firebolt::rialto::HaveDataRequest;
    using ResponseType = ::firebolt::rialto::HaveDataResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    using FunctionType = void (Stub::*)(google::protobuf::RpcController *, const RequestType *, ResponseType *,
                                        google::protobuf::Closure *);
    static constexpr FunctionType m_kFunction{&Stub::haveData};
};

struct Pause
{
    using RequestType = ::firebolt::rialto::PauseRequest;
    using ResponseType = ::firebolt::rialto::PauseResponse;
    using Stub = ::firebolt::rialto::MediaPipelineModule_Stub;
    using FunctionType = void (Stub::*)(google::protobuf::RpcController *, const RequestType *, ResponseType *,
                                        google::protobuf::Closure *);
    static constexpr FunctionType m_kFunction{&Stub::pause};
};

// mediakeys module
struct CreateMediaKeys
{
    using RequestType = ::firebolt::rialto::CreateMediaKeysRequest;
    using ResponseType = ::firebolt::rialto::CreateMediaKeysResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    using FunctionType = void (Stub::*)(google::protobuf::RpcController *, const RequestType *, ResponseType *,
                                        google::protobuf::Closure *);
    static constexpr FunctionType m_kFunction{&Stub::createMediaKeys};
};

struct CreateKeySession
{
    using RequestType = ::firebolt::rialto::CreateKeySessionRequest;
    using ResponseType = ::firebolt::rialto::CreateKeySessionResponse;
    using Stub = ::firebolt::rialto::MediaKeysModule_Stub;
    using FunctionType = void (Stub::*)(google::protobuf::RpcController *, const RequestType *, ResponseType *,
                                        google::protobuf::Closure *);
    static constexpr FunctionType m_kFunction{&Stub::createKeySession};
};

// control module
struct GetSharedMemory
{
    using RequestType = ::firebolt::rialto::GetSharedMemoryRequest;
    using ResponseType = ::firebolt::rialto::GetSharedMemoryResponse;
    using Stub = ::firebolt::rialto::ControlModule_Stub;
    using FunctionType = void (Stub::*)(google::protobuf::RpcController *, const RequestType *, ResponseType *,
                                        google::protobuf::Closure *);
    static constexpr FunctionType m_kFunction{&Stub::getSharedMemory};
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_ACTION_TRAITS_H_
