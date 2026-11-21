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

#ifndef FIREBOLT_RIALTO_CLIENT_VULKAN_LIBRARY_H_
#define FIREBOLT_RIALTO_CLIENT_VULKAN_LIBRARY_H_

#include "IVulkanApi.h"

#include <memory>
#include <mutex>

namespace firebolt::rialto::client
{
/**
 * @brief Loads libvulkan at run time for Vulkan video frame capture, so libRialtoClient has no link-time Vulkan
 *        dependency.
 */
class IVulkanLibrary
{
public:
    IVulkanLibrary() = default;
    virtual ~IVulkanLibrary() = default;

    IVulkanLibrary(const IVulkanLibrary &) = delete;
    IVulkanLibrary &operator=(const IVulkanLibrary &) = delete;

    /** The process-wide Vulkan loader. */
    static std::shared_ptr<IVulkanLibrary> instance();

    /**
     * @brief The entry points for a device.
     *
     * @retval null if libvulkan cannot be loaded, a core 1.1 trampoline is missing, or the device lacks a required
     *         command (for example vkGetMemoryFdPropertiesKHR when VK_KHR_external_memory_fd is not enabled).
     */
    virtual std::shared_ptr<IVulkanApi> deviceApi(VkDevice device) = 0;
};

class VulkanLibrary : public IVulkanLibrary
{
public:
    static constexpr const char *kLibraryName{"libvulkan.so.1"};
    static constexpr const char *kLibraryEnv{"RIALTO_VULKAN_LIBRARY"};

    VulkanLibrary() = default;
    ~VulkanLibrary() override;

    std::shared_ptr<IVulkanApi> deviceApi(VkDevice device) override;

private:
    bool load();

    std::mutex m_mutex;
    bool m_loadAttempted{false};
    void *m_handle{nullptr};
};

/**
 * @brief Whether a device can capture (R13 2.1): Vulkan 1.1 or later and the samplerYcbcrConversion feature.
 */
bool isVulkanDeviceSupported(IVulkanApi &api, VkPhysicalDevice physicalDevice);
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_VULKAN_LIBRARY_H_
