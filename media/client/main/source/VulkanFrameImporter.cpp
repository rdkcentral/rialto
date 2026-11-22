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

#include "VulkanFrameImporter.h"
#include "RialtoClientLogging.h"

#include <unistd.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <utility>

namespace firebolt::rialto::client
{
namespace
{
constexpr std::uint32_t fourcc(char a, char b, char c, char d)
{
    return static_cast<std::uint32_t>(a) | (static_cast<std::uint32_t>(b) << 8) |
           (static_cast<std::uint32_t>(c) << 16) | (static_cast<std::uint32_t>(d) << 24);
}

bool isYcbcr(VkFormat format)
{
    return format == VK_FORMAT_G8_B8R8_2PLANE_420_UNORM || format == VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM ||
           format == VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
}

// Wire values (proto VideoFrameCapture*): 0 is UNKNOWN in each enum.
VkSamplerYcbcrModelConversion ycbcrModel(std::uint32_t matrix)
{
    switch (matrix)
    {
    case 1:
        return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601;
    case 3:
        return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020;
    default:
        return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
    }
}

VkSamplerYcbcrRange ycbcrRange(std::uint32_t range)
{
    return range == 1 ? VK_SAMPLER_YCBCR_RANGE_ITU_FULL : VK_SAMPLER_YCBCR_RANGE_ITU_NARROW;
}

VkChromaLocation chromaLocation(std::uint32_t siting, VkChromaLocation unknown)
{
    switch (siting)
    {
    case 1:
        return VK_CHROMA_LOCATION_COSITED_EVEN;
    case 2:
        return VK_CHROMA_LOCATION_MIDPOINT;
    default:
        return unknown;
    }
}

/** The modifier's properties for a format, if the device can sample that format with that modifier. */
std::optional<VkDrmFormatModifierPropertiesEXT> modifierProperties(IVulkanApi &api, VkPhysicalDevice physicalDevice,
                                                                   VkFormat format, std::uint64_t modifier)
{
    VkDrmFormatModifierPropertiesListEXT list{VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT};
    VkFormatProperties2 properties{VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2, &list};
    api.getPhysicalDeviceFormatProperties2(physicalDevice, format, &properties);
    if (list.drmFormatModifierCount == 0)
    {
        return std::nullopt;
    }
    std::vector<VkDrmFormatModifierPropertiesEXT> modifiers(list.drmFormatModifierCount);
    list.pDrmFormatModifierProperties = modifiers.data();
    api.getPhysicalDeviceFormatProperties2(physicalDevice, format, &properties);
    modifiers.resize(std::min<std::size_t>(modifiers.size(), list.drmFormatModifierCount));
    for (const VkDrmFormatModifierPropertiesEXT &kProperties : modifiers)
    {
        if (kProperties.drmFormatModifier == modifier &&
            (kProperties.drmFormatModifierTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
        {
            return kProperties;
        }
    }
    return std::nullopt;
}

bool imageFormatSupported(IVulkanApi &api, VkPhysicalDevice physicalDevice, VkFormat format, std::uint64_t modifier,
                          VkImageCreateFlags flags)
{
    VkPhysicalDeviceImageDrmFormatModifierInfoEXT modifierInfo{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT};
    modifierInfo.drmFormatModifier = modifier;
    modifierInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkPhysicalDeviceExternalImageFormatInfo externalInfo{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO,
                                                         &modifierInfo};
    externalInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    VkPhysicalDeviceImageFormatInfo2 info{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, &externalInfo};
    info.format = format;
    info.type = VK_IMAGE_TYPE_2D;
    info.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
    info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    info.flags = flags;
    VkImageFormatProperties2 properties{VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2};
    return api.getPhysicalDeviceImageFormatProperties2(physicalDevice, &info, &properties) == VK_SUCCESS;
}

/** Closes a descriptor that no import took. */
void closeFd(int &fd)
{
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
}

void closeFds(std::vector<int> &fds)
{
    for (int &fd : fds)
    {
        closeFd(fd);
    }
}

constexpr VkImageAspectFlagBits kPlaneAspects[]{VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT,
                                                VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT,
                                                VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT,
                                                VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT};
} // namespace

std::optional<VkFormat> vulkanFormatForDrmFormat(std::uint32_t drmFormat)
{
    switch (drmFormat)
    {
    case fourcc('N', 'V', '1', '2'):
        return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    case fourcc('Y', 'U', '1', '2'):
        return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
    case fourcc('P', '0', '1', '0'):
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
    case fourcc('A', 'R', '2', '4'): // DRM ARGB8888 is B, G, R, A in memory
    case fourcc('X', 'R', '2', '4'):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case fourcc('A', 'B', '2', '4'): // DRM ABGR8888 is R, G, B, A in memory
    case fourcc('X', 'B', '2', '4'):
        return VK_FORMAT_R8G8B8A8_UNORM;
    default:
        return std::nullopt;
    }
}

VulkanFrameImporter::VulkanFrameImporter(const std::shared_ptr<VulkanDeviceContext> &context)
    : m_context{context}, m_inFlight(context->ring().size())
{
}

VulkanFrameImporter::~VulkanFrameImporter()
{
    // design D6: wait for Rialto's own submissions, destroy every object, then send the releases still held.
    // Frames released after the last acquisition get no release barrier: their images are destroyed here.
    std::vector<PendingRelease> releases;
    for (std::size_t i = 0; i < m_inFlight.size(); ++i)
    {
        if (m_inFlight[i].busy)
        {
            VkFence fence{m_context->ring()[i].fence};
            if (fence != VK_NULL_HANDLE)
                m_context->api().waitForFences(m_context->device(), 1, &fence, VK_TRUE, UINT64_MAX);
            m_inFlight[i].busy = false;
            std::move(m_inFlight[i].releases.begin(), m_inFlight[i].releases.end(), std::back_inserter(releases));
            m_inFlight[i].releases.clear();
        }
    }
    std::vector<std::shared_ptr<VulkanImportedImage>> imports;
    {
        std::lock_guard<std::mutex> lock{m_retiredMutex};
        std::move(m_pendingReleases.begin(), m_pendingReleases.end(), std::back_inserter(releases));
        m_pendingReleases.clear();
        imports.swap(m_retired); // every import: frames and cache entries, which retire them, are gone
    }

    // SoC spec 6.5 order across all imports: views, samplers and conversions, images, then memory
    IVulkanApi &api{m_context->api()};
    const VkDevice kDevice{m_context->device()};
    for (const auto &kImport : imports)
    {
        if (kImport->view != VK_NULL_HANDLE)
            api.destroyImageView(kDevice, kImport->view);
        kImport->view = VK_NULL_HANDLE;
        kImport->samplerEntry.reset();
    }
    sweepSamplers(true);
    for (const auto &kImport : imports)
    {
        if (kImport->image != VK_NULL_HANDLE)
            api.destroyImage(kDevice, kImport->image);
        kImport->image = VK_NULL_HANDLE;
    }
    for (const auto &kImport : imports)
    {
        destroyImport(*kImport); // frees the memory; everything else is already gone
    }

    for (PendingRelease &release : releases)
    {
        release.sendRelease();
    }
}

std::shared_ptr<IImportedImage> VulkanFrameImporter::importFrame(const CaptureFrameDescription &description,
                                                                 const std::vector<CaptureObjectFd> &fds,
                                                                 bool &fdsConsumed)
{
    // every received descriptor is consumed by a memory import or closed here
    fdsConsumed = true;
    std::map<std::uint64_t, int> received;
    for (const CaptureObjectFd &kFd : fds)
    {
        int fd{kFd.fd};
        if (!received.emplace(kFd.objectId, fd).second)
        {
            closeFd(fd); // a second descriptor for the same object
        }
    }

    // one memory import for a non-disjoint image; one per plane when the planes are in more than one object
    bool disjoint{false};
    for (const CapturePlane &kPlane : description.planes)
    {
        disjoint = disjoint || kPlane.objectId != description.planes.front().objectId;
    }
    std::vector<std::uint64_t> importObjects;
    for (const CapturePlane &kPlane : description.planes)
    {
        importObjects.push_back(kPlane.objectId);
        if (!disjoint)
            break;
    }

    // give each import its own descriptor: the received one first, duplicates for further imports of the same
    // object. Duplicates are made now, before any import hands the received descriptor to Vulkan.
    std::vector<int> importFds;
    std::map<std::uint64_t, std::size_t> uses;
    bool ok{true};
    for (std::uint64_t objectId : importObjects)
    {
        auto it = received.find(objectId);
        if (it == received.end())
        {
            RIALTO_CLIENT_LOG_ERROR("No descriptor for object %llu", static_cast<unsigned long long>(objectId));
            ok = false;
            break;
        }
        if (uses[objectId]++ == 0)
        {
            importFds.push_back(it->second);
            continue;
        }
        const int kDuplicate{dup(it->second)};
        if (kDuplicate < 0)
        {
            RIALTO_CLIENT_LOG_ERROR("Cannot duplicate the descriptor of object %llu",
                                    static_cast<unsigned long long>(objectId));
            ok = false;
            break;
        }
        importFds.push_back(kDuplicate);
    }
    // received descriptors are either in importFds now or used by no plane
    for (auto &[objectId, fd] : received)
    {
        if (uses.count(objectId) == 0)
            closeFd(fd);
    }
    if (!ok)
    {
        closeFds(importFds); // holds the received descriptor of every object used so far
        return nullptr;
    }

    auto image = std::make_shared<VulkanImportedImage>();
    if (!createImport(description, disjoint, importFds, *image))
    {
        destroyImport(*image);
        closeFds(importFds); // descriptors no import reached
        return nullptr;
    }
    return image;
}

bool VulkanFrameImporter::createImport(const CaptureFrameDescription &description, bool disjoint,
                                       std::vector<int> &importFds, VulkanImportedImage &import)
{
    IVulkanApi &api{m_context->api()};
    const VkDevice kDevice{m_context->device()};
    const VkPhysicalDevice kPhysicalDevice{m_context->physicalDevice()};

    const std::optional<VkFormat> kFormat{vulkanFormatForDrmFormat(description.drmFormat)};
    if (!kFormat || description.planes.empty() || description.planes.size() > std::size(kPlaneAspects))
    {
        RIALTO_CLIENT_LOG_ERROR("Vulkan capture cannot import DRM format 0x%08x with %zu planes", description.drmFormat,
                                description.planes.size());
        return false;
    }
    import.format = *kFormat;

    const std::optional<VkDrmFormatModifierPropertiesEXT> kModifier{
        modifierProperties(api, kPhysicalDevice, *kFormat, description.drmModifier)};
    if (!kModifier || kModifier->drmFormatModifierPlaneCount != description.planes.size())
    {
        RIALTO_CLIENT_LOG_ERROR("The Vulkan device cannot sample format 0x%08x with modifier 0x%llx and %zu planes",
                                description.drmFormat, static_cast<unsigned long long>(description.drmModifier),
                                description.planes.size());
        return false;
    }
    import.features = kModifier->drmFormatModifierTilingFeatures;

    const VkImageCreateFlags kFlags{disjoint ? static_cast<VkImageCreateFlags>(VK_IMAGE_CREATE_DISJOINT_BIT) : 0U};
    if (!imageFormatSupported(api, kPhysicalDevice, *kFormat, description.drmModifier, kFlags))
    {
        RIALTO_CLIENT_LOG_ERROR("The Vulkan device cannot import format 0x%08x with modifier 0x%llx from DMA-BUF",
                                description.drmFormat, static_cast<unsigned long long>(description.drmModifier));
        return false;
    }

    // the image, with the frame's plane layouts; each import is bound at offset 0 of its object
    std::vector<VkSubresourceLayout> layouts;
    for (const CapturePlane &kPlane : description.planes)
    {
        VkSubresourceLayout layout{};
        layout.offset = kPlane.offsetBytes;
        layout.size = kPlane.lengthBytes;
        layout.rowPitch = kPlane.strideBytes;
        layouts.push_back(layout);
    }
    VkImageDrmFormatModifierExplicitCreateInfoEXT modifierInfo{
        VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT};
    modifierInfo.drmFormatModifier = description.drmModifier;
    modifierInfo.drmFormatModifierPlaneCount = static_cast<std::uint32_t>(layouts.size());
    modifierInfo.pPlaneLayouts = layouts.data();
    VkExternalMemoryImageCreateInfo externalInfo{VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, &modifierInfo};
    externalInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, &externalInfo};
    imageInfo.flags = kFlags;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = *kFormat;
    imageInfo.extent = {description.backingWidth, description.backingHeight, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (api.createImage(kDevice, &imageInfo, &import.image) != VK_SUCCESS)
    {
        import.image = VK_NULL_HANDLE;
        RIALTO_CLIENT_LOG_ERROR("Cannot create the Vulkan image for a captured frame");
        return false;
    }

    std::vector<VkBindImagePlaneMemoryInfo> planeInfos(importFds.size(),
                                                       {VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO});
    std::vector<VkBindImageMemoryInfo> bindInfos(importFds.size(), {VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO});
    for (std::size_t i = 0; i < importFds.size(); ++i)
    {
        if (!importMemory(import, disjoint, kPlaneAspects[i], importFds[i]))
        {
            return false;
        }
        bindInfos[i].image = import.image;
        bindInfos[i].memory = import.memories.back();
        if (disjoint)
        {
            planeInfos[i].planeAspect = kPlaneAspects[i];
            bindInfos[i].pNext = &planeInfos[i];
        }
    }
    if (api.bindImageMemory2(kDevice, static_cast<std::uint32_t>(bindInfos.size()), bindInfos.data()) != VK_SUCCESS)
    {
        RIALTO_CLIENT_LOG_ERROR("Cannot bind memory to the Vulkan image for a captured frame");
        return false;
    }

    return createSamplerAndView(description, import);
}

bool VulkanFrameImporter::importMemory(VulkanImportedImage &import, bool disjoint, VkImageAspectFlagBits planeAspect,
                                       int &fd)
{
    IVulkanApi &api{m_context->api()};
    const VkDevice kDevice{m_context->device()};

    VkMemoryFdPropertiesKHR fdProperties{VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR};
    if (api.getMemoryFdPropertiesKHR(kDevice, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, fd, &fdProperties) !=
        VK_SUCCESS)
    {
        RIALTO_CLIENT_LOG_ERROR("Cannot query the Vulkan memory types of a DMA-BUF");
        closeFd(fd);
        return false;
    }
    VkImagePlaneMemoryRequirementsInfo planeRequirements{VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO};
    planeRequirements.planeAspect = planeAspect;
    VkImageMemoryRequirementsInfo2 requirementsInfo{VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
                                                    disjoint ? &planeRequirements : nullptr};
    requirementsInfo.image = import.image;
    VkMemoryRequirements2 requirements{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    api.getImageMemoryRequirements2(kDevice, &requirementsInfo, &requirements);

    const std::uint32_t kTypes{fdProperties.memoryTypeBits & requirements.memoryRequirements.memoryTypeBits};
    if (kTypes == 0)
    {
        RIALTO_CLIENT_LOG_ERROR("No Vulkan memory type can import this DMA-BUF for the image");
        closeFd(fd);
        return false;
    }
    std::uint32_t typeIndex{0};
    while (!(kTypes & (1U << typeIndex)))
    {
        ++typeIndex;
    }

    // drivers commonly expect a dedicated allocation for an imported image; a disjoint image cannot have one
    VkMemoryDedicatedAllocateInfo dedicated{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    dedicated.image = import.image;
    VkImportMemoryFdInfoKHR importInfo{VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR, disjoint ? nullptr : &dedicated};
    importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    importInfo.fd = fd;
    VkMemoryAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &importInfo};
    allocateInfo.allocationSize = requirements.memoryRequirements.size;
    allocateInfo.memoryTypeIndex = typeIndex;
    VkDeviceMemory memory{VK_NULL_HANDLE};
    if (api.allocateMemory(kDevice, &allocateInfo, &memory) != VK_SUCCESS)
    {
        RIALTO_CLIENT_LOG_ERROR("Cannot import a DMA-BUF into Vulkan memory");
        closeFd(fd); // a failed import does not take the descriptor
        return false;
    }
    fd = -1; // owned by Vulkan now
    import.memories.push_back(memory);
    return true;
}

bool VulkanFrameImporter::createSamplerAndView(const CaptureFrameDescription &description, VulkanImportedImage &import)
{
    IVulkanApi &api{m_context->api()};
    const VkDevice kDevice{m_context->device()};

    const VulkanSamplerKey kKey{import.format,
                                description.drmModifier,
                                description.colour.matrix,
                                description.colour.range,
                                description.colour.horizontalSiting,
                                description.colour.verticalSiting};
    bool createdNow{false};
    auto it = m_samplers.find(kKey);
    if (it == m_samplers.end())
    {
        std::shared_ptr<VulkanSamplerEntry> entry{createSamplerEntry(description, import)};
        if (!entry)
        {
            return false;
        }
        it = m_samplers.emplace(kKey, entry).first;
        createdNow = true;
    }
    import.samplerEntry = it->second;
    import.sampler = it->second->sampler;
    import.conversion = it->second->conversion;

    VkSamplerYcbcrConversionInfo conversionInfo{VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO};
    conversionInfo.conversion = import.conversion;
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                   import.conversion != VK_NULL_HANDLE ? &conversionInfo : nullptr};
    viewInfo.image = import.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = import.format;
    viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                           VK_COMPONENT_SWIZZLE_IDENTITY};
    viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (api.createImageView(kDevice, &viewInfo, &import.view) != VK_SUCCESS)
    {
        import.view = VK_NULL_HANDLE;
        RIALTO_CLIENT_LOG_ERROR("Cannot create the Vulkan image view for a captured frame");
        if (createdNow)
        {
            // created by this attempt: destroyed with it
            import.samplerEntry.reset();
            destroySamplerEntry(*it->second);
            m_samplers.erase(it);
        }
        return false;
    }
    m_lastSamplerKey = kKey;
    return true;
}

std::shared_ptr<VulkanSamplerEntry> VulkanFrameImporter::createSamplerEntry(const CaptureFrameDescription &description,
                                                                            const VulkanImportedImage &import)
{
    IVulkanApi &api{m_context->api()};
    const VkDevice kDevice{m_context->device()};
    const bool kYcbcr{isYcbcr(import.format)};
    const bool kLinear{(import.features & (kYcbcr ? VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT
                                                  : VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) != 0};
    const VkFilter kFilter{kLinear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST};
    auto entry = std::make_shared<VulkanSamplerEntry>();

    VkSamplerYcbcrConversionInfo conversionInfo{VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO};
    if (kYcbcr)
    {
        VkSamplerYcbcrConversionCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO};
        info.format = import.format;
        info.ycbcrModel = ycbcrModel(description.colour.matrix);
        info.ycbcrRange = ycbcrRange(description.colour.range);
        info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                           VK_COMPONENT_SWIZZLE_IDENTITY};
        // unknown siting: MPEG-2 style 4:2:0, co-sited horizontally and midpoint vertically
        info.xChromaOffset = chromaLocation(description.colour.horizontalSiting, VK_CHROMA_LOCATION_COSITED_EVEN);
        info.yChromaOffset = chromaLocation(description.colour.verticalSiting, VK_CHROMA_LOCATION_MIDPOINT);
        info.chromaFilter = kFilter;
        info.forceExplicitReconstruction = VK_FALSE;
        if (api.createSamplerYcbcrConversion(kDevice, &info, &entry->conversion) != VK_SUCCESS)
        {
            RIALTO_CLIENT_LOG_ERROR("Cannot create the Vulkan YCbCr conversion for a captured frame");
            return nullptr;
        }
        conversionInfo.conversion = entry->conversion;
    }

    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, kYcbcr ? &conversionInfo : nullptr};
    samplerInfo.magFilter = kFilter;
    samplerInfo.minFilter = kFilter;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    if (api.createSampler(kDevice, &samplerInfo, &entry->sampler) != VK_SUCCESS)
    {
        RIALTO_CLIENT_LOG_ERROR("Cannot create the Vulkan sampler for a captured frame");
        entry->sampler = VK_NULL_HANDLE;
        destroySamplerEntry(*entry);
        return nullptr;
    }
    return entry;
}

