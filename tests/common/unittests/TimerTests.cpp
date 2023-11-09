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

#include "ITimer.h"

#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>

using firebolt::rialto::common::ITimer;
using firebolt::rialto::common::ITimerFactory;
using firebolt::rialto::common::TimerType;

TEST(TimerTests, ShouldTimeoutOneShotTimer)
{
    std::mutex mtx;
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
    cv.wait_for(lock, std::chrono::milliseconds{110}, [&]() { return callFlag; });
    EXPECT_TRUE(callFlag);
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
    std::condition_variable cv;
    std::unique_lock<std::mutex> lock{mtx};
    unsigned callCounter{0};
    std::unique_ptr<ITimer> timer{ITimerFactory::getFactory()->createTimer(
        std::chrono::milliseconds{30},
        [&]()
        {
            std::unique_lock<std::mutex> lock{mtx};
            ++callCounter;
            cv.notify_one();
        },
        TimerType::PERIODIC)};
    EXPECT_TRUE(timer->isActive());
    cv.wait_for(lock, std::chrono::milliseconds{110}, [&]() { return callCounter >= 3; });
    EXPECT_TRUE(callCounter >= 3);
}
