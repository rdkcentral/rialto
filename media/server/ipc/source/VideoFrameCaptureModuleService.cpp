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
#include "VideoFrameCaptureStatus.h"
#include <IIpcController.h>

#include <exception>
#include <optional>

namespace
{
// The reply for an acquired frame (proposal 3.4): identity, timing, layout, colour and the descriptors.
// The pool description sent when the frame description changes: identity, format, size, colour and the slot
// layouts. Layouts carry no object IDs and no descriptors.
// The frame-independent part of the pool metadata: identity, format, backing size, slot limits and slot layouts.
void fillPoolLayout(const firebolt::rialto::server::CaptureSessionRuntime &runtime,
                    const firebolt::rialto::capture::CaptureCapabilities &capabilities,
                    const firebolt::rialto::capture::Size &backingSize,
                    firebolt::rialto::GetVideoFrameCapturePoolResponse &metadata)
{
    metadata.set_status(firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK);
    metadata.set_pool_id(runtime.poolId());
    metadata.set_drm_fourcc(capabilities.drmFormat);
    metadata.set_drm_modifier(capabilities.drmModifier);
    metadata.set_texture_width(backingSize.width);
    metadata.set_texture_height(backingSize.height);
    metadata.set_minimum_slots(capabilities.minimumSlots);
    metadata.set_maximum_slots(capabilities.maximumSlots);
    for (const auto &kLayout : runtime.pool().slotLayouts)
    {
        auto *slot = metadata.add_slots();
        slot->set_slot_index(kLayout.slotIndex);
        for (const auto &kPlane : kLayout.planes)
        {
            auto *plane = slot->add_planes();
            plane->set_plane_index(kPlane.planeIndex);
            plane->set_offset_bytes(kPlane.offsetBytes);
            plane->set_length_bytes(kPlane.lengthBytes);
            plane->set_stride_bytes(kPlane.strideBytes);
        }
    }
}

// The pool metadata an acquisition carries when the frame description changed.
void fillPoolMetadata(const firebolt::rialto::server::CaptureSessionRuntime &runtime,
                      const firebolt::rialto::capture::CapturedFrame &frame,
                      firebolt::rialto::GetVideoFrameCapturePoolResponse &metadata)
{
    fillPoolLayout(runtime, frame.capabilities, frame.backingSize, metadata);
    auto *colour = metadata.mutable_color_description();
    colour->set_ycbcr_color_space(
        static_cast<firebolt::rialto::VideoFrameCaptureYcbcrColorSpace>(frame.colorDescription.matrix));
    colour->set_sample_range(static_cast<firebolt::rialto::VideoFrameCaptureSampleRange>(frame.colorDescription.range));
    colour->set_horizontal_chroma_siting(
        static_cast<firebolt::rialto::VideoFrameCaptureChromaSiting>(frame.colorDescription.horizontalSiting));
    colour->set_vertical_chroma_siting(
        static_cast<firebolt::rialto::VideoFrameCaptureChromaSiting>(frame.colorDescription.verticalSiting));
}

void fillFrameReply(const firebolt::rialto::capture::CapturedFrame &frame, bool flushed,
                    firebolt::rialto::AcquireCurrentVideoFrameResponse &response)
{
    response.set_slot_index(frame.slotIndex);
    response.set_acquisition_token(frame.acquisitionToken);
    response.set_presentation_time_ns(frame.presentationTimeNs);
    auto *region = response.mutable_visible_region();
    region->set_x(frame.visibleRegion.x);
    region->set_y(frame.visibleRegion.y);
    region->set_width(frame.visibleRegion.size.width);
    region->set_height(frame.visibleRegion.size.height);
    response.set_dma_buf_objects_flushed(flushed);

    auto *metadata = response.mutable_frame_metadata();
    metadata->set_drm_format(frame.capabilities.drmFormat);
    metadata->set_drm_modifier(frame.capabilities.drmModifier);
    metadata->set_backing_width(frame.backingSize.width);
    metadata->set_backing_height(frame.backingSize.height);
    for (const auto &kObject : frame.dmaBufObjects)
    {
        auto *object = metadata->add_dma_buf_objects();
        object->set_object_id(kObject.objectId);
        object->set_size_bytes(kObject.sizeBytes);
    }
    for (const auto &kPlane : frame.planes)
    {
        auto *plane = metadata->add_planes();
        plane->set_plane_index(kPlane.planeIndex);
        plane->set_dma_buf_object_id(kPlane.dmaBufObjectId);
        plane->set_offset_bytes(kPlane.offsetBytes);
        plane->set_length_bytes(kPlane.lengthBytes);
        plane->set_stride_bytes(kPlane.strideBytes);
    }
    auto *colour = metadata->mutable_color_description();
    colour->set_ycbcr_color_space(
        static_cast<firebolt::rialto::VideoFrameCaptureYcbcrColorSpace>(frame.colorDescription.matrix));
    colour->set_sample_range(static_cast<firebolt::rialto::VideoFrameCaptureSampleRange>(frame.colorDescription.range));
    colour->set_horizontal_chroma_siting(
        static_cast<firebolt::rialto::VideoFrameCaptureChromaSiting>(frame.colorDescription.horizontalSiting));
    colour->set_vertical_chroma_siting(
        static_cast<firebolt::rialto::VideoFrameCaptureChromaSiting>(frame.colorDescription.verticalSiting));

    for (const auto &kExported : frame.exportedDmaBufs)
    {
        auto *exported = response.add_exported_dma_bufs();
        exported->set_dma_buf_object_id(kExported.dmaBufObjectId);
        exported->set_fd(kExported.fd);
    }
}

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

firebolt::rialto::VideoFrameCaptureWireStatus
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

void VideoFrameCaptureModuleService::getCapabilities(
    ::google::protobuf::RpcController *controller,
    const ::firebolt::rialto::GetVideoFrameCaptureCapabilitiesRequest *request,
    ::firebolt::rialto::GetVideoFrameCaptureCapabilitiesResponse *response, ::google::protobuf::Closure *done)
{
    service::VideoFrameCaptureClientId clientId{0};
    if (!clientIdOf(controller, clientId))
    {
        controller->SetFailed("Unknown client");
        done->Run();
        return;
    }
    const std::optional<GraphicsAPIType> kCaptureApi{
        m_videoFrameCaptureService.captureApi(clientId, request->capture_id())};
    if (!kCaptureApi || toGraphicsApi(request->api()) != kCaptureApi)
    {
        response->set_status(VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT); // unknown capture, or another API
        done->Run();
        return;
    }
    std::shared_ptr<CaptureSessionRuntime> runtime{
        m_videoFrameCaptureService.captureRuntime(clientId, request->capture_id())};
    if (!runtime)
    {
        response->set_status(VIDEO_FRAME_CAPTURE_STATUS_SESSION_NOT_READY); // not committed, or retired
        done->Run();
        return;
    }

    capture::CaptureCapabilities capabilities{};
    const capture::CaptureStatus kStatus{runtime->capabilities(capabilities)};
    if (kStatus == capture::CaptureStatus::NO_FRAME)
    {
        response->set_status(VIDEO_FRAME_CAPTURE_STATUS_SESSION_NOT_READY); // retired meanwhile
    }
    else if (kStatus != capture::CaptureStatus::OK)
    {
        response->set_status(toWireStatus(kStatus));
    }
    else
    {
        response->set_status(VIDEO_FRAME_CAPTURE_STATUS_OK);
        response->set_max_concurrent_frames(runtime->slotCount()); // the admitted N
        response->set_max_resources_per_frame(1);
        response->set_supports_protected_content(0);
        response->set_supports_hdr(0);
        response->add_drm_fourccs(capabilities.drmFormat);
        response->add_drm_modifiers(capabilities.drmModifier);
    }
    done->Run();
}

void VideoFrameCaptureModuleService::getPool(::google::protobuf::RpcController *controller,
                                             const ::firebolt::rialto::GetVideoFrameCapturePoolRequest *request,
                                             ::firebolt::rialto::GetVideoFrameCapturePoolResponse *response,
                                             ::google::protobuf::Closure *done)
{
    service::VideoFrameCaptureClientId clientId{0};
    if (!clientIdOf(controller, clientId))
    {
        controller->SetFailed("Unknown client");
        done->Run();
        return;
    }
    if (!m_videoFrameCaptureService.hasCapture(clientId, request->capture_id()))
    {
        response->set_status(VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);
        done->Run();
        return;
    }
    std::shared_ptr<CaptureSessionRuntime> runtime{
        m_videoFrameCaptureService.captureRuntime(clientId, request->capture_id())};
    if (!runtime)
    {
        response->set_status(VIDEO_FRAME_CAPTURE_STATUS_SESSION_NOT_READY); // not committed, or retired
        done->Run();
        return;
    }
    // the admitted layout: no colour (that is per frame) and no descriptors
    fillPoolLayout(*runtime, runtime->admittedCapabilities(), runtime->pool().backingSize, *response);
    done->Run();
}

void VideoFrameCaptureModuleService::acquireCurrentFrame(::google::protobuf::RpcController *controller,
                                                         const ::firebolt::rialto::AcquireCurrentVideoFrameRequest *request,
                                                         ::firebolt::rialto::AcquireCurrentVideoFrameResponse *response,
                                                         ::google::protobuf::Closure *done)
{
    service::VideoFrameCaptureClientId clientId{0};
    if (!clientIdOf(controller, clientId))
    {
        controller->SetFailed("Unknown client");
        done->Run();
        return;
    }
    if (!m_videoFrameCaptureService.hasCapture(clientId, request->capture_id()))
    {
        response->set_status(VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);
        done->Run();
        return;
    }
    std::shared_ptr<CaptureSessionRuntime> runtime{
        m_videoFrameCaptureService.captureRuntime(clientId, request->capture_id())};
    if (!runtime)
    {
        response->set_status(VIDEO_FRAME_CAPTURE_STATUS_NO_FRAME); // not committed: no session yet
        done->Run();
        return;
    }

    CaptureSessionRuntime::DeliveryTicket ticket;
    const capture::CaptureStatus kStatus{runtime->acquire(
        [&runtime, response](const capture::CapturedFrame &frame, bool flushed, bool descriptionChanged)
        {
            fillFrameReply(frame, flushed, *response);
            if (descriptionChanged)
            {
                fillPoolMetadata(*runtime, frame, *response->mutable_pool_metadata());
            }
        },
        ticket)};
    response->set_status(toWireStatus(kStatus));
    if (kStatus == capture::CaptureStatus::OK)
    {
        response->set_pool_id(runtime->poolId());
        // the IPC closes the exported descriptors once the reply is sent, whatever happens (R1)
        if (auto *ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller))
        {
            ipcController->setReplyFdsOwned(true);
            // only the send outcome says whether the client holds the frame; the callback keeps the runtime alive
            ipcController->setDeliveryCallback([runtime, ticket](bool delivered)
                                               { runtime->onDelivery(ticket, delivered); });
        }
        else
        {
            runtime->onDelivery(ticket, false); // no way to learn the outcome: do not leave the lock behind
        }
    }
    done->Run();
}

void VideoFrameCaptureModuleService::releaseFrame(::google::protobuf::RpcController *controller,
                                                  const ::firebolt::rialto::ReleaseVideoFrameRequest *request,
                                                  ::firebolt::rialto::ReleaseVideoFrameResponse *response,
                                                  ::google::protobuf::Closure *done)
{
    service::VideoFrameCaptureClientId clientId{0};
    if (!clientIdOf(controller, clientId))
    {
        controller->SetFailed("Unknown client");
        done->Run();
        return;
    }
    // the frame's session may be retired, or its capture destroyed, by now: releases still reach it (R10 1.3)
    std::shared_ptr<CaptureSessionRuntime> runtime{
        m_videoFrameCaptureService.sessionForRelease(clientId, request->capture_id(), request->pool_id())};
    if (!runtime)
    {
        response->set_status(VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);
        done->Run();
        return;
    }
    response->set_status(toWireStatus(runtime->release(request->slot_index(), request->acquisition_token())));
    done->Run();
}
} // namespace firebolt::rialto::server::ipc
