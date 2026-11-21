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

#include "VideoFrameCaptureIpc.h"
#include "RialtoClientLogging.h"

#include <exception>
#include <stdexcept>

namespace firebolt::rialto::client
{
std::shared_ptr<IVideoFrameCaptureIpcFactory> IVideoFrameCaptureIpcFactory::createFactory()
{
    std::shared_ptr<IVideoFrameCaptureIpcFactory> factory;
    try
    {
        factory = std::make_shared<VideoFrameCaptureIpcFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the video frame capture ipc factory, reason: %s", e.what());
    }
    return factory;
}

std::shared_ptr<IVideoFrameCaptureIpc> VideoFrameCaptureIpcFactory::createVideoFrameCaptureIpc() const
{
    std::shared_ptr<IVideoFrameCaptureIpc> videoFrameCaptureIpc;
    try
    {
        videoFrameCaptureIpc = std::make_shared<VideoFrameCaptureIpc>(IIpcClientAccessor::instance().getIpcClient());
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the video frame capture ipc, reason: %s", e.what());
    }
    return videoFrameCaptureIpc;
}

VideoFrameCaptureIpc::VideoFrameCaptureIpc(IIpcClient &ipcClient) : IpcModule(ipcClient)
{
    if (!attachChannel())
    {
        throw std::runtime_error("Failed attach to the ipc channel");
    }
}

VideoFrameCaptureIpc::~VideoFrameCaptureIpc()
{
    detachChannel();
}

bool VideoFrameCaptureIpc::createRpcStubs(const std::shared_ptr<ipc::IChannel> &ipcChannel)
{
    m_videoFrameCaptureStub = std::make_unique<::firebolt::rialto::VideoFrameCaptureModule_Stub>(ipcChannel.get());
    return m_videoFrameCaptureStub != nullptr;
}

bool VideoFrameCaptureIpc::createVideoFrameCapture(GraphicsAPIType api, std::uint32_t &captureId)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::CreateVideoFrameCaptureRequest request;
    request.set_api(api == GraphicsAPIType::VULKAN ? firebolt::rialto::GRAPHICS_API_TYPE_VULKAN
                                                   : firebolt::rialto::GRAPHICS_API_TYPE_GL);
    firebolt::rialto::CreateVideoFrameCaptureResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_videoFrameCaptureStub->createVideoFrameCapture(ipcController.get(), &request, &response, blockingClosure.get());
    blockingClosure->wait();

    if (ipcController->Failed())
    {
        // includes a server without the VideoFrameCaptureModule
        RIALTO_CLIENT_LOG_WARN("Failed to create a video frame capture due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }
    if (response.status() != firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK || !response.has_capture_id())
    {
        RIALTO_CLIENT_LOG_WARN("Server did not create a video frame capture, status %d",
                               static_cast<int>(response.status()));
        return false;
    }
    captureId = response.capture_id();
    return true;
}

bool VideoFrameCaptureIpc::destroyVideoFrameCapture(std::uint32_t captureId)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::DestroyVideoFrameCaptureRequest request;
    request.set_capture_id(captureId);
    firebolt::rialto::DestroyVideoFrameCaptureResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_videoFrameCaptureStub->destroyVideoFrameCapture(ipcController.get(), &request, &response, blockingClosure.get());
    blockingClosure->wait();

    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_WARN("Failed to destroy video frame capture %u due to '%s'", captureId,
                               ipcController->ErrorText().c_str());
        return false;
    }
    return true;
}
} // namespace firebolt::rialto::client
