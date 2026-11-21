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

#ifndef FIREBOLT_RIALTO_CLIENT_VULKAN_DEVICE_CONTEXT_H_
#define FIREBOLT_RIALTO_CLIENT_VULKAN_DEVICE_CONTEXT_H_

#include "IVulkanApi.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace firebolt::rialto::client
{
/**
 * @brief The application's device and queue as Vulkan capture uses them, with Rialto's command pool on the
 *        queue's family and a ring of command buffers, each with a fence (R13 design D4).
 */
class VulkanDeviceContext
{
public:
    /** One command buffer and the fence of its last submission. */
    struct Submission
    {
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
        VkFence fence{VK_NULL_HANDLE};
    };

    static constexpr std::uint32_t kRingSize{3};

    /**
     * @brief Creates the command pool on \a queueFamilyIndex and the ring.
     *
     * @retval null, with everything created so far destroyed, on failure.
     */
    static std::shared_ptr<VulkanDeviceContext> create(const std::shared_ptr<IVulkanApi> &api,
                                                       VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue,
                                                       std::uint32_t queueFamilyIndex, VkImageLayout requiredImageLayout);

    /** Waits for the ring's fences, then destroys the fences, command buffers and pool. */
    ~VulkanDeviceContext();

    VulkanDeviceContext(const VulkanDeviceContext &) = delete;
    VulkanDeviceContext &operator=(const VulkanDeviceContext &) = delete;

    IVulkanApi &api() const { return *m_api; }
    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    VkDevice device() const { return m_device; }
    VkQueue queue() const { return m_queue; }
    std::uint32_t queueFamilyIndex() const { return m_queueFamilyIndex; }
    VkImageLayout requiredImageLayout() const { return m_requiredImageLayout; }
    VkCommandPool commandPool() const { return m_commandPool; }
    std::vector<Submission> &ring() { return m_ring; }

private:
    VulkanDeviceContext(const std::shared_ptr<IVulkanApi> &api, VkPhysicalDevice physicalDevice, VkDevice device,
                        VkQueue queue, std::uint32_t queueFamilyIndex, VkImageLayout requiredImageLayout);

    std::shared_ptr<IVulkanApi> m_api;
    const VkPhysicalDevice m_physicalDevice;
    const VkDevice m_device;
    const VkQueue m_queue;
    const std::uint32_t m_queueFamilyIndex;
    const VkImageLayout m_requiredImageLayout;
    VkCommandPool m_commandPool{VK_NULL_HANDLE};
    std::vector<Submission> m_ring;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_VULKAN_DEVICE_CONTEXT_H_
