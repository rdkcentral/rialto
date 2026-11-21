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

#include "VulkanImportTestBase.h"

using namespace firebolt::rialto::client;
using ::testing::_;

class VulkanFrameImporterTest : public VulkanImportTestBase
{
protected:
    VulkanFrameImporter m_sut{m_context};
};

TEST_F(VulkanFrameImporterTest, OneObjectWithTwoPlanesImportsOnce)
{
    const int kFd{newFd()};
    bool consumed{false};
    EXPECT_CALL(*m_api, bindImageMemory2(_, 1, _));
    auto image =
        std::dynamic_pointer_cast<VulkanImportedImage>(m_sut.importFrame(frame(kNv12, {7, 7}), {{7, kFd}}, consumed));
    ASSERT_NE(image, nullptr);
    EXPECT_TRUE(consumed);
    ASSERT_EQ(m_importFds.size(), 1U);
    EXPECT_EQ(m_importFds[0], kFd); // the received descriptor itself, consumed by the import
    EXPECT_EQ(m_lastImageFlags & VK_IMAGE_CREATE_DISJOINT_BIT, 0U);
    EXPECT_EQ(image->format, VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
    EXPECT_NE(image->image, VK_NULL_HANDLE);
    EXPECT_NE(image->view, VK_NULL_HANDLE);
    EXPECT_NE(image->sampler, VK_NULL_HANDLE);
    EXPECT_NE(image->conversion, VK_NULL_HANDLE);
    EXPECT_EQ(image->memories.size(), 1U);

    m_sut.retire(image);
    m_sut.drain();
    EXPECT_TRUE(m_live.empty());
}

TEST_F(VulkanFrameImporterTest, ObjectImportedTwiceUsesADuplicatedDescriptor)
{
    // three planes in two objects: disjoint, so object 1 is imported for planes 0 and 2
    const int kFd1{newFd()};
    const int kFd2{newFd()};
    bool consumed{false};
    EXPECT_CALL(*m_api, bindImageMemory2(_, 3, _));
    auto image = std::dynamic_pointer_cast<VulkanImportedImage>(
        m_sut.importFrame(frame(kYu12, {1, 2, 1}), {{1, kFd1}, {2, kFd2}}, consumed));
    ASSERT_NE(image, nullptr);
    EXPECT_TRUE(consumed);
    EXPECT_NE(m_lastImageFlags & VK_IMAGE_CREATE_DISJOINT_BIT, 0U);
    ASSERT_EQ(m_importFds.size(), 3U);
    EXPECT_EQ(m_importFds[0], kFd1);
    EXPECT_EQ(m_importFds[1], kFd2);
    EXPECT_NE(m_importFds[2], kFd1); // a duplicate, made before kFd1 went to Vulkan
    EXPECT_NE(m_importFds[2], kFd2);
    EXPECT_EQ(image->memories.size(), 3U);
    m_sut.retire(image);
    m_sut.drain();
    EXPECT_TRUE(m_live.empty());
}

TEST_F(VulkanFrameImporterTest, AllocationFailureClosesItsDescriptorAndDestroysEverything)
{
    const int kFd1{newFd()};
    const int kFd2{newFd()};
    const int kUnused{newFd()};
    m_failAllocation = 2;
    bool consumed{false};
    EXPECT_CALL(*m_api, bindImageMemory2(_, _, _)).Times(0);
    EXPECT_CALL(*m_api, createImageView(_, _, _)).Times(0);
    EXPECT_EQ(m_sut.importFrame(frame(kNv12, {1, 2}), {{1, kFd1}, {2, kFd2}, {3, kUnused}}, consumed), nullptr);
    EXPECT_TRUE(consumed); // closed or consumed here; the caller must not close them again
    EXPECT_FALSE(isOpen(kFd2)) << "the failed import's descriptor is closed";
    EXPECT_FALSE(isOpen(kUnused)) << "a descriptor no plane uses is closed";
    EXPECT_TRUE(m_live.empty()) << "the image and the first memory are destroyed";
}

TEST_F(VulkanFrameImporterTest, ImageFailureClosesEveryDescriptor)
{
    const int kFd{newFd()};
    EXPECT_CALL(*m_api, createImage(_, _, _)).WillOnce(::testing::Return(VK_ERROR_OUT_OF_DEVICE_MEMORY));
    bool consumed{false};
    EXPECT_EQ(m_sut.importFrame(frame(kNv12, {1, 1}), {{1, kFd}}, consumed), nullptr);
    EXPECT_TRUE(consumed);
    EXPECT_FALSE(isOpen(kFd));
    EXPECT_TRUE(m_importFds.empty());
}

TEST_F(VulkanFrameImporterTest, UnsupportedFormatClosesEveryDescriptor)
{
    const int kFd{newFd()};
    EXPECT_CALL(*m_api, createImage(_, _, _)).Times(0);
    bool consumed{false};
    EXPECT_EQ(m_sut.importFrame(frame(0x56595559 /* YUYV */, {1}), {{1, kFd}}, consumed), nullptr);
    EXPECT_FALSE(isOpen(kFd));
}

TEST_F(VulkanFrameImporterTest, ShouldUsePlaneLayoutsAndModifier)
{
    auto description = frame(kNv12, {1, 1});
    description.drmModifier = 0;
    description.planes[1].offsetBytes = 1920 * 1088;
    EXPECT_CALL(*m_api, createImage(_, _, _))
        .WillOnce(::testing::Invoke(
            [this](VkDevice, const VkImageCreateInfo *info, VkImage *image)
            {
                EXPECT_EQ(info->tiling, VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT);
                EXPECT_EQ(info->extent.width, 1920U);
                EXPECT_EQ(info->extent.height, 1088U);
                const auto *kExternal = static_cast<const VkExternalMemoryImageCreateInfo *>(info->pNext);
                EXPECT_EQ(kExternal->handleTypes, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT);
                const auto *kModifier =
                    static_cast<const VkImageDrmFormatModifierExplicitCreateInfoEXT *>(kExternal->pNext);
                EXPECT_EQ(kModifier->drmFormatModifierPlaneCount, 2U);
                EXPECT_EQ(kModifier->pPlaneLayouts[1].offset, 1920U * 1088U);
                EXPECT_EQ(kModifier->pPlaneLayouts[1].rowPitch, 1920U);
                return created(image);
            }));
    bool consumed{false};
    auto image = m_sut.importFrame(description, {{1, newFd()}}, consumed);
    ASSERT_NE(image, nullptr);
    m_sut.retire(image);
}
