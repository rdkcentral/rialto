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

#include "VideoFrameCaptureRegistry.h"
#include "VideoFrameCaptureService.h"

#include <gtest/gtest.h>

#include <set>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server::service;

namespace
{
constexpr VideoFrameCaptureClientId kClientA{1};
constexpr VideoFrameCaptureClientId kClientB{2};
} // namespace

TEST(VideoFrameCaptureRegistryTest, ShouldGiveUniqueNonZeroIdsAndNeverReuseThem)
{
    VideoFrameCaptureRegistry registry;
    const std::uint32_t kFirst = registry.add(GraphicsAPIType::GL);
    const std::uint32_t kSecond = registry.add(GraphicsAPIType::VULKAN);
    EXPECT_NE(kFirst, 0U);
    EXPECT_NE(kSecond, 0U);
    EXPECT_NE(kFirst, kSecond);

    EXPECT_TRUE(registry.remove(kSecond));
    const std::uint32_t kThird = registry.add(GraphicsAPIType::GL);
    EXPECT_NE(kThird, kFirst);
    EXPECT_NE(kThird, kSecond); // a destroyed ID is not handed out again

    ASSERT_NE(registry.find(kFirst), nullptr);
    EXPECT_EQ(registry.find(kFirst)->api, GraphicsAPIType::GL);
    EXPECT_EQ(registry.find(kSecond), nullptr);
    EXPECT_FALSE(registry.remove(kSecond));
    EXPECT_EQ(registry.size(), 2U);
}

TEST(VideoFrameCaptureServiceTest, ShouldRegisterCapturesPerClient)
{
    VideoFrameCaptureService service;
    std::uint32_t first{0}, second{0};
    EXPECT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, first), VideoFrameCaptureServiceStatus::OK);
    EXPECT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, second), VideoFrameCaptureServiceStatus::OK);
    EXPECT_NE(first, 0U);
    EXPECT_NE(first, second);
    EXPECT_TRUE(service.hasCapture(kClientA, first));
    EXPECT_TRUE(service.hasCapture(kClientA, second));
}

TEST(VideoFrameCaptureServiceTest, ShouldNotFindCaptureOfAnotherClient)
{
    VideoFrameCaptureService service;
    std::uint32_t captureId{0};
    ASSERT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, captureId), VideoFrameCaptureServiceStatus::OK);

    EXPECT_FALSE(service.hasCapture(kClientB, captureId));
    EXPECT_FALSE(service.destroyCapture(kClientB, captureId));
    EXPECT_TRUE(service.hasCapture(kClientA, captureId)); // unaffected
}

TEST(VideoFrameCaptureServiceTest, ShouldDestroyOnceAndNotFindDestroyedId)
{
    VideoFrameCaptureService service;
    std::uint32_t captureId{0};
    ASSERT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, captureId), VideoFrameCaptureServiceStatus::OK);

    EXPECT_TRUE(service.destroyCapture(kClientA, captureId));
    EXPECT_FALSE(service.hasCapture(kClientA, captureId));
    EXPECT_FALSE(service.destroyCapture(kClientA, captureId));
    EXPECT_FALSE(service.hasCapture(kClientA, 12345));
}

TEST(VideoFrameCaptureServiceTest, ShouldRemoveOnlyTheDisconnectedClientsCaptures)
{
    VideoFrameCaptureService service;
    std::uint32_t a1{0}, a2{0}, b1{0};
    service.createCapture(kClientA, GraphicsAPIType::GL, a1);
    service.createCapture(kClientA, GraphicsAPIType::GL, a2);
    service.createCapture(kClientB, GraphicsAPIType::GL, b1);

    service.removeClient(kClientA);

    EXPECT_FALSE(service.hasCapture(kClientA, a1));
    EXPECT_FALSE(service.hasCapture(kClientA, a2));
    EXPECT_TRUE(service.hasCapture(kClientB, b1));
}
