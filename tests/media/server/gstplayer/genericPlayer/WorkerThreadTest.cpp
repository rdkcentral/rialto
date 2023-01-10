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
#include "GenericPlayerTaskMock.h"
#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>

using firebolt::rialto::server::GenericPlayerTaskMock;
using testing::Invoke;
using testing::StrictMock;

TEST(WorkerThreadTest, shouldEnqueueTaskAndExit)
{
    std::mutex m_taskMutex;
    std::condition_variable m_taskCv;
    bool m_taskDone{false};
    auto sut = firebolt::rialto::server::WorkerThreadFactory().createWorkerThread();

    std::unique_ptr<firebolt::rialto::server::IPlayerTask> someTask{std::make_unique<StrictMock<GenericPlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<GenericPlayerTaskMock> &>(*someTask), execute())
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

    std::unique_ptr<firebolt::rialto::server::IPlayerTask> shutdownTask{
        std::make_unique<StrictMock<GenericPlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<GenericPlayerTaskMock> &>(*shutdownTask), execute())
        .WillOnce(Invoke([&]() { sut->stop(); }));
    sut->enqueueTask(std::move(shutdownTask));

    // sut.reset();
}
