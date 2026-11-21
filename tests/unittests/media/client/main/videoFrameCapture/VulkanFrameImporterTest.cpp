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
    std::unique_ptr<VulkanFrameImporter> m_sut{std::make_unique<VulkanFrameImporter>(m_context)};
};

TEST_F(VulkanFrameImporterTest, OneObjectWithTwoPlanesImportsOnce)
{
    const int kFd{newFd()};
    bool consumed{false};
    EXPECT_CALL(*m_api, bindImageMemory2(_, 1, _));
    auto image =
        std::dynamic_pointer_cast<VulkanImportedImage>(m_sut->importFrame(frame(kNv12, {7, 7}), {{7, kFd}}, consumed));
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

    m_sut->retire(image);
    m_sut.reset(); // drains, then destroys the cached sampler and conversion
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
        m_sut->importFrame(frame(kYu12, {1, 2, 1}), {{1, kFd1}, {2, kFd2}}, consumed));
    ASSERT_NE(image, nullptr);
    EXPECT_TRUE(consumed);
    EXPECT_NE(m_lastImageFlags & VK_IMAGE_CREATE_DISJOINT_BIT, 0U);
    ASSERT_EQ(m_importFds.size(), 3U);
    EXPECT_EQ(m_importFds[0], kFd1);
    EXPECT_EQ(m_importFds[1], kFd2);
    EXPECT_NE(m_importFds[2], kFd1); // a duplicate, made before kFd1 went to Vulkan
    EXPECT_NE(m_importFds[2], kFd2);
    EXPECT_EQ(image->memories.size(), 3U);
    m_sut->retire(image);
    m_sut.reset(); // drains, then destroys the cached sampler and conversion
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
    EXPECT_EQ(m_sut->importFrame(frame(kNv12, {1, 2}), {{1, kFd1}, {2, kFd2}, {3, kUnused}}, consumed), nullptr);
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
    EXPECT_EQ(m_sut->importFrame(frame(kNv12, {1, 1}), {{1, kFd}}, consumed), nullptr);
    EXPECT_TRUE(consumed);
    EXPECT_FALSE(isOpen(kFd));
    EXPECT_TRUE(m_importFds.empty());
}

TEST_F(VulkanFrameImporterTest, UnsupportedFormatClosesEveryDescriptor)
{
    const int kFd{newFd()};
    EXPECT_CALL(*m_api, createImage(_, _, _)).Times(0);
    bool consumed{false};
    EXPECT_EQ(m_sut->importFrame(frame(0x56595559 /* YUYV */, {1}), {{1, kFd}}, consumed), nullptr);
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
    auto image = m_sut->importFrame(description, {{1, newFd()}}, consumed);
    ASSERT_NE(image, nullptr);
    m_sut->retire(image);
}

TEST_F(VulkanFrameImporterTest, SameDescriptionSharesSamplerAndConversion)
{
    EXPECT_CALL(*m_api, createSampler(_, _, _)).Times(1);
    EXPECT_CALL(*m_api, createSamplerYcbcrConversion(_, _, _)).Times(1);
    bool consumed{false};
    auto first = std::dynamic_pointer_cast<VulkanImportedImage>(
        m_sut->importFrame(frame(kNv12, {1, 1}), {{1, newFd()}}, consumed));
    auto second = std::dynamic_pointer_cast<VulkanImportedImage>(
        m_sut->importFrame(frame(kNv12, {2, 2}), {{2, newFd()}}, consumed));
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first->image, second->image);
    EXPECT_EQ(first->sampler, second->sampler);
    EXPECT_EQ(first->conversion, second->conversion);
    EXPECT_EQ(m_sut->samplerCount(), 1U);
    m_sut->retire(first);
    m_sut->retire(second);
}

TEST_F(VulkanFrameImporterTest, SameDescriptionKeepsHandlesAfterEveryImportIsGone)
{
    // a flush retires every import; the next frame of the same description must keep the same sampler
    EXPECT_CALL(*m_api, createSampler(_, _, _)).Times(1);
    bool consumed{false};
    auto first = std::dynamic_pointer_cast<VulkanImportedImage>(
        m_sut->importFrame(frame(kNv12, {1, 1}), {{1, newFd()}}, consumed));
    ASSERT_NE(first, nullptr);
    const VkSampler kSampler{first->sampler};
    m_sut->retire(first);
    first.reset();
    m_sut->drain();
    EXPECT_EQ(m_live.count(reinterpret_cast<std::uintptr_t>(kSampler)), 1U);
    auto next = std::dynamic_pointer_cast<VulkanImportedImage>(
        m_sut->importFrame(frame(kNv12, {2, 2}), {{2, newFd()}}, consumed));
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->sampler, kSampler);
    m_sut->retire(next);
}

