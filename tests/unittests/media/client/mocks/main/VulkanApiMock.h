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

#ifndef FIREBOLT_RIALTO_CLIENT_VULKAN_API_MOCK_H_
#define FIREBOLT_RIALTO_CLIENT_VULKAN_API_MOCK_H_

#include "VulkanLibrary.h"
#include <gmock/gmock.h>

namespace firebolt::rialto::client
{
class VulkanApiMock : public IVulkanApi
{
public:
    MOCK_METHOD(void, getPhysicalDeviceProperties2, (VkPhysicalDevice, VkPhysicalDeviceProperties2 *), (override));
    MOCK_METHOD(void, getPhysicalDeviceFeatures2, (VkPhysicalDevice, VkPhysicalDeviceFeatures2 *), (override));
    MOCK_METHOD(void, getPhysicalDeviceFormatProperties2, (VkPhysicalDevice, VkFormat, VkFormatProperties2 *),
                (override));
    MOCK_METHOD(VkResult, getPhysicalDeviceImageFormatProperties2,
                (VkPhysicalDevice, const VkPhysicalDeviceImageFormatInfo2 *, VkImageFormatProperties2 *), (override));
    MOCK_METHOD(void, getPhysicalDeviceMemoryProperties, (VkPhysicalDevice, VkPhysicalDeviceMemoryProperties *),
                (override));
    MOCK_METHOD(VkResult, getMemoryFdPropertiesKHR,
                (VkDevice, VkExternalMemoryHandleTypeFlagBits, int, VkMemoryFdPropertiesKHR *), (override));
    MOCK_METHOD(VkResult, createImage, (VkDevice, const VkImageCreateInfo *, VkImage *), (override));
    MOCK_METHOD(void, destroyImage, (VkDevice, VkImage), (override));
    MOCK_METHOD(void, getImageMemoryRequirements2,
                (VkDevice, const VkImageMemoryRequirementsInfo2 *, VkMemoryRequirements2 *), (override));
    MOCK_METHOD(VkResult, allocateMemory, (VkDevice, const VkMemoryAllocateInfo *, VkDeviceMemory *), (override));
    MOCK_METHOD(void, freeMemory, (VkDevice, VkDeviceMemory), (override));
    MOCK_METHOD(VkResult, bindImageMemory2, (VkDevice, uint32_t, const VkBindImageMemoryInfo *), (override));
    MOCK_METHOD(VkResult, createImageView, (VkDevice, const VkImageViewCreateInfo *, VkImageView *), (override));
    MOCK_METHOD(void, destroyImageView, (VkDevice, VkImageView), (override));
    MOCK_METHOD(VkResult, createSamplerYcbcrConversion,
                (VkDevice, const VkSamplerYcbcrConversionCreateInfo *, VkSamplerYcbcrConversion *), (override));
    MOCK_METHOD(void, destroySamplerYcbcrConversion, (VkDevice, VkSamplerYcbcrConversion), (override));
    MOCK_METHOD(VkResult, createSampler, (VkDevice, const VkSamplerCreateInfo *, VkSampler *), (override));
    MOCK_METHOD(void, destroySampler, (VkDevice, VkSampler), (override));
    MOCK_METHOD(VkResult, createCommandPool, (VkDevice, const VkCommandPoolCreateInfo *, VkCommandPool *), (override));
    MOCK_METHOD(void, destroyCommandPool, (VkDevice, VkCommandPool), (override));
    MOCK_METHOD(VkResult, allocateCommandBuffers, (VkDevice, const VkCommandBufferAllocateInfo *, VkCommandBuffer *),
                (override));
    MOCK_METHOD(void, freeCommandBuffers, (VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer *), (override));
    MOCK_METHOD(VkResult, beginCommandBuffer, (VkCommandBuffer, const VkCommandBufferBeginInfo *), (override));
    MOCK_METHOD(VkResult, endCommandBuffer, (VkCommandBuffer), (override));
    MOCK_METHOD(VkResult, resetCommandBuffer, (VkCommandBuffer, VkCommandBufferResetFlags), (override));
    MOCK_METHOD(void, cmdPipelineBarrier,
                (VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, uint32_t, const VkImageMemoryBarrier *),
                (override));
    MOCK_METHOD(VkResult, queueSubmit, (VkQueue, uint32_t, const VkSubmitInfo *, VkFence), (override));
    MOCK_METHOD(VkResult, createFence, (VkDevice, const VkFenceCreateInfo *, VkFence *), (override));
    MOCK_METHOD(void, destroyFence, (VkDevice, VkFence), (override));
    MOCK_METHOD(VkResult, getFenceStatus, (VkDevice, VkFence), (override));
    MOCK_METHOD(VkResult, resetFences, (VkDevice, uint32_t, const VkFence *), (override));
    MOCK_METHOD(VkResult, waitForFences, (VkDevice, uint32_t, const VkFence *, VkBool32, uint64_t), (override));
};

class VulkanLibraryMock : public IVulkanLibrary
{
public:
    MOCK_METHOD(std::shared_ptr<IVulkanApi>, deviceApi, (VkDevice device), (override));
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_VULKAN_API_MOCK_H_
