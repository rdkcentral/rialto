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

#include "VulkanLibrary.h"
#include "RialtoClientLogging.h"

#include <dlfcn.h>

#include <cstdlib>
#include <type_traits>

namespace firebolt::rialto::client
{
namespace
{
/**
 * @brief The physical-device trampolines exported by the loader.
 */
struct LoaderFunctions
{
    PFN_vkGetPhysicalDeviceProperties2 getPhysicalDeviceProperties2{nullptr};
    PFN_vkGetPhysicalDeviceFeatures2 getPhysicalDeviceFeatures2{nullptr};
    PFN_vkGetPhysicalDeviceFormatProperties2 getPhysicalDeviceFormatProperties2{nullptr};
    PFN_vkGetPhysicalDeviceImageFormatProperties2 getPhysicalDeviceImageFormatProperties2{nullptr};
    PFN_vkGetPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties{nullptr};
    PFN_vkGetDeviceProcAddr getDeviceProcAddr{nullptr};
};

class VulkanApi : public IVulkanApi
{
public:
    VulkanApi(const LoaderFunctions &loader, VkDevice device) : m_loader{loader}
    {
        auto get = [&](auto &function, const char *name)
        {
            function =
                reinterpret_cast<std::remove_reference_t<decltype(function)>>(m_loader.getDeviceProcAddr(device, name));
            if (!function)
            {
                RIALTO_CLIENT_LOG_ERROR("Vulkan device command %s is missing (extension not enabled?)", name);
                m_complete = false;
            }
        };
        get(m_getMemoryFdPropertiesKHR, "vkGetMemoryFdPropertiesKHR");
        get(m_createImage, "vkCreateImage");
        get(m_destroyImage, "vkDestroyImage");
        get(m_getImageMemoryRequirements2, "vkGetImageMemoryRequirements2");
        get(m_allocateMemory, "vkAllocateMemory");
        get(m_freeMemory, "vkFreeMemory");
        get(m_bindImageMemory2, "vkBindImageMemory2");
        get(m_createImageView, "vkCreateImageView");
        get(m_destroyImageView, "vkDestroyImageView");
        get(m_createSamplerYcbcrConversion, "vkCreateSamplerYcbcrConversion");
        get(m_destroySamplerYcbcrConversion, "vkDestroySamplerYcbcrConversion");
        get(m_createSampler, "vkCreateSampler");
        get(m_destroySampler, "vkDestroySampler");
        get(m_createCommandPool, "vkCreateCommandPool");
        get(m_destroyCommandPool, "vkDestroyCommandPool");
        get(m_allocateCommandBuffers, "vkAllocateCommandBuffers");
        get(m_freeCommandBuffers, "vkFreeCommandBuffers");
        get(m_beginCommandBuffer, "vkBeginCommandBuffer");
        get(m_endCommandBuffer, "vkEndCommandBuffer");
        get(m_resetCommandBuffer, "vkResetCommandBuffer");
        get(m_cmdPipelineBarrier, "vkCmdPipelineBarrier");
        get(m_queueSubmit, "vkQueueSubmit");
        get(m_createFence, "vkCreateFence");
        get(m_destroyFence, "vkDestroyFence");
        get(m_getFenceStatus, "vkGetFenceStatus");
        get(m_resetFences, "vkResetFences");
        get(m_waitForFences, "vkWaitForFences");
    }

    bool isComplete() const { return m_complete; }

