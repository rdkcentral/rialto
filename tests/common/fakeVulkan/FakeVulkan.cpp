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

// A fake libvulkan for tests: the core 1.1 trampolines Rialto uses and the device commands through
// vkGetDeviceProcAddr. It reports a Vulkan 1.1 device with samplerYcbcrConversion, accepts DMA-BUF imports, counts
// live objects, and signals every fence at once. It renders nothing.

#include "FakeVulkanControl.h"

#include <vulkan/vulkan_core.h>

#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <set>
#include <string>

namespace
{
std::mutex g_mutex;
std::set<std::uintptr_t> g_objects;
std::uintptr_t g_nextHandle{0x10000};
std::string g_missingCommand;
std::atomic<int> g_submits{0};
std::atomic<int> g_barriers{0};

template <typename Handle> Handle createObject()
{
    std::lock_guard<std::mutex> lock{g_mutex};
    const std::uintptr_t kHandle{g_nextHandle++};
    g_objects.insert(kHandle);
    return reinterpret_cast<Handle>(kHandle);
}

template <typename Handle> void destroyObject(Handle handle)
{
    if (handle == VK_NULL_HANDLE)
        return;
    std::lock_guard<std::mutex> lock{g_mutex};
    g_objects.erase(reinterpret_cast<std::uintptr_t>(handle));
}
} // namespace

