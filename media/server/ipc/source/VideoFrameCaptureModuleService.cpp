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

#include "VideoFrameCaptureModuleService.h"
#include "RialtoServerLogging.h"
#include <IIpcController.h>

#include <exception>
#include <optional>

namespace
{
std::optional<firebolt::rialto::GraphicsAPIType> toGraphicsApi(firebolt::rialto::VideoFrameCaptureGraphicsAPIType api)
{
    switch (api)
    {
    case firebolt::rialto::GRAPHICS_API_TYPE_GL:
        return firebolt::rialto::GraphicsAPIType::GL;
    case firebolt::rialto::GRAPHICS_API_TYPE_VULKAN:
        return firebolt::rialto::GraphicsAPIType::VULKAN;
    default:
        return std::nullopt;
    }
}

firebolt::rialto::VideoFrameCaptureStatus
toProtoStatus(firebolt::rialto::server::service::VideoFrameCaptureServiceStatus status)
{
    using firebolt::rialto::server::service::VideoFrameCaptureServiceStatus;
    switch (status)
    {
    case VideoFrameCaptureServiceStatus::OK:
        return firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK;
    case VideoFrameCaptureServiceStatus::UNSUPPORTED:
        return firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_UNSUPPORTED;
    case VideoFrameCaptureServiceStatus::INVALID_ARGUMENT:
        return firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT;
    }
    return firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_FATAL_ERROR;
}
} // namespace

namespace firebolt::rialto::server::ipc
{
std::shared_ptr<IVideoFrameCaptureModuleServiceFactory> IVideoFrameCaptureModuleServiceFactory::createFactory()
{
    std::shared_ptr<IVideoFrameCaptureModuleServiceFactory> factory;
    try
    {
        factory = std::make_shared<VideoFrameCaptureModuleServiceFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the video frame capture module service factory, reason: %s", e.what());
    }
    return factory;
}

std::shared_ptr<IVideoFrameCaptureModuleService>
VideoFrameCaptureModuleServiceFactory::create(service::IVideoFrameCaptureService &videoFrameCaptureService) const
{
    std::shared_ptr<IVideoFrameCaptureModuleService> module;
    try
    {
        module = std::make_shared<VideoFrameCaptureModuleService>(videoFrameCaptureService);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the video frame capture module service, reason: %s", e.what());
    }
    return module;
}

VideoFrameCaptureModuleService::VideoFrameCaptureModuleService(service::IVideoFrameCaptureService &videoFrameCaptureService)
    : m_videoFrameCaptureService{videoFrameCaptureService}
{
}

void VideoFrameCaptureModuleService::clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client connected to the video frame capture module");
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_clientIds[ipcClient] = m_nextClientId++;
    }
    ipcClient->exportService(shared_from_this());
}

void VideoFrameCaptureModuleService::clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client disconnected from the video frame capture module");
    std::optional<service::VideoFrameCaptureClientId> clientId;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        auto it = m_clientIds.find(ipcClient);
        if (it != m_clientIds.end())
        {
            clientId = it->second;
            m_clientIds.erase(it);
        }
    }
    if (clientId)
    {
        m_videoFrameCaptureService.removeClient(*clientId);
    }
}

std::optional<service::VideoFrameCaptureClientId>
VideoFrameCaptureModuleService::captureClientIdOf(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto it = m_clientIds.find(ipcClient);
    if (it == m_clientIds.end())
        return std::nullopt;
    return it->second;
}

bool VideoFrameCaptureModuleService::clientIdOf(::google::protobuf::RpcController *controller,
                                                service::VideoFrameCaptureClientId &clientId)
{
    auto *ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        return false;
    }
    std::lock_guard<std::mutex> lock{m_mutex};
    auto it = m_clientIds.find(ipcController->getClient());
    if (it == m_clientIds.end())
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture call from an unknown client");
        return false;
    }
    clientId = it->second;
    return true;
}

void VideoFrameCaptureModuleService::createVideoFrameCapture(
    ::google::protobuf::RpcController *controller, const ::firebolt::rialto::CreateVideoFrameCaptureRequest *request,
    ::firebolt::rialto::CreateVideoFrameCaptureResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    service::VideoFrameCaptureClientId clientId{0};
    if (!clientIdOf(controller, clientId))
    {
        controller->SetFailed("Unknown client");
        done->Run();
        return;
    }

    std::uint32_t captureId{0};
    const service::VideoFrameCaptureServiceStatus kStatus =
        m_videoFrameCaptureService.createCapture(clientId, toGraphicsApi(request->api()), captureId);
    response->set_status(toProtoStatus(kStatus));
    if (kStatus == service::VideoFrameCaptureServiceStatus::OK)
    {
        response->set_capture_id(captureId);
    }
    done->Run();
}

void VideoFrameCaptureModuleService::destroyVideoFrameCapture(
    ::google::protobuf::RpcController *controller, const ::firebolt::rialto::DestroyVideoFrameCaptureRequest *request,
    ::firebolt::rialto::DestroyVideoFrameCaptureResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    (void)response;
    service::VideoFrameCaptureClientId clientId{0};
    if (!clientIdOf(controller, clientId))
    {
        controller->SetFailed("Unknown client");
    }
    else if (!m_videoFrameCaptureService.destroyCapture(clientId, request->capture_id()))
    {
        controller->SetFailed("Unknown capture");
    }
    done->Run();
}
} // namespace firebolt::rialto::server::ipc