    void getPhysicalDeviceProperties2(VkPhysicalDevice device, VkPhysicalDeviceProperties2 *properties) override
    {
        m_loader.getPhysicalDeviceProperties2(device, properties);
    }
    void getPhysicalDeviceFeatures2(VkPhysicalDevice device, VkPhysicalDeviceFeatures2 *features) override
    {
        m_loader.getPhysicalDeviceFeatures2(device, features);
    }
    void getPhysicalDeviceFormatProperties2(VkPhysicalDevice device, VkFormat format,
                                            VkFormatProperties2 *properties) override
    {
        m_loader.getPhysicalDeviceFormatProperties2(device, format, properties);
    }
    VkResult getPhysicalDeviceImageFormatProperties2(VkPhysicalDevice device,
                                                     const VkPhysicalDeviceImageFormatInfo2 *info,
                                                     VkImageFormatProperties2 *properties) override
    {
        return m_loader.getPhysicalDeviceImageFormatProperties2(device, info, properties);
    }
    void getPhysicalDeviceMemoryProperties(VkPhysicalDevice device, VkPhysicalDeviceMemoryProperties *properties) override
    {
        m_loader.getPhysicalDeviceMemoryProperties(device, properties);
    }
    VkResult getMemoryFdPropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits type, int fd,
                                      VkMemoryFdPropertiesKHR *properties) override
    {
        return m_getMemoryFdPropertiesKHR(device, type, fd, properties);
    }
    VkResult createImage(VkDevice device, const VkImageCreateInfo *info, VkImage *image) override
    {
        return m_createImage(device, info, nullptr, image);
    }
    void destroyImage(VkDevice device, VkImage image) override { m_destroyImage(device, image, nullptr); }
    void getImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2 *info,
                                     VkMemoryRequirements2 *requirements) override
    {
        m_getImageMemoryRequirements2(device, info, requirements);
    }
    VkResult allocateMemory(VkDevice device, const VkMemoryAllocateInfo *info, VkDeviceMemory *memory) override
    {
        return m_allocateMemory(device, info, nullptr, memory);
    }
    void freeMemory(VkDevice device, VkDeviceMemory memory) override { m_freeMemory(device, memory, nullptr); }
    VkResult bindImageMemory2(VkDevice device, uint32_t count, const VkBindImageMemoryInfo *infos) override
    {
        return m_bindImageMemory2(device, count, infos);
    }
    VkResult createImageView(VkDevice device, const VkImageViewCreateInfo *info, VkImageView *view) override
    {
        return m_createImageView(device, info, nullptr, view);
    }
    void destroyImageView(VkDevice device, VkImageView view) override { m_destroyImageView(device, view, nullptr); }
    VkResult createSamplerYcbcrConversion(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *info,
                                          VkSamplerYcbcrConversion *conversion) override
    {
        return m_createSamplerYcbcrConversion(device, info, nullptr, conversion);
    }
    void destroySamplerYcbcrConversion(VkDevice device, VkSamplerYcbcrConversion conversion) override
    {
        m_destroySamplerYcbcrConversion(device, conversion, nullptr);
    }
    VkResult createSampler(VkDevice device, const VkSamplerCreateInfo *info, VkSampler *sampler) override
    {
        return m_createSampler(device, info, nullptr, sampler);
    }
    void destroySampler(VkDevice device, VkSampler sampler) override { m_destroySampler(device, sampler, nullptr); }
    VkResult createCommandPool(VkDevice device, const VkCommandPoolCreateInfo *info, VkCommandPool *pool) override
    {
        return m_createCommandPool(device, info, nullptr, pool);
    }
    void destroyCommandPool(VkDevice device, VkCommandPool pool) override
    {
        m_destroyCommandPool(device, pool, nullptr);
    }
    VkResult allocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *info,
                                    VkCommandBuffer *buffers) override
    {
        return m_allocateCommandBuffers(device, info, buffers);
    }
    void freeCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t count, const VkCommandBuffer *buffers) override
    {
        m_freeCommandBuffers(device, pool, count, buffers);
    }
    VkResult beginCommandBuffer(VkCommandBuffer buffer, const VkCommandBufferBeginInfo *info) override
    {
        return m_beginCommandBuffer(buffer, info);
    }
    VkResult endCommandBuffer(VkCommandBuffer buffer) override { return m_endCommandBuffer(buffer); }
    VkResult resetCommandBuffer(VkCommandBuffer buffer, VkCommandBufferResetFlags flags) override
    {
        return m_resetCommandBuffer(buffer, flags);
    }
    void cmdPipelineBarrier(VkCommandBuffer buffer, VkPipelineStageFlags srcStages, VkPipelineStageFlags dstStages,
                            uint32_t imageBarrierCount, const VkImageMemoryBarrier *imageBarriers) override
    {
        m_cmdPipelineBarrier(buffer, srcStages, dstStages, 0, 0, nullptr, 0, nullptr, imageBarrierCount, imageBarriers);
    }
    VkResult queueSubmit(VkQueue queue, uint32_t count, const VkSubmitInfo *submits, VkFence fence) override
    {
        return m_queueSubmit(queue, count, submits, fence);
    }
    VkResult createFence(VkDevice device, const VkFenceCreateInfo *info, VkFence *fence) override
    {
        return m_createFence(device, info, nullptr, fence);
    }
    void destroyFence(VkDevice device, VkFence fence) override { m_destroyFence(device, fence, nullptr); }
    VkResult getFenceStatus(VkDevice device, VkFence fence) override { return m_getFenceStatus(device, fence); }
    VkResult resetFences(VkDevice device, uint32_t count, const VkFence *fences) override
    {
        return m_resetFences(device, count, fences);
    }
    VkResult waitForFences(VkDevice device, uint32_t count, const VkFence *fences, VkBool32 waitAll,
                           uint64_t timeout) override
    {
        return m_waitForFences(device, count, fences, waitAll, timeout);
    }

