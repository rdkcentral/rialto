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

#include "CaptureConfiguration.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <tuple>
#include <vector>

using firebolt::rialto::server::captureTargetSlots;
using firebolt::rialto::server::chooseCaptureSlotCount;

namespace
{
constexpr const char *kTargetSlots{"RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS"};
} // namespace

class CaptureSlotCountTest : public ::testing::Test
{
protected:
    void TearDown() override { unsetenv(kTargetSlots); }
};

TEST_F(CaptureSlotCountTest, SlotCountTable)
{
    // HAL range [minimumSlots, maximumSlots], T, expected N (roadmap A3 + A22, option C)
    const std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t>>
        kTable{{1, 4, 3, 3}, {3, 4, 3, 3}, {1, 2, 3, 2}, {4, 6, 3, 4},
               {2, 2, 3, 2}, {5, 5, 3, 5}, {1, 4, 2, 2}, {1, 4, 99, 4}};
    for (const auto &[kMinimum, kMaximum, kTarget, kExpected] : kTable)
    {
        EXPECT_EQ(chooseCaptureSlotCount(kMinimum, kMaximum, kTarget), kExpected)
            << "[" << kMinimum << "," << kMaximum << "] T=" << kTarget;
    }
}

TEST_F(CaptureSlotCountTest, SingleSlotHalCannotBeAdmitted)
{
    EXPECT_EQ(chooseCaptureSlotCount(1, 1, 3), 0u);
    EXPECT_EQ(chooseCaptureSlotCount(1, 1, 1), 0u);
}

TEST_F(CaptureSlotCountTest, TargetDefaultsToThree)
{
    unsetenv(kTargetSlots);
    EXPECT_EQ(captureTargetSlots(), 3u);
}

TEST_F(CaptureSlotCountTest, ValidTargetIsUsed)
{
    setenv(kTargetSlots, "2", 1);
    EXPECT_EQ(captureTargetSlots(), 2u);
    setenv(kTargetSlots, "99", 1);
    EXPECT_EQ(captureTargetSlots(), 99u); // no upper cap; the clamp to maximumSlots bounds it
}

TEST_F(CaptureSlotCountTest, InvalidTargetFallsBackToThree)
{
    for (const char *kValue : {"abc", "1", "0", "-4", "3x"})
    {
        setenv(kTargetSlots, kValue, 1);
        EXPECT_EQ(captureTargetSlots(), 3u) << kValue;
        EXPECT_EQ(chooseCaptureSlotCount(1, 4, captureTargetSlots()), 3u) << kValue;
    }
}
