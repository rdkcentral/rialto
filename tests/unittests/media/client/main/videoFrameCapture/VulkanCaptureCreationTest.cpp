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
#include "VulkanDeviceContext.h"
#include "VulkanVideoFrameCapture.h"

#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgReferee;
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

TEST_F(VulkanCaptureCreationTest, RegistersAsVulkanWithACommandPoolOnTheGivenFamily)
{
    deviceIs(VK_API_VERSION_1_1, true);
    EXPECT_CALL(*m_vulkan, deviceApi(m_device)).WillOnce(Return(m_api));
    const VkCommandPool kPool{reinterpret_cast<VkCommandPool>(0x40)};
    EXPECT_CALL(*m_api, createCommandPool(m_device, _, _))
        .WillOnce(Invoke(
            [kPool](VkDevice, const VkCommandPoolCreateInfo *info, VkCommandPool *pool)
            {
                EXPECT_EQ(info->queueFamilyIndex, 2U);
                *pool = kPool;
                return VK_SUCCESS;
            }));
    EXPECT_CALL(*m_api, allocateCommandBuffers(m_device, _, _))
        .WillOnce(Invoke(
            [kPool](VkDevice, const VkCommandBufferAllocateInfo *info, VkCommandBuffer *buffers)
            {
                EXPECT_EQ(info->commandPool, kPool);
                for (uint32_t i = 0; i < info->commandBufferCount; ++i)
                    buffers[i] = reinterpret_cast<VkCommandBuffer>(0x100 + i);
                return VK_SUCCESS;
            }));
    int fence{0};
    EXPECT_CALL(*m_api, createFence(m_device, _, _))
        .Times(VulkanDeviceContext::kRingSize)
        .WillRepeatedly(Invoke(
            [&fence](VkDevice, const VkFenceCreateInfo *info, VkFence *created)
            {
                EXPECT_TRUE(info->flags & VK_FENCE_CREATE_SIGNALED_BIT);
                *created = reinterpret_cast<VkFence>(0x200 + ++fence);
                return VK_SUCCESS;
            }));
    auto ipc = std::make_shared<StrictMock<VideoFrameCaptureIpcMock>>();
    EXPECT_CALL(*m_ipcFactory, createVideoFrameCaptureIpc()).WillOnce(Return(ipc));
    EXPECT_CALL(*ipc, createVideoFrameCapture(GraphicsAPIType::VULKAN, _))
        .WillOnce(DoAll(SetArgReferee<1>(7U), Return(true)));

    auto capture = create();
    ASSERT_NE(capture, nullptr);
    EXPECT_EQ(capture->apiType(), GraphicsAPIType::VULKAN);
    auto *vulkanCapture = dynamic_cast<VulkanVideoFrameCapture *>(capture.get());
    ASSERT_NE(vulkanCapture, nullptr);
    EXPECT_EQ(vulkanCapture->context()->queueFamilyIndex(), 2U);
    EXPECT_EQ(vulkanCapture->context()->commandPool(), kPool);

    // destruction: unregistered, then the ring and pool destroyed after their fences
    EXPECT_CALL(*ipc, destroyVideoFrameCapture(7U)).WillOnce(Return(true));
    EXPECT_CALL(*m_api, waitForFences(m_device, VulkanDeviceContext::kRingSize, _, VK_TRUE, _)).WillOnce(Return(VK_SUCCESS));
    EXPECT_CALL(*m_api, destroyFence(m_device, _)).Times(VulkanDeviceContext::kRingSize);
    EXPECT_CALL(*m_api, freeCommandBuffers(m_device, kPool, VulkanDeviceContext::kRingSize, _));
    EXPECT_CALL(*m_api, destroyCommandPool(m_device, kPool));
    capture.reset();
}

TEST_F(VulkanCaptureCreationTest, CommandPoolFailureRegistersNothing)
{
    deviceIs(VK_API_VERSION_1_1, true);
    EXPECT_CALL(*m_vulkan, deviceApi(m_device)).WillOnce(Return(m_api));
    EXPECT_CALL(*m_api, createCommandPool(m_device, _, _)).WillOnce(Return(VK_ERROR_OUT_OF_DEVICE_MEMORY));
    EXPECT_CALL(*m_api, destroyCommandPool(_, _)).Times(0);
    EXPECT_EQ(create(), nullptr); // no IPC: the strict IPC factory would flag it
}
