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

#include "VulkanVideoFrame.h"
#include "VulkanFrameImporter.h"

#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

TEST(VulkanVideoFrameTest, ReportsTheImportAndTheRequiredLayout)
{
    auto image = std::make_shared<VulkanImportedImage>();
    image->format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    image->image = reinterpret_cast<VkImage>(0x100);
    image->view = reinterpret_cast<VkImageView>(0x200);
    image->sampler = reinterpret_cast<VkSampler>(0x300);
    image->conversion = reinterpret_cast<VkSamplerYcbcrConversion>(0x400);
    CaptureFrameDescription description;
    description.slotIndex = 2;
    description.presentationTimeNs = 1234;
    description.backingWidth = 1920;
    description.backingHeight = 1088;
    description.visibleWidth = 1920;
    description.visibleHeight = 1080;

    const VulkanVideoFrame kFrame{description, std::make_shared<ImportCacheEntry>(ImportCacheKey{}, image, nullptr),
                                  nullptr, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    EXPECT_EQ(kFrame.imageFormat(), VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
    EXPECT_EQ(kFrame.imageLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EXPECT_EQ(kFrame.layout(), GraphicsTextureLayout::RGBA);
    EXPECT_EQ(kFrame.numResources(), 1U);
    ASSERT_EQ(kFrame.images().size(), 1U);
    EXPECT_EQ(kFrame.images()[0].image, image->image);
    EXPECT_EQ(kFrame.images()[0].imageView, image->view);
    EXPECT_EQ(kFrame.images()[0].sampler, image->sampler);
    EXPECT_EQ(kFrame.images()[0].ycbcrConversion, image->conversion);
    EXPECT_EQ(kFrame.frameIndex(), 2U);
    EXPECT_EQ(kFrame.presentationTimeStampNs(), 1234);
    EXPECT_EQ(kFrame.textureWidth(), 1920U);
    EXPECT_EQ(kFrame.textureHeight(), 1088U);
    EXPECT_EQ(kFrame.visibleHeight(), 1080U);
}

TEST(VulkanVideoFrameTest, RgbFrameHasANullConversion)
{
    auto image = std::make_shared<VulkanImportedImage>();
    image->format = VK_FORMAT_B8G8R8A8_UNORM;
    image->image = reinterpret_cast<VkImage>(0x100);
    image->sampler = reinterpret_cast<VkSampler>(0x300);
    const VulkanVideoFrame kFrame{{},
                                  std::make_shared<ImportCacheEntry>(ImportCacheKey{}, image, nullptr),
                                  nullptr,
                                  VK_IMAGE_LAYOUT_GENERAL};
    EXPECT_EQ(kFrame.imageFormat(), VK_FORMAT_B8G8R8A8_UNORM);
    EXPECT_EQ(kFrame.imageLayout(), VK_IMAGE_LAYOUT_GENERAL);
    ASSERT_EQ(kFrame.images().size(), 1U);
    EXPECT_NE(kFrame.images()[0].sampler, VK_NULL_HANDLE);
    EXPECT_EQ(kFrame.images()[0].ycbcrConversion, VK_NULL_HANDLE);
}
