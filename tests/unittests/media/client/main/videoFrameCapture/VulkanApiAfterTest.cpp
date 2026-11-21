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

// Rialto's header first, then Vulkan: the handle typedefs must agree
#include "IVideoFrameCapture.h"

#include <vulkan/vulkan.h>

#include <gtest/gtest.h>

#include <type_traits>

using namespace firebolt::rialto;

static_assert(std::is_same_v<decltype(VulkanImageResource::image), VkImage>);

TEST(VulkanApiAfterTest, ShouldCompileWithVulkanHeadersAfter)
{
    std::shared_ptr<IVideoFrameCaptureFactory> factory = IVideoFrameCaptureFactory::createFactory();
    if (!factory)
        GTEST_SKIP() << "no capture factory";
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    VkQueue queue{VK_NULL_HANDLE};
    // no Vulkan device here: creation fails cleanly
    EXPECT_EQ(factory->createVulkanVideoFrameCapture(physicalDevice, device, queue, 0,
                                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
              nullptr);
    VulkanImageResource resource{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};
    EXPECT_EQ(resource.ycbcrConversion, VK_NULL_HANDLE);
}
