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

#ifndef FIREBOLT_RIALTO_CLIENT_I_VULKAN_API_H_
#define FIREBOLT_RIALTO_CLIENT_I_VULKAN_API_H_

#include <vulkan/vulkan_core.h>

namespace firebolt::rialto::client
{
/**
 * @brief The Vulkan entry points used by Vulkan video frame capture (R13 design D1), for one device.
 *
 * Physical-device queries go through the loader's exported core 1.1 trampolines, so no VkInstance is needed;
 * device commands come from vkGetDeviceProcAddr. Tests replace the whole table with a mock.
 */
class IVulkanApi
{
public:
    virtual ~IVulkanApi() = default;

    // physical device
    virtual void getPhysicalDeviceProperties2(VkPhysicalDevice device, VkPhysicalDeviceProperties2 *properties) = 0;
    virtual void getPhysicalDeviceFeatures2(VkPhysicalDevice device, VkPhysicalDeviceFeatures2 *features) = 0;
    virtual void getPhysicalDeviceFormatProperties2(VkPhysicalDevice device, VkFormat format,
                                                    VkFormatProperties2 *properties) = 0;
    virtual VkResult getPhysicalDeviceImageFormatProperties2(VkPhysicalDevice device,
                                                             const VkPhysicalDeviceImageFormatInfo2 *info,
                                                             VkImageFormatProperties2 *properties) = 0;
    virtual void getPhysicalDeviceMemoryProperties(VkPhysicalDevice device,
                                                   VkPhysicalDeviceMemoryProperties *properties) = 0;

    // memory and images
    virtual VkResult getMemoryFdPropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits type, int fd,
                                              VkMemoryFdPropertiesKHR *properties) = 0;
    virtual VkResult createImage(VkDevice device, const VkImageCreateInfo *info, VkImage *image) = 0;
    virtual void destroyImage(VkDevice device, VkImage image) = 0;
    virtual void getImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2 *info,
                                             VkMemoryRequirements2 *requirements) = 0;
    virtual VkResult allocateMemory(VkDevice device, const VkMemoryAllocateInfo *info, VkDeviceMemory *memory) = 0;
    virtual void freeMemory(VkDevice device, VkDeviceMemory memory) = 0;
    virtual VkResult bindImageMemory2(VkDevice device, uint32_t count, const VkBindImageMemoryInfo *infos) = 0;
    virtual VkResult createImageView(VkDevice device, const VkImageViewCreateInfo *info, VkImageView *view) = 0;
    virtual void destroyImageView(VkDevice device, VkImageView view) = 0;
    virtual VkResult createSamplerYcbcrConversion(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *info,
                                                  VkSamplerYcbcrConversion *conversion) = 0;
    virtual void destroySamplerYcbcrConversion(VkDevice device, VkSamplerYcbcrConversion conversion) = 0;
    virtual VkResult createSampler(VkDevice device, const VkSamplerCreateInfo *info, VkSampler *sampler) = 0;
    virtual void destroySampler(VkDevice device, VkSampler sampler) = 0;

    // commands and synchronization
    virtual VkResult createCommandPool(VkDevice device, const VkCommandPoolCreateInfo *info, VkCommandPool *pool) = 0;
    virtual void destroyCommandPool(VkDevice device, VkCommandPool pool) = 0;
    virtual VkResult allocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *info,
                                            VkCommandBuffer *buffers) = 0;
    virtual void freeCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t count,
                                    const VkCommandBuffer *buffers) = 0;
    virtual VkResult beginCommandBuffer(VkCommandBuffer buffer, const VkCommandBufferBeginInfo *info) = 0;
    virtual VkResult endCommandBuffer(VkCommandBuffer buffer) = 0;
    virtual VkResult resetCommandBuffer(VkCommandBuffer buffer, VkCommandBufferResetFlags flags) = 0;
    virtual void cmdPipelineBarrier(VkCommandBuffer buffer, VkPipelineStageFlags srcStages,
                                    VkPipelineStageFlags dstStages, uint32_t imageBarrierCount,
                                    const VkImageMemoryBarrier *imageBarriers) = 0;
    virtual VkResult queueSubmit(VkQueue queue, uint32_t count, const VkSubmitInfo *submits, VkFence fence) = 0;
    virtual VkResult createFence(VkDevice device, const VkFenceCreateInfo *info, VkFence *fence) = 0;
    virtual void destroyFence(VkDevice device, VkFence fence) = 0;
    virtual VkResult getFenceStatus(VkDevice device, VkFence fence) = 0;
    virtual VkResult resetFences(VkDevice device, uint32_t count, const VkFence *fences) = 0;
    virtual VkResult waitForFences(VkDevice device, uint32_t count, const VkFence *fences, VkBool32 waitAll,
                                   uint64_t timeout) = 0;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_VULKAN_API_H_
