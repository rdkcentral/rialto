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

#include "ControlModuleService.h"
#include "ControlClientServerInternal.h"
#include "IPlaybackService.h"
#include "RialtoServerLogging.h"
#include <IIpcController.h>
#include <algorithm>
#include <cstdint>

namespace firebolt::rialto::server::ipc
{
std::shared_ptr<IControlModuleServiceFactory> IControlModuleServiceFactory::createFactory()
{
    std::shared_ptr<IControlModuleServiceFactory> factory;

    try
    {
        factory = std::make_shared<ControlModuleServiceFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the rialto control module service factory, reason: %s", e.what());
    }

    return factory;
}

std::shared_ptr<IControlModuleService> ControlModuleServiceFactory::create(service::IPlaybackService &playbackService,
                                                                           service::IControlService &controlService) const
{
    std::shared_ptr<IControlModuleService> controlModule;

    try
    {
        controlModule = std::make_shared<ControlModuleService>(playbackService, controlService);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the rialto control module service, reason: %s", e.what());
    }

    return controlModule;
}

ControlModuleService::ControlModuleService(service::IPlaybackService &playbackService,
                                           service::IControlService &controlService)
    : m_playbackService{playbackService}, m_controlService{controlService}
{
}

ControlModuleService::~ControlModuleService()
{
    for (const auto &controlId : m_controlIds)
    {
        m_controlService.unregisterClient(controlId.second);
    }
}

void ControlModuleService::clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client Connected!");
    auto controlClient{std::make_shared<ControlClientServerInternal>(ipcClient)};
    m_controlIds.emplace(ipcClient, m_controlService.registerClient(controlClient));
    ipcClient->exportService(shared_from_this());
}

void ControlModuleService::clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client disconnected!");
    auto controlIdIter = m_controlIds.find(ipcClient);
    if (m_controlIds.end() != controlIdIter)
    {
        m_controlService.unregisterClient(controlIdIter->second);
        m_controlIds.erase(controlIdIter);
    }
}

void ControlModuleService::getSharedMemory(::google::protobuf::RpcController *controller,
                                           const ::firebolt::rialto::GetSharedMemoryRequest *request,
                                           ::firebolt::rialto::GetSharedMemoryResponse *response,
                                           ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    int32_t fd;
    uint32_t size;
    if (!m_playbackService.getSharedMemory(fd, size))
    {
        RIALTO_SERVER_LOG_ERROR("getSharedMemory failed");
        controller->SetFailed("Operation failed");
        done->Run();
        return;
    }
    response->set_fd(fd);
    response->set_size(size);
    done->Run();
}

void ControlModuleService::ack(::google::protobuf::RpcController *controller,
                               const ::firebolt::rialto::AckRequest *request, ::firebolt::rialto::AckResponse *response,
                               ::google::protobuf::Closure *done)
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
    if (!m_controlService.ack(request->id()))
    {
        RIALTO_SERVER_LOG_ERROR("ack failed");
        controller->SetFailed("Operation failed");
        done->Run();
        return;
    }
    done->Run();
}
} // namespace firebolt::rialto::server::ipc
