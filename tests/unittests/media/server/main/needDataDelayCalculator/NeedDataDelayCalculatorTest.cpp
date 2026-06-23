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

#include "NeedDataDelayCalculator.h"
#include <gtest/gtest.h>

namespace
{
constexpr std::chrono::milliseconds kDefaultNeedMediaDataResendTimeMs{15};
constexpr std::chrono::milliseconds kIncreasedNeedMediaDataResendTimeMs{30};
} // namespace

TEST(NeedDataDelayCalculatorTest, ShouldReturnDefaultDelayWhenNoDelayIsSet)
{
    firebolt::rialto::server::NeedDataDelayCalculator delayCalculator;
    EXPECT_EQ(delayCalculator.getNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO),
              kDefaultNeedMediaDataResendTimeMs);
}

TEST(NeedDataDelayCalculatorTest, ShouldIncreaseAndDecreaseDelay)
{
    firebolt::rialto::server::NeedDataDelayCalculator delayCalculator;
    delayCalculator.increaseNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO);
    EXPECT_EQ(delayCalculator.getNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO),
              kDefaultNeedMediaDataResendTimeMs);
    delayCalculator.increaseNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO);
    EXPECT_EQ(delayCalculator.getNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO),
              kIncreasedNeedMediaDataResendTimeMs);
    delayCalculator.decreaseNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO);
    EXPECT_EQ(delayCalculator.getNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO),
              kDefaultNeedMediaDataResendTimeMs);
}

TEST(NeedDataDelayCalculatorTest, ShouldIncreaseAndResetDelay)
{
    firebolt::rialto::server::NeedDataDelayCalculator delayCalculator;
    delayCalculator.increaseNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO);
    EXPECT_EQ(delayCalculator.getNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO),
              kDefaultNeedMediaDataResendTimeMs);
    delayCalculator.increaseNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO);
    EXPECT_EQ(delayCalculator.getNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO),
              kIncreasedNeedMediaDataResendTimeMs);
    delayCalculator.resetMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO);
    EXPECT_EQ(delayCalculator.getNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO),
              kDefaultNeedMediaDataResendTimeMs);
}

TEST(NeedDataDelayCalculatorTest, ShouldIncreaseAndResetAllDelays)
{
    firebolt::rialto::server::NeedDataDelayCalculator delayCalculator;
    delayCalculator.increaseNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO);
    EXPECT_EQ(delayCalculator.getNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO),
              kDefaultNeedMediaDataResendTimeMs);
    delayCalculator.increaseNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO);
    EXPECT_EQ(delayCalculator.getNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO),
              kIncreasedNeedMediaDataResendTimeMs);
    delayCalculator.resetMediaDataDelay();
    EXPECT_EQ(delayCalculator.getNeedMediaDataDelay(firebolt::rialto::MediaSourceType::VIDEO),
              kDefaultNeedMediaDataResendTimeMs);
}
