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

#ifndef VULKAN_IMPORT_TEST_BASE_H_
#define VULKAN_IMPORT_TEST_BASE_H_

#include "VulkanApiMock.h"
#include "VulkanDeviceContext.h"
#include "VulkanFrameImporter.h"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <unistd.h>

#include <cstdint>
#include <memory>
#include <set>
#include <vector>

/**
 * A Vulkan importer over the mock table, which hands out unique handles and tracks the live ones. A successful
 * memory import closes its descriptor, as a driver taking ownership would.
 */
class VulkanImportTestBase : public ::testing::Test
{
protected:
    static constexpr std::uint32_t kNv12{0x3231564e};
    static constexpr std::uint32_t kYu12{0x32315559};
    static constexpr std::uint32_t kAr24{0x34325241};

    VulkanImportTestBase()
    {
        using ::testing::_;
        using ::testing::Invoke;
        using ::testing::Return;
        ON_CALL(*m_api, getPhysicalDeviceFormatProperties2(_, _, _))
            .WillByDefault(Invoke(
                [this](VkPhysicalDevice, VkFormat format, VkFormatProperties2 *properties)
                {
                    auto *list = static_cast<VkDrmFormatModifierPropertiesListEXT *>(properties->pNext);
                    ASSERT_NE(list, nullptr);
                    if (list->pDrmFormatModifierProperties)
                    {
                        const bool kThreePlanes{format == VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM};
                        const bool kOnePlane{format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_R8G8B8A8_UNORM};
                        list->pDrmFormatModifierProperties[0] = {0, kOnePlane ? 1U : (kThreePlanes ? 3U : 2U),
                                                                 m_modifierFeatures};
                    }
                    list->drmFormatModifierCount = 1;
                }));
        ON_CALL(*m_api, getMemoryFdPropertiesKHR(_, _, _, _))
            .WillByDefault(Invoke(
                [](VkDevice, VkExternalMemoryHandleTypeFlagBits, int, VkMemoryFdPropertiesKHR *properties)
                {
                    properties->memoryTypeBits = 0x6;
                    return VK_SUCCESS;
                }));
        ON_CALL(*m_api, getImageMemoryRequirements2(_, _, _))
            .WillByDefault(Invoke([](VkDevice, const VkImageMemoryRequirementsInfo2 *, VkMemoryRequirements2 *requirements)
                                  { requirements->memoryRequirements = {8192, 4096, 0x3}; }));
        ON_CALL(*m_api, createImage(_, _, _))
            .WillByDefault(Invoke(
                [this](VkDevice, const VkImageCreateInfo *info, VkImage *image)
                {
                    m_lastImageFlags = info->flags;
                    return created(image);
                }));
        ON_CALL(*m_api, allocateMemory(_, _, _))
            .WillByDefault(Invoke(
                [this](VkDevice, const VkMemoryAllocateInfo *info, VkDeviceMemory *memory)
                {
                    const auto *kImport = static_cast<const VkImportMemoryFdInfoKHR *>(info->pNext);
                    EXPECT_EQ(kImport->sType, VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR);
                    EXPECT_EQ(info->memoryTypeIndex, 1U); // first of fd types 0x6 and image types 0x3
                    EXPECT_NE(fcntl(kImport->fd, F_GETFD), -1) << "each import needs its own valid descriptor";
                    m_importFds.push_back(kImport->fd);
                    if (m_failAllocation == m_importFds.size())
                        return VK_ERROR_INVALID_EXTERNAL_HANDLE;
                    close(kImport->fd); // owned by the driver now
                    return created(memory);
                }));
        ON_CALL(*m_api, createSamplerYcbcrConversion(_, _, _))
            .WillByDefault(Invoke(
                [this](VkDevice, const VkSamplerYcbcrConversionCreateInfo *info, VkSamplerYcbcrConversion *conversion)
                {
                    m_lastConversion = *info;
                    return created(conversion);
                }));
        ON_CALL(*m_api, createSampler(_, _, _))
            .WillByDefault(Invoke(
                [this](VkDevice, const VkSamplerCreateInfo *info, VkSampler *sampler)
                {
                    m_lastSampler = *info;
                    return created(sampler);
                }));
        ON_CALL(*m_api, createImageView(_, _, _))
            .WillByDefault(
                Invoke([this](VkDevice, const VkImageViewCreateInfo *, VkImageView *view) { return created(view); }));
        ON_CALL(*m_api, destroyImage(_, _)).WillByDefault(Invoke([this](VkDevice, VkImage h) { destroyed(h); }));
        ON_CALL(*m_api, freeMemory(_, _)).WillByDefault(Invoke([this](VkDevice, VkDeviceMemory h) { destroyed(h); }));
        ON_CALL(*m_api, destroyImageView(_, _)).WillByDefault(Invoke([this](VkDevice, VkImageView h) { destroyed(h); }));
        ON_CALL(*m_api, destroySampler(_, _)).WillByDefault(Invoke([this](VkDevice, VkSampler h) { destroyed(h); }));
        ON_CALL(*m_api, destroySamplerYcbcrConversion(_, _))
            .WillByDefault(Invoke([this](VkDevice, VkSamplerYcbcrConversion h) { destroyed(h); }));
    }

