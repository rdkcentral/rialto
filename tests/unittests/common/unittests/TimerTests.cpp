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

#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>

#include "ITimer.h"

using firebolt::rialto::common::ITimer;
using firebolt::rialto::common::ITimerFactory;
using firebolt::rialto::common::TimerType;

namespace
{
// kEnoughTimeForTestToComplete must allow the test to complete under valgrind which runs slower
constexpr std::chrono::milliseconds kEnoughTimeForTestToComplete{200};
} // namespace

TEST(TimerTests, ShouldTimeoutOneShotTimer)
{
    std::mutex mtx;
    { // This scope is required to suppress a false warning from cppcheck (about the mutex above)
        std::condition_variable cv;
        std::unique_lock<std::mutex> lock{mtx};
        bool callFlag{false};
        std::unique_ptr<ITimer> timer{ITimerFactory::getFactory()->createTimer(std::chrono::milliseconds{100},
                                                                               [&]()
                                                                               {
                                                                                   std::unique_lock<std::mutex> lock{mtx};
                                                                                   callFlag = true;
                                                                                   cv.notify_one();
                                                                               })};
        EXPECT_TRUE(timer->isActive());
        cv.wait_for(lock, kEnoughTimeForTestToComplete);
        EXPECT_TRUE(callFlag);
    }
}

TEST(TimerTests, ShouldCancelTimer)
{
    std::atomic_bool callFlag{false};
    std::unique_ptr<ITimer> timer{
        ITimerFactory::getFactory()->createTimer(std::chrono::milliseconds{100}, [&]() { callFlag = true; })};
    EXPECT_TRUE(timer->isActive());
    timer->cancel();
    EXPECT_FALSE(timer->isActive());
    EXPECT_FALSE(callFlag);
}

TEST(TimerTests, ShouldTimeoutPeriodicTimer)
{
    std::mutex mtx;
    { // This scope is required to suppress a false warning from cppcheck (about the mutex above)
        std::condition_variable cv;
        std::unique_lock<std::mutex> lock{mtx};
        unsigned callCounter{0};
        std::unique_ptr<ITimer> timer{ITimerFactory::getFactory()->createTimer(
            std::chrono::milliseconds{30},
            [&]()
            {
                std::unique_lock<std::mutex> lock{mtx};
                ++callCounter;
                if (callCounter >= 3)
                {
                    cv.notify_one();
                }
            },
            TimerType::PERIODIC)};
        EXPECT_TRUE(timer->isActive());
        cv.wait_for(lock, kEnoughTimeForTestToComplete);
        EXPECT_GE(callCounter, 3);
    }
}