// exported entry points: outside the unnamed namespace, which would give them internal linkage
extern "C"
{
    // physical device trampolines
    VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2(VkPhysicalDevice, VkPhysicalDeviceProperties2 *properties)
    {
        properties->properties.apiVersion = VK_API_VERSION_1_1;
    }
    VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2(VkPhysicalDevice, VkPhysicalDeviceFeatures2 *features)
    {
        for (auto *next = static_cast<VkBaseOutStructure *>(features->pNext); next; next = next->pNext)
        {
            if (next->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES)
                reinterpret_cast<VkPhysicalDeviceSamplerYcbcrConversionFeatures *>(next)->samplerYcbcrConversion = VK_TRUE;
        }
    }
    VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2(VkPhysicalDevice, VkFormat,
                                                                    VkFormatProperties2 *properties)
    {
        properties->formatProperties.optimalTilingFeatures =
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT;
        for (auto *next = static_cast<VkBaseOutStructure *>(properties->pNext); next; next = next->pNext)
        {
            if (next->sType == VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT)
            {
                auto *list = reinterpret_cast<VkDrmFormatModifierPropertiesListEXT *>(next);
                if (list->pDrmFormatModifierProperties && list->drmFormatModifierCount >= 1)
                {
                    list->pDrmFormatModifierProperties[0] =
                        {0, 2,
                         VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                             VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT};
                }
                list->drmFormatModifierCount = 1; // the linear modifier, two planes
            }
        }
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice,
                                                                             const VkPhysicalDeviceImageFormatInfo2 *,
                                                                             VkImageFormatProperties2 *)
    {
        return VK_SUCCESS;
    }
    VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice,
                                                                   VkPhysicalDeviceMemoryProperties *properties)
    {
        properties->memoryTypeCount = 1;
        properties->memoryTypes[0] = {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0};
        properties->memoryHeapCount = 1;
    }

    // device commands
    VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryFdPropertiesKHR(VkDevice, VkExternalMemoryHandleTypeFlagBits, int,
                                                              VkMemoryFdPropertiesKHR *properties)
    {
        properties->memoryTypeBits = 1;
        return VK_SUCCESS;
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(VkDevice, const VkImageCreateInfo *, const VkAllocationCallbacks *,
                                                 VkImage *image)
    {
        *image = createObject<VkImage>();
        return VK_SUCCESS;
    }
    VKAPI_ATTR void VKAPI_CALL vkDestroyImage(VkDevice, VkImage image, const VkAllocationCallbacks *)
    {
        destroyObject(image);
    }
    VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements2(VkDevice, const VkImageMemoryRequirementsInfo2 *,
                                                             VkMemoryRequirements2 *requirements)
    {
        requirements->memoryRequirements = {4096, 4096, 1};
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(VkDevice, const VkMemoryAllocateInfo *info,
                                                    const VkAllocationCallbacks *, VkDeviceMemory *memory)
    {
        // a successful import takes ownership of the descriptor
        for (auto *next = static_cast<const VkBaseInStructure *>(info->pNext); next; next = next->pNext)
        {
            if (next->sType == VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR)
                close(reinterpret_cast<const VkImportMemoryFdInfoKHR *>(next)->fd);
        }
        *memory = createObject<VkDeviceMemory>();
        return VK_SUCCESS;
    }
    VKAPI_ATTR void VKAPI_CALL vkFreeMemory(VkDevice, VkDeviceMemory memory, const VkAllocationCallbacks *)
    {
        destroyObject(memory);
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory2(VkDevice, uint32_t, const VkBindImageMemoryInfo *)
    {
        return VK_SUCCESS;
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(VkDevice, const VkImageViewCreateInfo *,
                                                     const VkAllocationCallbacks *, VkImageView *view)
    {
        *view = createObject<VkImageView>();
        return VK_SUCCESS;
    }
    VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(VkDevice, VkImageView view, const VkAllocationCallbacks *)
    {
        destroyObject(view);
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateSamplerYcbcrConversion(VkDevice, const VkSamplerYcbcrConversionCreateInfo *,
                                                                  const VkAllocationCallbacks *,
                                                                  VkSamplerYcbcrConversion *conversion)
    {
        *conversion = createObject<VkSamplerYcbcrConversion>();
        return VK_SUCCESS;
    }
    VKAPI_ATTR void VKAPI_CALL vkDestroySamplerYcbcrConversion(VkDevice, VkSamplerYcbcrConversion conversion,
                                                               const VkAllocationCallbacks *)
    {
        destroyObject(conversion);
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler(VkDevice, const VkSamplerCreateInfo *, const VkAllocationCallbacks *,
                                                   VkSampler *sampler)
    {
        *sampler = createObject<VkSampler>();
        return VK_SUCCESS;
    }
    VKAPI_ATTR void VKAPI_CALL vkDestroySampler(VkDevice, VkSampler sampler, const VkAllocationCallbacks *)
    {
        destroyObject(sampler);
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(VkDevice, const VkCommandPoolCreateInfo *,
                                                       const VkAllocationCallbacks *, VkCommandPool *pool)
    {
        *pool = createObject<VkCommandPool>();
        return VK_SUCCESS;
    }
    VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(VkDevice, VkCommandPool pool, const VkAllocationCallbacks *)
    {
        destroyObject(pool);
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(VkDevice, const VkCommandBufferAllocateInfo *info,
                                                            VkCommandBuffer *buffers)
    {
        for (uint32_t i = 0; i < info->commandBufferCount; ++i)
            buffers[i] = reinterpret_cast<VkCommandBuffer>(0x5000 + i); // freed with their pool
        return VK_SUCCESS;
    }
    VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer *) {}
    VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(VkCommandBuffer, const VkCommandBufferBeginInfo *)
    {
        return VK_SUCCESS;
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(VkCommandBuffer)
    {
        return VK_SUCCESS;
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(VkCommandBuffer, VkCommandBufferResetFlags)
    {
        return VK_SUCCESS;
    }
    VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags,
                                                    VkDependencyFlags, uint32_t, const VkMemoryBarrier *, uint32_t,
                                                    const VkBufferMemoryBarrier *, uint32_t imageBarrierCount,
                                                    const VkImageMemoryBarrier *)
    {
        g_barriers += static_cast<int>(imageBarrierCount);
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue, uint32_t, const VkSubmitInfo *, VkFence)
    {
        ++g_submits;
        return VK_SUCCESS;
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateFence(VkDevice, const VkFenceCreateInfo *, const VkAllocationCallbacks *,
                                                 VkFence *fence)
    {
        *fence = createObject<VkFence>();
        return VK_SUCCESS;
    }
    VKAPI_ATTR void VKAPI_CALL vkDestroyFence(VkDevice, VkFence fence, const VkAllocationCallbacks *)
    {
        destroyObject(fence);
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(VkDevice, VkFence)
    {
        return VK_SUCCESS;
    } // signalled at once
    VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(VkDevice, uint32_t, const VkFence *)
    {
        return VK_SUCCESS;
    }
    VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(VkDevice, uint32_t, const VkFence *, VkBool32, uint64_t)
    {
        return VK_SUCCESS;
    }

    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice, const char *name)
    {
        {
            std::lock_guard<std::mutex> lock{g_mutex};
            if (!g_missingCommand.empty() && g_missingCommand == name)
                return nullptr;
        }
        struct Entry
        {
            const char *name;
            PFN_vkVoidFunction function;
        };
#define FAKE_VULKAN_ENTRY(function) {#function, reinterpret_cast<PFN_vkVoidFunction>(&function)}
        static const Entry kEntries[]{
            FAKE_VULKAN_ENTRY(vkGetMemoryFdPropertiesKHR),
            FAKE_VULKAN_ENTRY(vkCreateImage),
            FAKE_VULKAN_ENTRY(vkDestroyImage),
            FAKE_VULKAN_ENTRY(vkGetImageMemoryRequirements2),
            FAKE_VULKAN_ENTRY(vkAllocateMemory),
            FAKE_VULKAN_ENTRY(vkFreeMemory),
            FAKE_VULKAN_ENTRY(vkBindImageMemory2),
            FAKE_VULKAN_ENTRY(vkCreateImageView),
            FAKE_VULKAN_ENTRY(vkDestroyImageView),
            FAKE_VULKAN_ENTRY(vkCreateSamplerYcbcrConversion),
            FAKE_VULKAN_ENTRY(vkDestroySamplerYcbcrConversion),
            FAKE_VULKAN_ENTRY(vkCreateSampler),
            FAKE_VULKAN_ENTRY(vkDestroySampler),
            FAKE_VULKAN_ENTRY(vkCreateCommandPool),
            FAKE_VULKAN_ENTRY(vkDestroyCommandPool),
            FAKE_VULKAN_ENTRY(vkAllocateCommandBuffers),
            FAKE_VULKAN_ENTRY(vkFreeCommandBuffers),
            FAKE_VULKAN_ENTRY(vkBeginCommandBuffer),
            FAKE_VULKAN_ENTRY(vkEndCommandBuffer),
            FAKE_VULKAN_ENTRY(vkResetCommandBuffer),
            FAKE_VULKAN_ENTRY(vkCmdPipelineBarrier),
            FAKE_VULKAN_ENTRY(vkQueueSubmit),
            FAKE_VULKAN_ENTRY(vkCreateFence),
            FAKE_VULKAN_ENTRY(vkDestroyFence),
            FAKE_VULKAN_ENTRY(vkGetFenceStatus),
            FAKE_VULKAN_ENTRY(vkResetFences),
            FAKE_VULKAN_ENTRY(vkWaitForFences),
        };
#undef FAKE_VULKAN_ENTRY
        for (const Entry &kEntry : kEntries)
        {
            if (std::strcmp(kEntry.name, name) == 0)
                return kEntry.function;
        }
        return nullptr;
    }

    // controls
    void fakeVulkanReset()
    {
        std::lock_guard<std::mutex> lock{g_mutex};
        g_objects.clear();
        g_missingCommand.clear();
        g_submits = 0;
        g_barriers = 0;
    }
    void fakeVulkanSetMissingDeviceCommand(const char *name)
    {
        std::lock_guard<std::mutex> lock{g_mutex};
        g_missingCommand = name ? name : "";
    }
    int fakeVulkanLiveObjects()
    {
        std::lock_guard<std::mutex> lock{g_mutex};
        return static_cast<int>(g_objects.size());
    }
    int fakeVulkanSubmitCount()
    {
        return g_submits;
    }
    int fakeVulkanBarrierCount()
    {
        return g_barriers;
    }
}
