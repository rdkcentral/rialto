/*
 * Copyright (C) 2023 Sky UK
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
