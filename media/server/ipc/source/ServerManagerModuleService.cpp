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

#include "ServerManagerModuleService.h"
#include "AckSender.h"
#include "ISessionServerManager.h"
#include "RialtoServerLogging.h"
#include <AudioDecoderCapabilities.h>
#include <IIpcController.h>
#include <VideoDecoderCapabilities.h>
#include <optional>

namespace
{
firebolt::rialto::common::AudioDecoderCapabilities convertAudioCapabilities(const rialto::AudioCapabilities &src)
{
    firebolt::rialto::common::AudioDecoderCapabilities result;
    result.interfaceVersion = src.interface_version();
    result.schemaVersion    = src.schema_version();
    // Capability fields are populated by GstCapabilities path B if needed;
    // here we carry only the structured data forwarded from the ServerManager.
    return result;
}

firebolt::rialto::common::VideoDecoderCapabilities convertVideoCapabilities(const rialto::VideoCapabilities &src)
{
    firebolt::rialto::common::VideoDecoderCapabilities result;
    result.interfaceVersion = src.interface_version();
    result.schemaVersion    = src.schema_version();
    return result;
}

firebolt::rialto::common::SessionServerState convertSessionServerState(const rialto::SessionServerState &state)
{
    switch (state)
    {
    case rialto::SessionServerState::UNINITIALIZED:
        return firebolt::rialto::common::SessionServerState::UNINITIALIZED;
    case rialto::SessionServerState::INACTIVE:
        return firebolt::rialto::common::SessionServerState::INACTIVE;
    case rialto::SessionServerState::ACTIVE:
        return firebolt::rialto::common::SessionServerState::ACTIVE;
    case rialto::SessionServerState::NOT_RUNNING:
        return firebolt::rialto::common::SessionServerState::NOT_RUNNING;
    case rialto::SessionServerState::ERROR:
        return firebolt::rialto::common::SessionServerState::ERROR;
    }
    return firebolt::rialto::common::SessionServerState::ERROR;
}
} // namespace

namespace firebolt::rialto::server::ipc
{
std::shared_ptr<IServerManagerModuleServiceFactory> IServerManagerModuleServiceFactory::createFactory()
{
    std::shared_ptr<IServerManagerModuleServiceFactory> factory;

    try
    {
        factory = std::make_shared<ServerManagerModuleServiceFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the server maanger module service factory, reason: %s", e.what());
    }

    return factory;
}

std::shared_ptr<::rialto::ServerManagerModule>
ServerManagerModuleServiceFactory::create(service::ISessionServerManager &sessionServerManager) const
{
    std::shared_ptr<::rialto::ServerManagerModule> serverManagerModule;
    try
    {
        serverManagerModule = std::make_shared<ServerManagerModuleService>(sessionServerManager);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the server manager module service, reason: %s", e.what());
    }

    return serverManagerModule;
}

ServerManagerModuleService::ServerManagerModuleService(service::ISessionServerManager &sessionServerManager)
    : m_sessionServerManager{sessionServerManager}
{
}

ServerManagerModuleService::~ServerManagerModuleService() {}

void ServerManagerModuleService::setConfiguration(::google::protobuf::RpcController *controller,
                                                  const ::rialto::SetConfigurationRequest *request,
                                                  ::rialto::SetConfigurationResponse *response,
                                                  ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("SetConfiguration received from ServerManager");
    common::MaxResourceCapabilitites maxResource{request->resources().maxplaybacks(),
                                                 request->resources().maxwebaudioplayers()};
    const auto kClientDisplayName = request->has_clientdisplayname() ? request->clientdisplayname() : "";

    // Deserialise typed capability fields into C++ structs (optional — absent fields → std::nullopt)
    std::optional<firebolt::rialto::common::AudioDecoderCapabilities> audioCaps;
    std::optional<firebolt::rialto::common::VideoDecoderCapabilities> videoCaps;
    if (request->has_audiocapabilities() && request->has_videocapabilities())
    {
        RIALTO_SERVER_LOG_DEBUG("ServerManagerModuleService: audio/video capabilities received in SetConfiguration");
        audioCaps = convertAudioCapabilities(request->audiocapabilities());
        videoCaps = convertVideoCapabilities(request->videocapabilities());
        RIALTO_SERVER_LOG_INFO("ServerManagerModuleService: capabilities deserialised and forwarded to configureServices");
    }
    else
    {
        RIALTO_SERVER_LOG_INFO("ServerManagerModuleService: no capability fields - session server will use GStreamer query fallback");
    }

    bool success{true};
    if (request->has_sessionmanagementsocketfd())
    {
        m_sessionServerManager.configureIpc(request->sessionmanagementsocketfd());
    }
    else
    {
        m_sessionServerManager.configureIpc(request->sessionmanagementsocketname(), request->socketpermissions(),
                                            request->socketowner(), request->socketgroup());
    }

    RIALTO_SERVER_LOG_INFO("USHA: ServerManagerModuleService: setConfiguration: calling configureServices");
    success &= m_sessionServerManager.configureServices(convertSessionServerState(request->initialsessionserverstate()),
                                                        maxResource, kClientDisplayName, request->appname(),
                                                        audioCaps, videoCaps);
    m_sessionServerManager.setLogLevels(static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().defaultloglevels()),
                                        static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().clientloglevels()),
                                        static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().sessionserverloglevels()),
                                        static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().ipcloglevels()),
                                        static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().servermanagerloglevels()),
                                        static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().commonloglevels()));
    if (!success)
    {
        RIALTO_SERVER_LOG_ERROR("SetConfiguration operation failed");
        controller->SetFailed("SetConfiguration failed");
    }
    done->Run();
}

void ServerManagerModuleService::setState(::google::protobuf::RpcController *controller,
                                          const ::rialto::SetStateRequest *request,
                                          ::rialto::SetStateResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("SetState received from ServerManager");
    bool success = m_sessionServerManager.setState(convertSessionServerState(request->sessionserverstate()));
    if (!success)
    {
        RIALTO_SERVER_LOG_ERROR("SetState operation failed");
        controller->SetFailed("SetState failed");
    }
    done->Run();
}

void ServerManagerModuleService::setLogLevels(::google::protobuf::RpcController *controller,
                                              const ::rialto::SetLogLevelsRequest *request,
                                              ::rialto::SetLogLevelsResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("setLogLevels received from ServerManager");
    m_sessionServerManager.setLogLevels(static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().defaultloglevels()),
                                        static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().clientloglevels()),
                                        static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().sessionserverloglevels()),
                                        static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().ipcloglevels()),
                                        static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().servermanagerloglevels()),
                                        static_cast<RIALTO_DEBUG_LEVEL>(request->loglevels().commonloglevels()));
    done->Run();
}

void ServerManagerModuleService::ping(::google::protobuf::RpcController *controller, const ::rialto::PingRequest *request,
                                      ::rialto::PingResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("Ping received from ServerManager");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }
    bool success = m_sessionServerManager.ping(request->id(), std::make_shared<AckSender>(ipcController->getClient()));
    if (!success)
    {
        RIALTO_SERVER_LOG_ERROR("Ping failed");
        controller->SetFailed("Ping failed");
    }
    done->Run();
}
} // namespace firebolt::rialto::server::ipc