TEST_F(VulkanFrameImporterTest, ColourChangeGivesNewHandlesAndHeldFramesKeepTheirs)
{
    bool consumed{false};
    auto bt709 = std::dynamic_pointer_cast<VulkanImportedImage>(
        m_sut->importFrame(frame(kNv12, {1, 1}), {{1, newFd()}}, consumed));
    ASSERT_NE(bt709, nullptr);
    auto description = frame(kNv12, {2, 2});
    description.colour.matrix = 1; // BT601
    auto bt601 =
        std::dynamic_pointer_cast<VulkanImportedImage>(m_sut->importFrame(description, {{2, newFd()}}, consumed));
    ASSERT_NE(bt601, nullptr);
    EXPECT_NE(bt601->sampler, bt709->sampler);
    EXPECT_NE(bt601->conversion, bt709->conversion);
    EXPECT_EQ(m_lastConversion.ycbcrModel, VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601);

    // the BT709 frame is still held: its sampler survives a drain
    m_sut->drain();
    EXPECT_EQ(m_live.count(reinterpret_cast<std::uintptr_t>(bt709->sampler)), 1U);
    EXPECT_EQ(m_live.count(reinterpret_cast<std::uintptr_t>(bt709->conversion)), 1U);

    // once released, the old description's sampler goes; the current one stays
    const VkSampler kOld{bt709->sampler};
    const VkSamplerYcbcrConversion kOldConversion{bt709->conversion};
    m_sut->retire(bt709);
    bt709.reset();
    m_sut->drain();
    EXPECT_EQ(m_live.count(reinterpret_cast<std::uintptr_t>(kOld)), 0U);
    EXPECT_EQ(m_live.count(reinterpret_cast<std::uintptr_t>(kOldConversion)), 0U);
    EXPECT_EQ(m_live.count(reinterpret_cast<std::uintptr_t>(bt601->sampler)), 1U);
    m_sut->retire(bt601);
}

TEST_F(VulkanFrameImporterTest, ConversionUsesTheFrameColour)
{
    bool consumed{false};
    auto image = m_sut->importFrame(frame(kNv12, {1, 1}), {{1, newFd()}}, consumed);
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(m_lastConversion.format, VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
    EXPECT_EQ(m_lastConversion.ycbcrModel, VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709);
    EXPECT_EQ(m_lastConversion.ycbcrRange, VK_SAMPLER_YCBCR_RANGE_ITU_NARROW);
    EXPECT_EQ(m_lastConversion.xChromaOffset, VK_CHROMA_LOCATION_COSITED_EVEN);
    EXPECT_EQ(m_lastConversion.yChromaOffset, VK_CHROMA_LOCATION_MIDPOINT);
    // the sampler carries the conversion, as an immutable sampler must
    ASSERT_NE(m_lastSampler.pNext, nullptr);
    EXPECT_EQ(m_lastSampler.addressModeU, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    EXPECT_EQ(m_lastSampler.unnormalizedCoordinates, VK_FALSE);
    m_sut->retire(image);
}

TEST_F(VulkanFrameImporterTest, RgbHasANullConversion)
{
    m_modifierFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    EXPECT_CALL(*m_api, createSamplerYcbcrConversion(_, _, _)).Times(0);
    bool consumed{false};
    auto image =
        std::dynamic_pointer_cast<VulkanImportedImage>(m_sut->importFrame(frame(kAr24, {1}), {{1, newFd()}}, consumed));
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->format, VK_FORMAT_B8G8R8A8_UNORM);
    EXPECT_NE(image->sampler, VK_NULL_HANDLE);
    EXPECT_EQ(image->conversion, VK_NULL_HANDLE);
    EXPECT_EQ(m_lastSampler.pNext, nullptr);
    EXPECT_EQ(m_lastSampler.magFilter, VK_FILTER_LINEAR);
    m_sut->retire(image);
}

TEST_F(VulkanFrameImporterTest, FiltersAreLinearOnlyWhenTheFormatReportsIt)
{
    m_modifierFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT; // no YCbCr linear filtering
    bool consumed{false};
    auto nearest = m_sut->importFrame(frame(kNv12, {1, 1}), {{1, newFd()}}, consumed);
    ASSERT_NE(nearest, nullptr);
    EXPECT_EQ(m_lastSampler.magFilter, VK_FILTER_NEAREST);
    EXPECT_EQ(m_lastSampler.minFilter, VK_FILTER_NEAREST);
    EXPECT_EQ(m_lastConversion.chromaFilter, VK_FILTER_NEAREST);
    m_sut->retire(nearest);

    m_modifierFeatures |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT;
    auto description = frame(kNv12, {2, 2});
    description.colour.range = 1; // a new description, so a new sampler
    auto linear = m_sut->importFrame(description, {{2, newFd()}}, consumed);
    ASSERT_NE(linear, nullptr);
    EXPECT_EQ(m_lastSampler.magFilter, VK_FILTER_LINEAR);
    EXPECT_EQ(m_lastConversion.chromaFilter, VK_FILTER_LINEAR);
    EXPECT_EQ(m_lastConversion.ycbcrRange, VK_SAMPLER_YCBCR_RANGE_ITU_FULL);
    m_sut->retire(linear);
}

TEST_F(VulkanFrameImporterTest, ViewFailureDestroysANewSampler)
{
    EXPECT_CALL(*m_api, createImageView(_, _, _)).WillOnce(::testing::Return(VK_ERROR_OUT_OF_HOST_MEMORY));
    bool consumed{false};
    EXPECT_EQ(m_sut->importFrame(frame(kNv12, {1, 1}), {{1, newFd()}}, consumed), nullptr);
    EXPECT_TRUE(m_live.empty());
    EXPECT_EQ(m_sut->samplerCount(), 0U);
}
