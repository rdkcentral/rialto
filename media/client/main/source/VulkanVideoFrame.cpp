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

#include "VulkanVideoFrame.h"
#include "VulkanFrameImporter.h"

#include <utility>

namespace firebolt::rialto::client
{
VulkanVideoFrame::VulkanVideoFrame(const CaptureFrameDescription &description,
                                   const std::shared_ptr<ImportCacheEntry> &import, std::unique_ptr<FrameLease> lease,
                                   VkImageLayout imageLayout)
    : m_description{description}, m_import{import}, m_lease{std::move(lease)}, m_imageLayout{imageLayout}
{
    auto vulkanImage = m_import ? std::dynamic_pointer_cast<VulkanImportedImage>(m_import->image()) : nullptr;
    if (vulkanImage)
    {
        m_imageFormat = static_cast<std::int32_t>(vulkanImage->format);
        m_images.push_back(
            VulkanImageResource{vulkanImage->image, vulkanImage->view, vulkanImage->sampler, vulkanImage->conversion});
    }
}
} // namespace firebolt::rialto::client
