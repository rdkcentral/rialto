/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "WorkerThread.h"
#include "PlayerTaskMock.h"
#include <condition_variable>
#include <future>
#include <gtest/gtest.h>
#include <mutex>
#include <vector>

using firebolt::rialto::server::PlayerTaskMock;
using testing::Invoke;
using testing::StrictMock;

TEST(WorkerThreadTest, shouldEnqueueTaskAndExit)
{
    std::mutex m_taskMutex;
    std::condition_variable m_taskCv;
    bool m_taskDone{false};
    auto sut = firebolt::rialto::server::WorkerThreadFactory().createWorkerThread();

    std::unique_ptr<firebolt::rialto::server::IPlayerTask> someTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*someTask), execute())
        .WillOnce(Invoke(
            [&]()
            {
                std::unique_lock<std::mutex> lock{m_taskMutex};
                m_taskDone = true;
                m_taskCv.notify_one();
            }));
    sut->enqueueTask(std::move(someTask));

    std::unique_lock<std::mutex> lock{m_taskMutex};
    m_taskCv.wait_for(lock, std::chrono::milliseconds(200), [&]() { return m_taskDone; });

    std::unique_ptr<firebolt::rialto::server::IPlayerTask> shutdownTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*shutdownTask), execute())
        .WillOnce(Invoke([&]() { sut->stop(); }));
    sut->enqueueTask(std::move(shutdownTask));

    // sut.reset();
}

TEST(WorkerThreadTest, shouldExecutePriorityTasksFirstInTheirOwnOrder)
{
    std::mutex orderMutex;
    std::vector<int> executionOrder;
    std::promise<void> blockerRelease;
    std::promise<void> blockerStarted;
    std::promise<void> allDone;
    auto sut = firebolt::rialto::server::WorkerThreadFactory().createWorkerThread();

    auto makeTask = [&](int id, bool isLast)
    {
        auto task = std::make_unique<StrictMock<PlayerTaskMock>>();
        EXPECT_CALL(*task, execute())
            .WillOnce(Invoke(
                [&, id, isLast]()
                {
                    std::unique_lock<std::mutex> lock{orderMutex};
                    executionOrder.push_back(id);
                    if (isLast)
                    {
                        allDone.set_value();
                    }
                }));
        return std::unique_ptr<firebolt::rialto::server::IPlayerTask>{std::move(task)};
    };

    auto blocker = std::make_unique<StrictMock<PlayerTaskMock>>();
    EXPECT_CALL(*blocker, execute())
        .WillOnce(Invoke(
            [&]()
            {
                blockerStarted.set_value();
                blockerRelease.get_future().wait();
            }));
    sut->enqueueTask(std::move(blocker));
    blockerStarted.get_future().wait();

    sut->enqueueTask(makeTask(1, false));
    sut->enqueuePriorityTask(makeTask(2, false));
    sut->enqueueTask(makeTask(3, true));
    sut->enqueuePriorityTask(makeTask(4, false));
    blockerRelease.set_value();

    EXPECT_EQ(std::future_status::ready, allDone.get_future().wait_for(std::chrono::seconds(1)));
    {
        std::unique_lock<std::mutex> lock{orderMutex};
        EXPECT_EQ(executionOrder, (std::vector<int>{2, 4, 1, 3}));
    }

    std::unique_ptr<firebolt::rialto::server::IPlayerTask> shutdownTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*shutdownTask), execute())
        .WillOnce(Invoke([&]() { sut->stop(); }));
    sut->enqueueTask(std::move(shutdownTask));
}
