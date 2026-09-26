/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "MediaCapabilitiesModuleService.h"
#include "CapabilityConverters.h"
#include "RialtoServerLogging.h"
#include <IIpcController.h>

namespace firebolt::rialto::server::ipc
{
std::shared_ptr<IMediaCapabilitiesModuleServiceFactory> IMediaCapabilitiesModuleServiceFactory::createFactory()
{
    std::shared_ptr<IMediaCapabilitiesModuleServiceFactory> factory;

    try
    {
        factory = std::make_shared<MediaCapabilitiesModuleServiceFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media capabilities module service factory, reason: %s", e.what());
    }

    return factory;
}

std::shared_ptr<IMediaCapabilitiesModuleService>
MediaCapabilitiesModuleServiceFactory::create(service::IMediaPipelineService &mediaPipelineService) const
{
    std::shared_ptr<IMediaCapabilitiesModuleService> mediaCapabilitiesModule;

    try
    {
        mediaCapabilitiesModule = std::make_shared<MediaCapabilitiesModuleService>(mediaPipelineService);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media capabilities module service, reason: %s", e.what());
    }

    return mediaCapabilitiesModule;
}

MediaCapabilitiesModuleService::MediaCapabilitiesModuleService(service::IMediaPipelineService &mediaPipelineService)
    : m_mediaPipelineService{mediaPipelineService}
{
}

MediaCapabilitiesModuleService::~MediaCapabilitiesModuleService() {}

void MediaCapabilitiesModuleService::clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client Connected!");
    ipcClient->exportService(shared_from_this());
}

void MediaCapabilitiesModuleService::clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client disconnected!");
}

void MediaCapabilitiesModuleService::getSupportedAudioCapabilities(
    ::google::protobuf::RpcController *controller,
    const ::firebolt::rialto::GetSupportedAudioCapabilitiesRequest * /*request*/,
    ::firebolt::rialto::AudioCapabilities *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }

    const firebolt::rialto::common::AudioDecoderCapabilities kAudioCapabilities =
        m_mediaPipelineService.getSupportedAudioCapabilities();
    firebolt::rialto::ipc::common::serialiseAudioCapabilities(kAudioCapabilities, response);

    done->Run();
}

void MediaCapabilitiesModuleService::getSupportedVideoCapabilities(
    ::google::protobuf::RpcController *controller,
    const ::firebolt::rialto::GetSupportedVideoCapabilitiesRequest * /*request*/,
    ::firebolt::rialto::VideoCapabilities *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }

    const firebolt::rialto::common::VideoDecoderCapabilities kVideoCapabilities =
        m_mediaPipelineService.getSupportedVideoCapabilities();
    firebolt::rialto::ipc::common::serialiseVideoCapabilities(kVideoCapabilities, response);

    done->Run();
}
} // namespace firebolt::rialto::server::ipc