void VulkanFrameImporter::destroySamplerEntry(const VulkanSamplerEntry &entry)
{
    IVulkanApi &api{m_context->api()};
    const VkDevice kDevice{m_context->device()};
    if (entry.sampler != VK_NULL_HANDLE)
        api.destroySampler(kDevice, entry.sampler);
    if (entry.conversion != VK_NULL_HANDLE)
        api.destroySamplerYcbcrConversion(kDevice, entry.conversion);
}

void VulkanFrameImporter::sweepSamplers(bool all)
{
    for (auto it = m_samplers.begin(); it != m_samplers.end();)
    {
        // imports change their references only here and in importFrame(), both on the acquiring thread
        const bool kUnused{it->second.use_count() == 1};
        const bool kKeep{!all && m_lastSamplerKey && it->first == *m_lastSamplerKey};
        if (kUnused && !kKeep)
        {
            destroySamplerEntry(*it->second);
            it = m_samplers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void VulkanFrameImporter::destroyImport(VulkanImportedImage &import)
{
    IVulkanApi &api{m_context->api()};
    const VkDevice kDevice{m_context->device()};
    // SoC spec 6.5 order, which is also the reverse of creation
    if (import.view != VK_NULL_HANDLE)
        api.destroyImageView(kDevice, import.view);
    // the sampler and conversion are the cache's; sweepSamplers() destroys them once no import holds them
    import.samplerEntry.reset();
    if (import.image != VK_NULL_HANDLE)
        api.destroyImage(kDevice, import.image);
    for (auto it = import.memories.rbegin(); it != import.memories.rend(); ++it)
        api.freeMemory(kDevice, *it);
    import.view = VK_NULL_HANDLE;
    import.sampler = VK_NULL_HANDLE;
    import.conversion = VK_NULL_HANDLE;
    import.image = VK_NULL_HANDLE;
    import.memories.clear();
}

void VulkanFrameImporter::retire(const std::shared_ptr<IImportedImage> &image)
{
    auto vulkanImage = std::dynamic_pointer_cast<VulkanImportedImage>(image);
    if (!vulkanImage)
        return;
    std::lock_guard<std::mutex> lock{m_retiredMutex};
    m_retired.push_back(vulkanImage);
}

void VulkanFrameImporter::drain()
{
    std::vector<std::shared_ptr<VulkanImportedImage>> retired;
    {
        std::lock_guard<std::mutex> lock{m_retiredMutex};
        retired.swap(m_retired);
    }
    std::vector<std::shared_ptr<VulkanImportedImage>> kept;
    for (const std::shared_ptr<VulkanImportedImage> &kImage : retired)
    {
        bool inFlight{false};
        {
            std::lock_guard<std::mutex> lock{m_retiredMutex};
            inFlight = kImage->releasesInFlight > 0;
        }
        if (inFlight)
            kept.push_back(kImage); // its release barrier still needs the image
        else
            destroyImport(*kImage);
    }
    if (!kept.empty())
    {
        std::lock_guard<std::mutex> lock{m_retiredMutex};
        m_retired.insert(m_retired.end(), kept.begin(), kept.end());
    }
    sweepSamplers(false);
}

bool VulkanFrameImporter::deferRelease(const std::shared_ptr<IImportedImage> &image, std::function<void()> sendRelease)
{
    auto vulkanImage = std::dynamic_pointer_cast<VulkanImportedImage>(image);
    if (!vulkanImage)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock{m_retiredMutex};
    ++vulkanImage->releasesInFlight;
    m_pendingReleases.push_back(PendingRelease{vulkanImage, std::move(sendRelease)});
    return true;
}

void VulkanFrameImporter::sendReleases(std::vector<PendingRelease> &releases)
{
    for (PendingRelease &release : releases)
    {
        release.sendRelease();
        std::lock_guard<std::mutex> lock{m_retiredMutex};
        --release.image->releasesInFlight;
    }
    releases.clear();
}

void VulkanFrameImporter::pollSubmissions()
{
    IVulkanApi &api{m_context->api()};
    auto &ring = m_context->ring();
    for (std::size_t i = 0; i < m_inFlight.size(); ++i)
    {
        if (m_inFlight[i].busy && api.getFenceStatus(m_context->device(), ring[i].fence) == VK_SUCCESS)
        {
            m_inFlight[i].busy = false;
            sendReleases(m_inFlight[i].releases);
        }
    }
}

std::size_t VulkanFrameImporter::freeSubmission()
{
    std::size_t oldest{0};
    for (std::size_t i = 0; i < m_inFlight.size(); ++i)
    {
        if (!m_inFlight[i].busy)
            return i;
        if (m_inFlight[i].sequence < m_inFlight[oldest].sequence)
            oldest = i;
    }
    // every entry is in flight: wait for the oldest, which the GPU should finish first
    VkFence fence{m_context->ring()[oldest].fence};
    if (fence != VK_NULL_HANDLE)
        m_context->api().waitForFences(m_context->device(), 1, &fence, VK_TRUE, UINT64_MAX);
    m_inFlight[oldest].busy = false;
    sendReleases(m_inFlight[oldest].releases);
    return oldest;
}

void VulkanFrameImporter::renewFence(VulkanDeviceContext::Submission &submission)
{
    IVulkanApi &api{m_context->api()};
    if (submission.fence != VK_NULL_HANDLE)
        api.destroyFence(m_context->device(), submission.fence);
    VkFenceCreateInfo info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (api.createFence(m_context->device(), &info, &submission.fence) != VK_SUCCESS)
    {
        submission.fence = VK_NULL_HANDLE;
    }
}

void VulkanFrameImporter::submitOwnershipTransfers(const std::shared_ptr<VulkanImportedImage> &acquired)
{
    pollSubmissions();
    std::vector<PendingRelease> releases;
    {
        std::lock_guard<std::mutex> lock{m_retiredMutex};
        releases.swap(m_pendingReleases);
    }
    if (releases.empty() && !acquired)
    {
        return;
    }

    IVulkanApi &api{m_context->api()};
    const VkDevice kDevice{m_context->device()};
    const std::uint32_t kFamily{m_context->queueFamilyIndex()};
    const VkImageLayout kLayout{m_context->requiredImageLayout()};
    const std::size_t kIndex{freeSubmission()};
    VulkanDeviceContext::Submission &submission{m_context->ring()[kIndex]};

    auto barrier = [](VkImage image)
    {
        VkImageMemoryBarrier result{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        result.image = image;
        result.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        return result;
    };
    std::vector<VkImageMemoryBarrier> releaseBarriers;
    for (const PendingRelease &kRelease : releases)
    {
        VkImageMemoryBarrier release{barrier(kRelease.image->image)};
        release.srcAccessMask = 0; // the application only read the image
        release.oldLayout = kLayout;
        release.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        release.srcQueueFamilyIndex = kFamily;
        release.dstQueueFamilyIndex = VK_QUEUE_FAMILY_FOREIGN_EXT;
        releaseBarriers.push_back(release);
    }

    bool recorded{api.resetCommandBuffer(submission.commandBuffer, 0) == VK_SUCCESS};
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    recorded = recorded && api.beginCommandBuffer(submission.commandBuffer, &beginInfo) == VK_SUCCESS;
    if (recorded)
    {
        if (!releaseBarriers.empty())
        {
            // after every earlier use of the images on this queue
            api.cmdPipelineBarrier(submission.commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                   static_cast<std::uint32_t>(releaseBarriers.size()), releaseBarriers.data());
        }
        if (acquired)
        {
            // before any later use of the image on this queue
            VkImageMemoryBarrier acquire{barrier(acquired->image)};
            acquire.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            acquire.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            acquire.newLayout = kLayout;
            acquire.srcQueueFamilyIndex = VK_QUEUE_FAMILY_FOREIGN_EXT;
            acquire.dstQueueFamilyIndex = kFamily;
            api.cmdPipelineBarrier(submission.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 1, &acquire);
        }
        recorded = api.endCommandBuffer(submission.commandBuffer) == VK_SUCCESS;
    }

    if (submission.fence == VK_NULL_HANDLE)
    {
        renewFence(submission); // lost by an earlier failure
    }
    bool submitted{false};
    if (recorded && submission.fence != VK_NULL_HANDLE && api.resetFences(kDevice, 1, &submission.fence) == VK_SUCCESS)
    {
        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &submission.commandBuffer;
        submitted = api.queueSubmit(m_context->queue(), 1, &submitInfo, submission.fence) == VK_SUCCESS;
        if (!submitted)
        {
            renewFence(submission); // the reset fence would never signal
        }
    }
    if (!submitted)
    {
        RIALTO_CLIENT_LOG_ERROR("Cannot submit Vulkan queue-family transfers for video frame capture");
        sendReleases(releases); // the slots must not stay locked; the producer may see the images early
        return;
    }
    m_inFlight[kIndex].busy = true;
    m_inFlight[kIndex].sequence = m_nextSequence++;
    m_inFlight[kIndex].releases = std::move(releases);
    pollSubmissions();
}
} // namespace firebolt::rialto::client
