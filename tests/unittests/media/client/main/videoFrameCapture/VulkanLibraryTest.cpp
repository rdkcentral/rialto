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

#include <gtest/gtest.h>

#include <dlfcn.h>

#include <cstdlib>

using firebolt::rialto::client::VulkanLibrary;

class VulkanLibraryTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        if (auto *fake = dlopen(FAKE_VULKAN_PATH, RTLD_NOW | RTLD_NOLOAD))
        {
            if (auto setMissing =
                    reinterpret_cast<void (*)(const char *)>(dlsym(fake, "fakeVulkanSetMissingDeviceCommand")))
                setMissing("");
            dlclose(fake);
        }
        unsetenv(VulkanLibrary::kLibraryEnv);
    }

    VkDevice m_device{reinterpret_cast<VkDevice>(0x20)};
};

TEST_F(VulkanLibraryTest, ShouldResolveTheDeviceTable)
{
    setenv(VulkanLibrary::kLibraryEnv, FAKE_VULKAN_PATH, 1);
    VulkanLibrary library;
    auto api = library.deviceApi(m_device);
    ASSERT_NE(api, nullptr);
    VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    api->getPhysicalDeviceProperties2(reinterpret_cast<VkPhysicalDevice>(0x10), &properties);
    EXPECT_GE(properties.properties.apiVersion, VK_API_VERSION_1_1);
}

TEST_F(VulkanLibraryTest, ShouldGiveNothingWithoutTheLibrary)
{
    setenv(VulkanLibrary::kLibraryEnv, "/nonexistent/libvulkan.so.1", 1);
    VulkanLibrary library;
    EXPECT_EQ(library.deviceApi(m_device), nullptr);
}

TEST_F(VulkanLibraryTest, ShouldGiveNothingWhenMemoryFdPropertiesIsNotEnabled)
{
    setenv(VulkanLibrary::kLibraryEnv, FAKE_VULKAN_PATH, 1);
    VulkanLibrary library;
    ASSERT_NE(library.deviceApi(m_device), nullptr); // loads the fake library
    void *fake = dlopen(FAKE_VULKAN_PATH, RTLD_NOW | RTLD_NOLOAD);
    ASSERT_NE(fake, nullptr);
    auto setMissing = reinterpret_cast<void (*)(const char *)>(dlsym(fake, "fakeVulkanSetMissingDeviceCommand"));
    ASSERT_NE(setMissing, nullptr);
    setMissing("vkGetMemoryFdPropertiesKHR"); // VK_KHR_external_memory_fd not enabled on the device
    EXPECT_EQ(library.deviceApi(m_device), nullptr);
    dlclose(fake);
}
