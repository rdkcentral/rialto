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
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <vector>

namespace firebolt::rialto::client
{
/**
 * @brief The key of a cached sampler and conversion (R13 design D3).
 */
struct VulkanSamplerKey
{
    VkFormat format{VK_FORMAT_UNDEFINED};
    std::uint64_t modifier{0};
    std::uint32_t matrix{0};
    std::uint32_t range{0};
    std::uint32_t horizontalSiting{0};
    std::uint32_t verticalSiting{0};

    bool operator<(const VulkanSamplerKey &other) const
    {
        return std::tie(format, modifier, matrix, range, horizontalSiting, verticalSiting) <
               std::tie(other.format, other.modifier, other.matrix, other.range, other.horizontalSiting,
                        other.verticalSiting);
    }
    bool operator==(const VulkanSamplerKey &other) const { return !(*this < other) && !(other < *this); }
};

/**
 * @brief A cached sampler and its conversion (null for RGB). Imports hold it; the importer destroys it.
 */
struct VulkanSamplerEntry
{
    VkSampler sampler{VK_NULL_HANDLE};
    VkSamplerYcbcrConversion conversion{VK_NULL_HANDLE};
};

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
    std::shared_ptr<VulkanSamplerEntry> samplerEntry; /**< Keeps the sampler and conversion alive. */

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
    /** Gets the frame's sampler and conversion from the cache, or creates them, and creates the view. */
    bool createSamplerAndView(const CaptureFrameDescription &description, VulkanImportedImage &import);
    /** Creates a sampler and its conversion (YUV only). @retval null on failure, with nothing left created. */
    std::shared_ptr<VulkanSamplerEntry> createSamplerEntry(const CaptureFrameDescription &description,
                                                           const VulkanImportedImage &import);
    void destroySamplerEntry(const VulkanSamplerEntry &entry);
    /**
     * @brief Destroys cached samplers that no import holds, except the most recent description's, which stays
     *        so that frames after a flush keep the same handles.
     */
    void sweepSamplers(bool all);
    /** Destroys an import's view, image and memory, and drops its sampler reference. */
    void destroyImport(VulkanImportedImage &image);

    std::shared_ptr<VulkanDeviceContext> m_context;
    std::mutex m_retiredMutex;
    std::vector<std::shared_ptr<VulkanImportedImage>> m_retired;
    std::map<VulkanSamplerKey, std::shared_ptr<VulkanSamplerEntry>> m_samplers;
    std::optional<VulkanSamplerKey> m_lastSamplerKey;

public:
    /** Number of cached samplers, for tests. */
    std::size_t samplerCount() const { return m_samplers.size(); }
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_VULKAN_FRAME_IMPORTER_H_