private:
    const LoaderFunctions m_loader;
    bool m_complete{true};
    PFN_vkGetMemoryFdPropertiesKHR m_getMemoryFdPropertiesKHR{nullptr};
    PFN_vkCreateImage m_createImage{nullptr};
    PFN_vkDestroyImage m_destroyImage{nullptr};
    PFN_vkGetImageMemoryRequirements2 m_getImageMemoryRequirements2{nullptr};
    PFN_vkAllocateMemory m_allocateMemory{nullptr};
    PFN_vkFreeMemory m_freeMemory{nullptr};
    PFN_vkBindImageMemory2 m_bindImageMemory2{nullptr};
    PFN_vkCreateImageView m_createImageView{nullptr};
    PFN_vkDestroyImageView m_destroyImageView{nullptr};
    PFN_vkCreateSamplerYcbcrConversion m_createSamplerYcbcrConversion{nullptr};
    PFN_vkDestroySamplerYcbcrConversion m_destroySamplerYcbcrConversion{nullptr};
    PFN_vkCreateSampler m_createSampler{nullptr};
    PFN_vkDestroySampler m_destroySampler{nullptr};
    PFN_vkCreateCommandPool m_createCommandPool{nullptr};
    PFN_vkDestroyCommandPool m_destroyCommandPool{nullptr};
    PFN_vkAllocateCommandBuffers m_allocateCommandBuffers{nullptr};
    PFN_vkFreeCommandBuffers m_freeCommandBuffers{nullptr};
    PFN_vkBeginCommandBuffer m_beginCommandBuffer{nullptr};
    PFN_vkEndCommandBuffer m_endCommandBuffer{nullptr};
    PFN_vkResetCommandBuffer m_resetCommandBuffer{nullptr};
    PFN_vkCmdPipelineBarrier m_cmdPipelineBarrier{nullptr};
    PFN_vkQueueSubmit m_queueSubmit{nullptr};
    PFN_vkCreateFence m_createFence{nullptr};
    PFN_vkDestroyFence m_destroyFence{nullptr};
    PFN_vkGetFenceStatus m_getFenceStatus{nullptr};
    PFN_vkResetFences m_resetFences{nullptr};
    PFN_vkWaitForFences m_waitForFences{nullptr};
};
} // namespace

std::shared_ptr<IVulkanLibrary> IVulkanLibrary::instance()
{
    static std::shared_ptr<IVulkanLibrary> library{std::make_shared<VulkanLibrary>()};
    return library;
}

VulkanLibrary::~VulkanLibrary()
{
    if (m_handle)
    {
        dlclose(m_handle);
    }
}

bool VulkanLibrary::load()
{
    if (!m_loadAttempted)
    {
        m_loadAttempted = true;
        const char *kOverride = std::getenv(kLibraryEnv);
        const char *kName = (kOverride && *kOverride) ? kOverride : kLibraryName;
        m_handle = dlopen(kName, RTLD_NOW | RTLD_LOCAL);
        if (!m_handle)
        {
            const char *kError = dlerror();
            RIALTO_CLIENT_LOG_WARN("Cannot load %s: %s", kName, kError ? kError : "unknown error");
        }
    }
    return m_handle != nullptr;
}

std::shared_ptr<IVulkanApi> VulkanLibrary::deviceApi(VkDevice device)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    if (!load())
    {
        return nullptr;
    }
    LoaderFunctions loader;
    bool ok{true};
    auto get = [&](auto &function, const char *name)
    {
        function = reinterpret_cast<std::remove_reference_t<decltype(function)>>(dlsym(m_handle, name));
        if (!function)
        {
            RIALTO_CLIENT_LOG_ERROR("Vulkan loader function %s is missing (loader older than 1.1?)", name);
            ok = false;
        }
    };
    get(loader.getPhysicalDeviceProperties2, "vkGetPhysicalDeviceProperties2");
    get(loader.getPhysicalDeviceFeatures2, "vkGetPhysicalDeviceFeatures2");
    get(loader.getPhysicalDeviceFormatProperties2, "vkGetPhysicalDeviceFormatProperties2");
    get(loader.getPhysicalDeviceImageFormatProperties2, "vkGetPhysicalDeviceImageFormatProperties2");
    get(loader.getPhysicalDeviceMemoryProperties, "vkGetPhysicalDeviceMemoryProperties");
    get(loader.getDeviceProcAddr, "vkGetDeviceProcAddr");
    if (!ok)
    {
        return nullptr;
    }
    auto api = std::make_shared<VulkanApi>(loader, device);
    return api->isComplete() ? api : nullptr;
}

bool isVulkanDeviceSupported(IVulkanApi &api, VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    api.getPhysicalDeviceProperties2(physicalDevice, &properties);
    if (properties.properties.apiVersion < VK_API_VERSION_1_1)
    {
        RIALTO_CLIENT_LOG_WARN("Vulkan video frame capture needs a Vulkan 1.1 device; this one is %u.%u",
                               VK_API_VERSION_MAJOR(properties.properties.apiVersion),
                               VK_API_VERSION_MINOR(properties.properties.apiVersion));
        return false;
    }
    VkPhysicalDeviceSamplerYcbcrConversionFeatures ycbcr{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES};
    VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &ycbcr};
    api.getPhysicalDeviceFeatures2(physicalDevice, &features);
    if (!ycbcr.samplerYcbcrConversion)
    {
        RIALTO_CLIENT_LOG_WARN("Vulkan video frame capture needs the samplerYcbcrConversion feature");
        return false;
    }
    return true;
}
} // namespace firebolt::rialto::client
