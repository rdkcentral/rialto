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

#ifndef FIREBOLT_RIALTO_CLIENT_VULKAN_FRAME_IMPORTER_H_
#define FIREBOLT_RIALTO_CLIENT_VULKAN_FRAME_IMPORTER_H_

#include "IFrameImporter.h"
#include "VulkanDeviceContext.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace firebolt::rialto::client
{
/**
 * @brief One frame layout imported as a Vulkan image (SoC spec 6.5), with its view, sampler and conversion.
 *
 * Destroying it makes no Vulkan call: the importer destroys its objects.
 */
class VulkanImportedImage : public IImportedImage
{
public:
    std::uint32_t textureTarget() const override { return 0; }
    const std::vector<std::uint32_t> &textureIds() const override { return m_noTextures; }

    VkFormat format{VK_FORMAT_UNDEFINED};
    VkFormatFeatureFlags features{0}; /**< The modifier's tiling features for the format. */
    VkImage image{VK_NULL_HANDLE};
    VkImageView view{VK_NULL_HANDLE};
    VkSampler sampler{VK_NULL_HANDLE};
    VkSamplerYcbcrConversion conversion{VK_NULL_HANDLE};
    std::vector<VkDeviceMemory> memories;

private:
    const std::vector<std::uint32_t> m_noTextures;
};

/**
 * @brief The Vulkan format for a DRM fourcc, or nothing if Vulkan capture cannot sample it.
 */
std::optional<VkFormat> vulkanFormatForDrmFormat(std::uint32_t drmFormat);

/**
 * @brief Imports captured frames as Vulkan images (R13 design D2).
 */
class VulkanFrameImporter : public IFrameImporter
{
public:
    explicit VulkanFrameImporter(const std::shared_ptr<VulkanDeviceContext> &context);
    ~VulkanFrameImporter() override;

    /**
     * @brief Imports a frame. Descriptors are always handled here: each is consumed by a successful memory import
     *        or closed, so fdsConsumed is true. On failure everything this attempt created is destroyed in reverse
     *        order.
     */
    std::shared_ptr<IImportedImage> importFrame(const CaptureFrameDescription &description,
                                                const std::vector<CaptureObjectFd> &fds, bool &fdsConsumed) override;
    void retire(const std::shared_ptr<IImportedImage> &image) override;
    void drain() override;

private:
    /**
     * @brief Creates the image, imports and binds its memory, and creates the sampler and view.
     *
     * @param[in] importFds : One descriptor per memory import, owned; each is consumed or closed here.
     */
    bool createImport(const CaptureFrameDescription &description, bool disjoint, std::vector<int> &importFds,
                      VulkanImportedImage &import);
    /** Imports one descriptor into memory for the image (or one plane of a disjoint image). Consumes or closes fd. */
    bool importMemory(VulkanImportedImage &import, bool disjoint, VkImageAspectFlagBits planeAspect, int &fd);
    /** Creates the frame's conversion (YUV only), sampler and view. */
    bool createSamplerAndView(const CaptureFrameDescription &description, VulkanImportedImage &import);
    /** Destroys an import's objects: view, sampler, conversion, image, then memory. */
    void destroyImport(VulkanImportedImage &image);

    std::shared_ptr<VulkanDeviceContext> m_context;
    std::mutex m_retiredMutex;
    std::vector<std::shared_ptr<VulkanImportedImage>> m_retired;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_VULKAN_FRAME_IMPORTER_H_