    ~VulkanImportTestBase() override
    {
        for (int fd : m_openFds)
        {
            if (fcntl(fd, F_GETFD) != -1)
                close(fd);
        }
    }

    template <typename Handle> VkResult created(Handle *handle)
    {
        const std::uintptr_t kValue{m_nextHandle++};
        m_live.insert(kValue);
        *handle = reinterpret_cast<Handle>(kValue);
        return VK_SUCCESS;
    }

    template <typename Handle> void destroyed(Handle handle)
    {
        EXPECT_EQ(m_live.erase(reinterpret_cast<std::uintptr_t>(handle)), 1U) << "destroyed twice or never created";
    }

    /** A real descriptor, so that duplication and closing can be observed. */
    int newFd()
    {
        int fds[2];
        EXPECT_EQ(pipe(fds), 0);
        close(fds[1]);
        m_openFds.push_back(fds[0]);
        return fds[0];
    }

    static bool isOpen(int fd) { return fcntl(fd, F_GETFD) != -1; }

    /** A frame description; planeObjects gives each plane's object. */
    static firebolt::rialto::client::CaptureFrameDescription frame(std::uint32_t drmFormat,
                                                                   const std::vector<std::uint64_t> &planeObjects)
    {
        firebolt::rialto::client::CaptureFrameDescription description;
        description.drmFormat = drmFormat;
        description.backingWidth = 1920;
        description.backingHeight = 1088;
        description.visibleWidth = 1920;
        description.visibleHeight = 1080;
        description.colour = {2, 2, 1, 2}; // BT709, narrow, co-sited, midpoint
        std::set<std::uint64_t> objects;
        for (std::size_t i = 0; i < planeObjects.size(); ++i)
        {
            description.planes.push_back({static_cast<std::uint32_t>(i), planeObjects[i], i * 4096, 4096, 1920});
            if (objects.insert(planeObjects[i]).second)
                description.objects.push_back({planeObjects[i], 1 << 22});
        }
        return description;
    }

    std::shared_ptr<::testing::NiceMock<firebolt::rialto::client::VulkanApiMock>> m_api{
        std::make_shared<::testing::NiceMock<firebolt::rialto::client::VulkanApiMock>>()};
    std::shared_ptr<firebolt::rialto::client::VulkanDeviceContext> m_context{
        firebolt::rialto::client::VulkanDeviceContext::create(m_api, reinterpret_cast<VkPhysicalDevice>(0x10),
                                                              reinterpret_cast<VkDevice>(0x20),
                                                              reinterpret_cast<VkQueue>(0x30), 2,
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)};
    VkFormatFeatureFlags m_modifierFeatures{VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                                            VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT};
    std::uintptr_t m_nextHandle{0x1000};
    std::set<std::uintptr_t> m_live;
    std::vector<int> m_openFds;
    std::vector<int> m_importFds;
    std::size_t m_failAllocation{0}; /**< 1-based allocation that fails; 0: none. */
    VkImageCreateFlags m_lastImageFlags{0};
    VkSamplerYcbcrConversionCreateInfo m_lastConversion{};
    VkSamplerCreateInfo m_lastSampler{};
};

#endif // VULKAN_IMPORT_TEST_BASE_H_
