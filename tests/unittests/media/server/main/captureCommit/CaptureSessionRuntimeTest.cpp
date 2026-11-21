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

#include "CaptureSessionRuntime.h"
#include "CaptureSessionMock.h"

#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using ::testing::NiceMock;

namespace
{
capture::CapturePool poolOf(std::uint32_t slots)
{
    capture::CapturePool pool;
    pool.backingSize = {1920, 1088};
    for (std::uint32_t slot = 0; slot < slots; ++slot)
        pool.slotLayouts.push_back(capture::CaptureSlotLayout{slot, {}});
    return pool;
}

wrappers::CaptureSessionPtr sessionTracking(bool &deleted)
{
    return wrappers::CaptureSessionPtr{new NiceMock<capture::CaptureSessionMock>(),
                                       [&deleted](capture::ICaptureSession *session)
                                       {
                                           deleted = true;
                                           delete session;
                                       }};
}
} // namespace

TEST(CaptureSessionRuntimeTest, ShouldKeepOneEmptySlotPerAdmittedSlot)
{
    bool deleted{false};
    {
        CaptureSessionRuntime runtime{sessionTracking(deleted), 4, poolOf(3)};
        EXPECT_EQ(runtime.poolId(), 4U);
        EXPECT_EQ(runtime.slotCount(), 3U);
        EXPECT_EQ(runtime.pool().backingSize.width, 1920U);
        for (std::uint32_t slot = 0; slot < 4; ++slot)
            EXPECT_FALSE(runtime.isSlotLocked(slot)); // slot 3 is outside the pool
        EXPECT_FALSE(runtime.isFatal());
        EXPECT_FALSE(runtime.isFlushPending());
        EXPECT_FALSE(deleted);
    }
    EXPECT_TRUE(deleted); // the runtime owns the HAL session
}
