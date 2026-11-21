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

#ifndef FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_CAPTURE_FACTORY_H_
#define FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_CAPTURE_FACTORY_H_

#include "EglLibrary.h"
#include "IVideoFrameCapture.h"
#include "IVideoFrameCaptureIpc.h"

#include <memory>

namespace firebolt::rialto::client
{
class IVulkanLibrary;

/**
 * @brief Client implementation of IVideoFrameCaptureFactory.
 *
 * A GL capture is created by loading libEGL at run time and then registering the capture with the server.
 */
class VideoFrameCaptureFactory : public IVideoFrameCaptureFactory
{
public:
    /**
     * @param[in] vulkanLibrary : The Vulkan loader; null when Vulkan capture is not built in.
     */
    VideoFrameCaptureFactory(const std::shared_ptr<IEglLibrary> &eglLibrary,
                             const std::shared_ptr<IVideoFrameCaptureIpcFactory> &videoFrameCaptureIpcFactory,
                             const std::shared_ptr<IVulkanLibrary> &vulkanLibrary = nullptr);
    ~VideoFrameCaptureFactory() override = default;

    std::shared_ptr<IGLVideoFrameCapture> createGLVideoFrameCapture(EGLDisplay eglDisplay, EGLContext eglContext) override;
    std::shared_ptr<IVulkanVideoFrameCapture> createVulkanVideoFrameCapture(VkPhysicalDevice physicalDevice,
                                                                            VkDevice device, VkQueue queue,
                                                                            uint32_t queueFamilyIndex,
                                                                            int32_t requiredImageLayout) override;

private:
    std::shared_ptr<IEglLibrary> m_eglLibrary;
    std::shared_ptr<IVideoFrameCaptureIpcFactory> m_videoFrameCaptureIpcFactory;
    std::shared_ptr<IVulkanLibrary> m_vulkanLibrary;
};

} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_CAPTURE_FACTORY_H_
