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

#include "VulkanDeviceContext.h"
#include "RialtoClientLogging.h"

#include <limits>

namespace firebolt::rialto::client
{
VulkanDeviceContext::VulkanDeviceContext(const std::shared_ptr<IVulkanApi> &api, VkPhysicalDevice physicalDevice,
                                         VkDevice device, VkQueue queue, std::uint32_t queueFamilyIndex,
                                         VkImageLayout requiredImageLayout)
    : m_api{api}, m_physicalDevice{physicalDevice}, m_device{device}, m_queue{queue},
      m_queueFamilyIndex{queueFamilyIndex}, m_requiredImageLayout{requiredImageLayout}
{
}

std::shared_ptr<VulkanDeviceContext> VulkanDeviceContext::create(const std::shared_ptr<IVulkanApi> &api,
                                                                 VkPhysicalDevice physicalDevice, VkDevice device,
                                                                 VkQueue queue, std::uint32_t queueFamilyIndex,
                                                                 VkImageLayout requiredImageLayout)
{
    std::shared_ptr<VulkanDeviceContext> context{
        new VulkanDeviceContext{api, physicalDevice, device, queue, queueFamilyIndex, requiredImageLayout}};

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex; // barriers are recorded for, and submitted to, this family
    if (api->createCommandPool(device, &poolInfo, &context->m_commandPool) != VK_SUCCESS)
    {
        RIALTO_CLIENT_LOG_ERROR("Cannot create the Vulkan command pool for video frame capture");
        context->m_commandPool = VK_NULL_HANDLE;
        return nullptr;
    }

    std::vector<VkCommandBuffer> buffers(kRingSize, VK_NULL_HANDLE);
    VkCommandBufferAllocateInfo bufferInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    bufferInfo.commandPool = context->m_commandPool;
    bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferInfo.commandBufferCount = kRingSize;
    if (api->allocateCommandBuffers(device, &bufferInfo, buffers.data()) != VK_SUCCESS)
    {
        RIALTO_CLIENT_LOG_ERROR("Cannot allocate Vulkan command buffers for video frame capture");
        return nullptr; // the destructor destroys the pool
    }
    for (VkCommandBuffer buffer : buffers)
    {
        context->m_ring.push_back(Submission{buffer, VK_NULL_HANDLE});
    }
    for (Submission &submission : context->m_ring)
    {
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // a free buffer
        if (api->createFence(device, &fenceInfo, &submission.fence) != VK_SUCCESS)
        {
            RIALTO_CLIENT_LOG_ERROR("Cannot create a Vulkan fence for video frame capture");
            submission.fence = VK_NULL_HANDLE;
            return nullptr;
        }
    }
    return context;
}

VulkanDeviceContext::~VulkanDeviceContext()
{
    std::vector<VkFence> fences;
    for (const Submission &kSubmission : m_ring)
    {
        if (kSubmission.fence != VK_NULL_HANDLE)
            fences.push_back(kSubmission.fence);
    }
    if (!fences.empty())
    {
        m_api->waitForFences(m_device, static_cast<std::uint32_t>(fences.size()), fences.data(), VK_TRUE,
                             std::numeric_limits<std::uint64_t>::max());
    }
    for (VkFence fence : fences)
    {
        m_api->destroyFence(m_device, fence);
    }
    std::vector<VkCommandBuffer> buffers;
    for (const Submission &kSubmission : m_ring)
    {
        buffers.push_back(kSubmission.commandBuffer);
    }
    if (!buffers.empty())
    {
        m_api->freeCommandBuffers(m_device, m_commandPool, static_cast<std::uint32_t>(buffers.size()), buffers.data());
    }
    if (m_commandPool != VK_NULL_HANDLE)
    {
        m_api->destroyCommandPool(m_device, m_commandPool);
    }
}
} // namespace firebolt::rialto::client
