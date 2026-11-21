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

#include "GlFrameImporter.h"

#include <gtest/gtest.h>

#include <map>

using namespace firebolt::rialto::client;
using namespace firebolt::rialto::client::egl;

namespace
{
constexpr std::uint32_t kNv12{0x3231564e};
constexpr std::uint64_t kModifier{0x0300000000606014}; // distinct halves

// The attribute list as name -> value, checking each name appears once and the list ends with EGL_NONE.
std::map<EGLint, EGLint> toMap(const std::vector<EGLint> &attributes)
{
    std::map<EGLint, EGLint> values;
    EXPECT_FALSE(attributes.empty());
    EXPECT_EQ(attributes.size() % 2, 1U);
    EXPECT_EQ(attributes.back(), EGL_NONE);
    for (std::size_t i = 0; i + 1 < attributes.size(); i += 2)
        EXPECT_TRUE(values.emplace(attributes[i], attributes[i + 1]).second) << "repeated " << attributes[i];
    return values;
}

CaptureFrameDescription nv12(std::vector<CapturePlane> planes)
{
    CaptureFrameDescription description;
    description.drmFormat = kNv12;
    description.drmModifier = kModifier;
    description.backingWidth = 1920;
    description.backingHeight = 1088;
    description.planes = std::move(planes);
    return description;
}
} // namespace

TEST(BuildEglAttributesTest, ExampleBSharesOneDescriptor)
{
    const auto kDescription = nv12({{0, 101, 0, 1920 * 1088, 1920}, {1, 101, 1920 * 1088, 1920 * 544, 1920}});
    const auto kAttributes = buildEglAttributes(kDescription, {{101, 10}});
    ASSERT_TRUE(kAttributes);
    auto values = toMap(*kAttributes);
    EXPECT_EQ(values[EGL_WIDTH], 1920);
    EXPECT_EQ(values[EGL_HEIGHT], 1088);
    EXPECT_EQ(values[EGL_LINUX_DRM_FOURCC_EXT], static_cast<EGLint>(kNv12));
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE0_FD_EXT], 10);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE1_FD_EXT], 10); // repeated
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE0_OFFSET_EXT], 0);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE1_OFFSET_EXT], 1920 * 1088);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE0_PITCH_EXT], 1920);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE1_PITCH_EXT], 1920);
}

TEST(BuildEglAttributesTest, ExampleCUsesEachObjectsDescriptor)
{
    const auto kDescription = nv12({{0, 101, 0, 1920 * 1088, 2048}, {1, 205, 0, 1920 * 544, 1024}});
    const auto kAttributes = buildEglAttributes(kDescription, {{101, 10}, {205, 11}});
    ASSERT_TRUE(kAttributes);
    auto values = toMap(*kAttributes);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE0_FD_EXT], 10);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE1_FD_EXT], 11);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE0_OFFSET_EXT], 0);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE1_OFFSET_EXT], 0);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE0_PITCH_EXT], 2048);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE1_PITCH_EXT], 1024);
}

TEST(BuildEglAttributesTest, ModifierOnEveryPlane)
{
    const auto kDescription = nv12({{0, 101, 0, 100, 64}, {1, 205, 0, 100, 64}});
    auto values = toMap(*buildEglAttributes(kDescription, {{101, 10}, {205, 11}}));
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT], 0x00606014);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT], 0x03000000);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT], 0x00606014);
    EXPECT_EQ(values[EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT], 0x03000000);
    EXPECT_EQ(values.count(EGL_DMA_BUF_PLANE2_FD_EXT), 0U);
}

TEST(BuildEglAttributesTest, ColourHintsForEveryValue)
{
    struct Case
    {
        CaptureColour colour;
        EGLint space;
        EGLint range;
        EGLint horizontal;
        EGLint vertical;
    };
    const Case kCases[]{
        {{1, 1, 1, 1}, EGL_ITU_REC601_EXT, EGL_YUV_FULL_RANGE_EXT, EGL_YUV_CHROMA_SITING_0_EXT, EGL_YUV_CHROMA_SITING_0_EXT},
        {{2, 2, 1, 2}, EGL_ITU_REC709_EXT, EGL_YUV_NARROW_RANGE_EXT, EGL_YUV_CHROMA_SITING_0_EXT, EGL_YUV_CHROMA_SITING_0_5_EXT},
        {{3, 2, 2, 2},
         EGL_ITU_REC2020_EXT,
         EGL_YUV_NARROW_RANGE_EXT,
         EGL_YUV_CHROMA_SITING_0_5_EXT,
         EGL_YUV_CHROMA_SITING_0_5_EXT},
    };
    for (const Case &kCase : kCases)
    {
        auto description = nv12({{0, 101, 0, 100, 64}});
        description.colour = kCase.colour;
        auto values = toMap(*buildEglAttributes(description, {{101, 10}}));
        EXPECT_EQ(values[EGL_YUV_COLOR_SPACE_HINT_EXT], kCase.space);
        EXPECT_EQ(values[EGL_SAMPLE_RANGE_HINT_EXT], kCase.range);
        EXPECT_EQ(values[EGL_YUV_CHROMA_HORIZONTAL_SITING_HINT_EXT], kCase.horizontal);
        EXPECT_EQ(values[EGL_YUV_CHROMA_VERTICAL_SITING_HINT_EXT], kCase.vertical);
    }
}

TEST(BuildEglAttributesTest, UnknownColourGivesNoHints)
{
    auto description = nv12({{0, 101, 0, 100, 64}});
    description.colour = {0, 0, 0, 0}; // an RGB format
    auto values = toMap(*buildEglAttributes(description, {{101, 10}}));
    EXPECT_EQ(values.count(EGL_YUV_COLOR_SPACE_HINT_EXT), 0U);
    EXPECT_EQ(values.count(EGL_SAMPLE_RANGE_HINT_EXT), 0U);
    EXPECT_EQ(values.count(EGL_YUV_CHROMA_HORIZONTAL_SITING_HINT_EXT), 0U);
    EXPECT_EQ(values.count(EGL_YUV_CHROMA_VERTICAL_SITING_HINT_EXT), 0U);
}

TEST(BuildEglAttributesTest, RejectsMissingDescriptorAndBadPlanes)
{
    EXPECT_FALSE(buildEglAttributes(nv12({{0, 101, 0, 100, 64}, {1, 205, 0, 100, 64}}), {{101, 10}}));
    EXPECT_FALSE(buildEglAttributes(nv12({{0, 101, 0, 100, 64}}), {{101, -1}}));
    EXPECT_FALSE(buildEglAttributes(nv12({{0, 101, 0, 100, 64}, {0, 101, 100, 100, 64}}), {{101, 10}}));
    EXPECT_FALSE(buildEglAttributes(nv12({{4, 101, 0, 100, 64}}), {{101, 10}}));
    EXPECT_FALSE(buildEglAttributes(nv12({}), {{101, 10}}));
    EXPECT_FALSE(buildEglAttributes(nv12({{0, 101, 0x100000000ULL, 100, 64}}), {{101, 10}}));
}
