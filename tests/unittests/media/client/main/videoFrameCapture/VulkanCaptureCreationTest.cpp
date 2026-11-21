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

#include "EglLibraryMock.h"
#include "VideoFrameCaptureFactory.h"
#include "VideoFrameCaptureIpcMock.h"
#include "VulkanApiMock.h"

#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;

/**
 * Creating a Vulkan capture checks Vulkan and the device before anything is registered with the server (R13 2.1).
 */
class VulkanCaptureCreationTest : public ::testing::Test
{
protected:
    // The device reports this API version and samplerYcbcrConversion support.
    void deviceIs(uint32_t apiVersion, bool ycbcrConversion)
    {
        ON_CALL(*m_api, getPhysicalDeviceProperties2(m_physicalDevice, _))
            .WillByDefault(Invoke([apiVersion](VkPhysicalDevice, VkPhysicalDeviceProperties2 *properties)
                                  { properties->properties.apiVersion = apiVersion; }));
        ON_CALL(*m_api, getPhysicalDeviceFeatures2(m_physicalDevice, _))
            .WillByDefault(Invoke(
                [ycbcrConversion](VkPhysicalDevice, VkPhysicalDeviceFeatures2 *features)
                {
                    auto *ycbcr = static_cast<VkPhysicalDeviceSamplerYcbcrConversionFeatures *>(features->pNext);
                    ASSERT_NE(ycbcr, nullptr);
                    ASSERT_EQ(ycbcr->sType, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES);
                    ycbcr->samplerYcbcrConversion = ycbcrConversion ? VK_TRUE : VK_FALSE;
                }));
    }

    std::shared_ptr<IVulkanVideoFrameCapture> create()
    {
        return m_sut.createVulkanVideoFrameCapture(m_physicalDevice, m_device, m_queue, 2,
                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    VkPhysicalDevice m_physicalDevice{reinterpret_cast<VkPhysicalDevice>(0x10)};
    VkDevice m_device{reinterpret_cast<VkDevice>(0x20)};
    VkQueue m_queue{reinterpret_cast<VkQueue>(0x30)};
    std::shared_ptr<NiceMock<VulkanApiMock>> m_api{std::make_shared<NiceMock<VulkanApiMock>>()};
    std::shared_ptr<StrictMock<VulkanLibraryMock>> m_vulkan{std::make_shared<StrictMock<VulkanLibraryMock>>()};
    // strict: any IPC use, that is any registration, fails the test
    std::shared_ptr<StrictMock<VideoFrameCaptureIpcFactoryMock>> m_ipcFactory{
        std::make_shared<StrictMock<VideoFrameCaptureIpcFactoryMock>>()};
    VideoFrameCaptureFactory m_sut{std::make_shared<StrictMock<EglLibraryMock>>(), m_ipcFactory, m_vulkan};
};

TEST_F(VulkanCaptureCreationTest, NoVulkanLibraryOrMissingCommandGivesNoCapture)
{
    // deviceApi() is null both when libvulkan cannot be loaded and when a required device command such as
    // vkGetMemoryFdPropertiesKHR is null (VulkanLibraryTest covers both causes)
    EXPECT_CALL(*m_vulkan, deviceApi(m_device)).WillOnce(Return(nullptr));
    EXPECT_EQ(create(), nullptr);
}

TEST_F(VulkanCaptureCreationTest, Vulkan10DeviceGivesNoCapture)
{
    deviceIs(VK_API_VERSION_1_0, true);
    EXPECT_CALL(*m_vulkan, deviceApi(m_device)).WillOnce(Return(m_api));
    EXPECT_EQ(create(), nullptr);
}

TEST_F(VulkanCaptureCreationTest, MissingSamplerYcbcrConversionGivesNoCapture)
{
    deviceIs(VK_API_VERSION_1_1, false);
    EXPECT_CALL(*m_vulkan, deviceApi(m_device)).WillOnce(Return(m_api));
    EXPECT_EQ(create(), nullptr);
}

TEST_F(VulkanCaptureCreationTest, NoVulkanLoaderGivesNoCapture)
{
    VideoFrameCaptureFactory withoutVulkan{std::make_shared<StrictMock<EglLibraryMock>>(), m_ipcFactory};
    EXPECT_EQ(withoutVulkan.createVulkanVideoFrameCapture(m_physicalDevice, m_device, m_queue, 2,
                                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
              nullptr);
}
