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

#include "VideoFrameCaptureFactory.h"
#include "GLVideoFrameCapture.h"
#include "GlFrameImporter.h"
#ifdef RIALTO_ENABLE_VULKAN_CAPTURE
#include "VulkanDeviceContext.h"
#include "VulkanFrameImporter.h"
#include "VulkanLibrary.h"
#include "VulkanVideoFrame.h"
#include "VulkanVideoFrameCapture.h"
#endif
#include "RialtoClientLogging.h"

#include <exception>

namespace firebolt::rialto
{
std::shared_ptr<IVideoFrameCaptureFactory> IVideoFrameCaptureFactory::createFactory()
{
    std::shared_ptr<IVideoFrameCaptureFactory> factory;

    try
    {
#ifdef RIALTO_ENABLE_VULKAN_CAPTURE
        const std::shared_ptr<client::IVulkanLibrary> kVulkanLibrary{client::IVulkanLibrary::instance()};
#else
        const std::shared_ptr<client::IVulkanLibrary> kVulkanLibrary;
#endif
        factory =
            std::make_shared<client::VideoFrameCaptureFactory>(client::IEglLibrary::instance(),
                                                               client::IVideoFrameCaptureIpcFactory::createFactory(),
                                                               kVulkanLibrary);
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the video frame capture factory, reason: %s", e.what());
    }

    return factory;
}
} // namespace firebolt::rialto

namespace firebolt::rialto::client
{
VideoFrameCaptureFactory::VideoFrameCaptureFactory(
    const std::shared_ptr<IEglLibrary> &eglLibrary,
    const std::shared_ptr<IVideoFrameCaptureIpcFactory> &videoFrameCaptureIpcFactory,
    const std::shared_ptr<IVulkanLibrary> &vulkanLibrary)
    : m_eglLibrary{eglLibrary}, m_videoFrameCaptureIpcFactory{videoFrameCaptureIpcFactory},
      m_vulkanLibrary{vulkanLibrary}
{
}

std::shared_ptr<IGLVideoFrameCapture> VideoFrameCaptureFactory::createGLVideoFrameCapture(EGLDisplay eglDisplay,
                                                                                          EGLContext eglContext)
{
    // load EGL first, so a client that cannot import frames never registers a capture with the server
    std::shared_ptr<IEglGl> gl = m_eglLibrary ? m_eglLibrary->functions() : nullptr;
    if (!gl)
    {
        RIALTO_CLIENT_LOG_WARN("Video frame capture is unavailable: cannot load EGL");
        return nullptr;
    }

    std::shared_ptr<IVideoFrameCaptureIpc> videoFrameCaptureIpc =
        m_videoFrameCaptureIpcFactory ? m_videoFrameCaptureIpcFactory->createVideoFrameCaptureIpc() : nullptr;
    if (!videoFrameCaptureIpc)
    {
        RIALTO_CLIENT_LOG_WARN("Video frame capture is unavailable: no connection to the server");
        return nullptr;
    }

    std::uint32_t captureId{0};
    if (!videoFrameCaptureIpc->createVideoFrameCapture(GraphicsAPIType::GL, captureId))
    {
        RIALTO_CLIENT_LOG_WARN("Video frame capture is unavailable: the server did not register it");
        return nullptr;
    }

    return std::make_shared<GLVideoFrameCapture>(videoFrameCaptureIpc, captureId, eglDisplay, eglContext,
                                                 std::make_shared<GlFrameImporter>(gl, eglDisplay, eglContext));
}

std::shared_ptr<IVulkanVideoFrameCapture>
VideoFrameCaptureFactory::createVulkanVideoFrameCapture(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue,
                                                        uint32_t queueFamilyIndex, int32_t requiredImageLayout)
{
#ifdef RIALTO_ENABLE_VULKAN_CAPTURE
    // load Vulkan and check the device first, so nothing is registered with the server for a device that cannot
    // import frames (R13 2.1)
    std::shared_ptr<IVulkanApi> api = m_vulkanLibrary ? m_vulkanLibrary->deviceApi(device) : nullptr;
    if (!api)
    {
        RIALTO_CLIENT_LOG_WARN("Vulkan video frame capture is unavailable: cannot load Vulkan or a command is missing");
        return nullptr;
    }
    if (!isVulkanDeviceSupported(*api, physicalDevice))
    {
        return nullptr;
    }
    std::shared_ptr<VulkanDeviceContext> context{
        VulkanDeviceContext::create(api, physicalDevice, device, queue, queueFamilyIndex,
                                    static_cast<VkImageLayout>(requiredImageLayout))};
    if (!context)
    {
        return nullptr;
    }

    std::shared_ptr<IVideoFrameCaptureIpc> videoFrameCaptureIpc =
        m_videoFrameCaptureIpcFactory ? m_videoFrameCaptureIpcFactory->createVideoFrameCaptureIpc() : nullptr;
    if (!videoFrameCaptureIpc)
    {
        RIALTO_CLIENT_LOG_WARN("Video frame capture is unavailable: no connection to the server");
        return nullptr;
    }
    std::uint32_t captureId{0};
    if (!videoFrameCaptureIpc->createVideoFrameCapture(GraphicsAPIType::VULKAN, captureId))
    {
        RIALTO_CLIENT_LOG_WARN("Video frame capture is unavailable: the server did not register it");
        return nullptr;
    }
    const VkImageLayout kImageLayout{context->requiredImageLayout()};
    CaptureCore::FrameMaker frameMaker{
        [kImageLayout](const CaptureFrameDescription &description, const std::shared_ptr<ImportCacheEntry> &import,
                       std::unique_ptr<FrameLease> lease) -> std::shared_ptr<IVideoFrame>
        { return std::make_shared<VulkanVideoFrame>(description, import, std::move(lease), kImageLayout); }};
    return std::make_shared<VulkanVideoFrameCapture>(videoFrameCaptureIpc, captureId, context,
                                                     std::make_shared<VulkanFrameImporter>(context), frameMaker);
#else
    (void)physicalDevice;
    (void)device;
    (void)queue;
    (void)queueFamilyIndex;
    (void)requiredImageLayout;
    RIALTO_CLIENT_LOG_WARN("Vulkan video frame capture is not built in");
    return nullptr;
#endif
}
} // namespace firebolt::rialto::client
