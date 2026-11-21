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

#include <unistd.h>

#include <exception>
#include <stdexcept>

namespace
{
firebolt::rialto::client::CaptureAcquireStatus toAcquireStatus(firebolt::rialto::VideoFrameCaptureStatus status)
{
    using firebolt::rialto::client::CaptureAcquireStatus;
    switch (status)
    {
    case firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK:
        return CaptureAcquireStatus::OK;
    case firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_NEW_FRAME:
        return CaptureAcquireStatus::NO_NEW_FRAME;
    case firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_FRAME:
        return CaptureAcquireStatus::NO_FRAME;
    case firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_UNSUPPORTED:
        return CaptureAcquireStatus::UNSUPPORTED;
    case firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT:
        return CaptureAcquireStatus::INVALID_ARGUMENT;
    case firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_RESOURCES:
        return CaptureAcquireStatus::NO_RESOURCES;
    default:
        return CaptureAcquireStatus::FATAL_ERROR;
    }
}

void closeReceivedFds(const firebolt::rialto::AcquireCurrentVideoFrameResponse &response)
{
    for (const auto &kExported : response.exported_dma_bufs())
    {
        if (kExported.fd() >= 0)
            close(kExported.fd());
    }
}
} // namespace

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

CaptureAcquireResult VideoFrameCaptureIpc::acquireCurrentFrame(std::uint32_t captureId)
{
    CaptureAcquireResult result;
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return result;
    }

    firebolt::rialto::AcquireCurrentVideoFrameRequest request;
    request.set_capture_id(captureId);
    firebolt::rialto::AcquireCurrentVideoFrameResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_videoFrameCaptureStub->acquireCurrentFrame(ipcController.get(), &request, &response, blockingClosure.get());
    blockingClosure->wait();

    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_WARN("Failed to acquire a video frame due to '%s'", ipcController->ErrorText().c_str());
        closeReceivedFds(response);
        return result;
    }

    result.status = toAcquireStatus(response.status());
    if (result.status != CaptureAcquireStatus::OK)
    {
        closeReceivedFds(response); // a non-OK reply carries none, but never leak what arrived
        return result;
    }

    CaptureFrameDescription &description = result.description;
    description.poolId = response.pool_id();
    description.slotIndex = response.slot_index();
    description.acquisitionToken = response.acquisition_token();
    description.presentationTimeNs = response.presentation_time_ns();
    description.visibleWidth = response.visible_region().width();
    description.visibleHeight = response.visible_region().height();
    description.dmaBufObjectsFlushed = response.dma_buf_objects_flushed();
    const auto &kMetadata = response.frame_metadata();
    description.drmFormat = kMetadata.drm_format();
    description.drmModifier = kMetadata.drm_modifier();
    description.backingWidth = kMetadata.backing_width();
    description.backingHeight = kMetadata.backing_height();
    for (const auto &kObject : kMetadata.dma_buf_objects())
        description.objects.push_back({kObject.object_id(), kObject.size_bytes()});
    for (const auto &kPlane : kMetadata.planes())
    {
        description.planes.push_back({kPlane.plane_index(), kPlane.dma_buf_object_id(), kPlane.offset_bytes(),
                                      kPlane.length_bytes(), kPlane.stride_bytes()});
    }
    const auto &kColour = kMetadata.color_description();
    description.colour = {static_cast<std::uint32_t>(kColour.ycbcr_color_space()),
                          static_cast<std::uint32_t>(kColour.sample_range()),
                          static_cast<std::uint32_t>(kColour.horizontal_chroma_siting()),
                          static_cast<std::uint32_t>(kColour.vertical_chroma_siting())};

    // the IPC layer placed received descriptors in these fields; they now belong to the caller
    for (const auto &kExported : response.exported_dma_bufs())
        result.fds.push_back({kExported.dma_buf_object_id(), kExported.fd()});

    if (response.has_pool_metadata())
    {
        result.pool = CapturePoolDescription{response.pool_metadata().pool_id(),
                                             static_cast<std::uint32_t>(response.pool_metadata().slots_size())};
    }
    return result;
}

bool VideoFrameCaptureIpc::releaseFrame(std::uint32_t captureId, std::uint32_t poolId, std::uint32_t slotIndex,
                                        std::uint64_t acquisitionToken)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::ReleaseVideoFrameRequest request;
    request.set_capture_id(captureId);
    request.set_pool_id(poolId);
    request.set_slot_index(slotIndex);
    request.set_acquisition_token(acquisitionToken);
    firebolt::rialto::ReleaseVideoFrameResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_videoFrameCaptureStub->releaseFrame(ipcController.get(), &request, &response, blockingClosure.get());
    blockingClosure->wait();

    if (ipcController->Failed() || response.status() != firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK)
    {
        RIALTO_CLIENT_LOG_WARN("Failed to release video frame slot %u of capture %u", slotIndex, captureId);
        return false;
    }
    return true;
}

bool VideoFrameCaptureIpc::getPool(std::uint32_t captureId, CapturePoolDescription &pool)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::GetVideoFrameCapturePoolRequest request;
    request.set_capture_id(captureId);
    firebolt::rialto::GetVideoFrameCapturePoolResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_videoFrameCaptureStub->getPool(ipcController.get(), &request, &response, blockingClosure.get());
    blockingClosure->wait();

    if (ipcController->Failed() || response.status() != firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK)
    {
        RIALTO_CLIENT_LOG_WARN("Failed to get the pool of video frame capture %u", captureId);
        return false;
    }
    pool = CapturePoolDescription{response.pool_id(), static_cast<std::uint32_t>(response.slots_size())};
    return true;
}
} // namespace firebolt::rialto::client
