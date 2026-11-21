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

#ifndef FIREBOLT_RIALTO_CLIENT_VULKAN_VIDEO_FRAME_H_
#define FIREBOLT_RIALTO_CLIENT_VULKAN_VIDEO_FRAME_H_

#include "FrameLease.h"
#include "IVideoFrameCapture.h"
#include "IVideoFrameCaptureIpc.h"
#include "IVulkanApi.h"
#include "ImportCache.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace firebolt::rialto::client
{
/**
 * @brief A Vulkan frame returned to the application. As for GL frames, holding a reference holds the capture slot.
 */
class VulkanVideoFrame : public IVulkanVideoFrame
{
public:
    /**
     * @param[in] imageLayout : The capture's requiredImageLayout; Rialto's acquire barrier leaves the image in it.
     */
    VulkanVideoFrame(const CaptureFrameDescription &description, const std::shared_ptr<ImportCacheEntry> &import,
                     std::unique_ptr<FrameLease> lease, VkImageLayout imageLayout);
    ~VulkanVideoFrame() override = default;

    std::uint32_t frameIndex() const override { return m_description.slotIndex; }
    std::uint32_t numResources() const override { return static_cast<std::uint32_t>(m_images.size()); }
    GraphicsTextureLayout layout() const override { return GraphicsTextureLayout::RGBA; }
    std::int64_t presentationTimeStampNs() const override { return m_description.presentationTimeNs; }
    std::uint32_t textureWidth() const override { return m_description.backingWidth; }
    std::uint32_t textureHeight() const override { return m_description.backingHeight; }
    std::uint32_t visibleWidth() const override { return m_description.visibleWidth; }
    std::uint32_t visibleHeight() const override { return m_description.visibleHeight; }

    std::int32_t imageFormat() const override { return m_imageFormat; }
    std::int32_t imageLayout() const override { return static_cast<std::int32_t>(m_imageLayout); }
    const std::vector<VulkanImageResource> &images() const override { return m_images; }

    /** The description the server sent for this frame. */
    const CaptureFrameDescription &description() const { return m_description; }
    /** The import, for the capture's barriers. */
    const std::shared_ptr<ImportCacheEntry> &import() const { return m_import; }

private:
    const CaptureFrameDescription m_description;
    std::shared_ptr<ImportCacheEntry> m_import; /**< Keeps the import alive while the frame exists. */
    std::unique_ptr<FrameLease> m_lease;        /**< Released when the last reference to the frame goes. */
    const VkImageLayout m_imageLayout;
    std::int32_t m_imageFormat{0};
    std::vector<VulkanImageResource> m_images;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_VULKAN_VIDEO_FRAME_H_
