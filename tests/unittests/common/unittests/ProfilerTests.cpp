/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#include <chrono>
#include <gtest/gtest.h>
#include <thread>

#include "IProfiler.h"

using firebolt::rialto::common::IProfiler;
using firebolt::rialto::common::IProfilerFactory;

TEST(ProfilerTests, ShouldCreateProfiler)
{
    std::unique_ptr<IProfiler> profiler = IProfilerFactory::getFactory()->createProfiler("test");
    EXPECT_NE(profiler, nullptr);
}

TEST(ProfilerTests, ShouldMeasureElapsedTime)
{
    std::unique_ptr<IProfiler> profiler = IProfilerFactory::getFactory()->createProfiler("test");
    ASSERT_NE(profiler, nullptr);

    profiler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    double elapsed = profiler->stop();

    // Allow some tolerance for timing variations
    EXPECT_GE(elapsed, 10.0);
    EXPECT_LT(elapsed, 50.0);
}

TEST(ProfilerTests, ShouldReturnZeroWhenStoppedWithoutStart)
{
    std::unique_ptr<IProfiler> profiler = IProfilerFactory::getFactory()->createProfiler("test");
    ASSERT_NE(profiler, nullptr);

    double elapsed = profiler->stop();
    EXPECT_EQ(elapsed, 0.0);
}

TEST(ProfilerTests, ShouldResetProfiler)
{
    std::unique_ptr<IProfiler> profiler = IProfilerFactory::getFactory()->createProfiler("test");
    ASSERT_NE(profiler, nullptr);

    profiler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    profiler->reset();

    double elapsed = profiler->stop();
    EXPECT_EQ(elapsed, 0.0);
}

TEST(ProfilerTests, ShouldRestartProfiling)
{
    std::unique_ptr<IProfiler> profiler = IProfilerFactory::getFactory()->createProfiler("test");
    ASSERT_NE(profiler, nullptr);

    profiler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    double firstElapsed = profiler->stop();

    profiler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    double secondElapsed = profiler->stop();

    // Both should have measured some time
    EXPECT_GT(firstElapsed, 0.0);
    EXPECT_GT(secondElapsed, 0.0);
}
