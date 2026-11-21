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

#ifndef FIREBOLT_RIALTO_CLIENT_VULKAN_VIDEO_FRAME_CAPTURE_H_
#define FIREBOLT_RIALTO_CLIENT_VULKAN_VIDEO_FRAME_CAPTURE_H_

#include "CaptureCore.h"
#include "IAttachableVideoFrameCapture.h"
#include "IFrameImporter.h"
#include "IVideoFrameCapture.h"
#include "IVideoFrameCaptureIpc.h"
#include "VulkanDeviceContext.h"

#include <atomic>
#include <cstdint>
#include <memory>

namespace firebolt::rialto::client
{
/**
 * @brief A Vulkan capture registered with the server. Destroying it removes the registration.
 */
class VulkanVideoFrameCapture : public IVulkanVideoFrameCapture, public IAttachableVideoFrameCapture
{
public:
    VulkanVideoFrameCapture(const std::shared_ptr<IVideoFrameCaptureIpc> &videoFrameCaptureIpc, std::uint32_t captureId,
                            const std::shared_ptr<VulkanDeviceContext> &context,
                            const std::shared_ptr<IFrameImporter> &importer,
                            const CaptureCore::FrameMaker &frameMaker = nullptr);
    ~VulkanVideoFrameCapture() override;

    GraphicsAPIType apiType() const override { return GraphicsAPIType::VULKAN; }
    VulkanFrameAcquisition acquireCurrentFrame() override;

    std::uint32_t captureId() const override { return m_captureId; }
    bool markAttached() override { return !m_attached.exchange(true); }
    void clearAttached() override { m_attached = false; }

    /** The device context, for tests. */
    const std::shared_ptr<VulkanDeviceContext> &context() const { return m_context; }

private:
    std::shared_ptr<VulkanDeviceContext> m_context;
    std::shared_ptr<CaptureCore> m_core;
    std::shared_ptr<IVideoFrameCaptureIpc> m_videoFrameCaptureIpc;
    const std::uint32_t m_captureId;
    std::atomic<bool> m_attached{false};
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_VULKAN_VIDEO_FRAME_CAPTURE_H_
