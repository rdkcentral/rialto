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

#include "MainThread.h"
#include "RialtoServerLogging.h"
#include <string>
#include <utility>
#include <vector>

namespace firebolt::rialto::server::service
{
MainThread::MainThread() : m_isMainThreadRunning{true}
{
    RIALTO_SERVER_LOG_DEBUG("MainThread is constructed");
    m_thread = std::thread(std::bind(&MainThread::mainThreadLoop, this));
}

MainThread::~MainThread()
{
    RIALTO_SERVER_LOG_DEBUG("MainThread is destructed");
    auto shutdownTask = [this]() { m_isMainThreadRunning = false; };
    enqueueTask(shutdownTask);
    m_thread.join();
}

void MainThread::mainThreadLoop()
{
    while (m_isMainThreadRunning)
    {
        Task task = waitForTask();
        task();
    }
}

MainThread::Task MainThread::waitForTask()
{
    std::unique_lock<std::mutex> lock(m_taskMutex);
    if (m_taskQueue.empty())
    {
        m_taskCv.wait(lock, [this] { return !m_taskQueue.empty(); });
    }
    Task task = m_taskQueue.front();
    m_taskQueue.pop();
    return task;
}

void MainThread::enqueueTask(MainThread::Task task)
{
    {
        std::unique_lock<std::mutex> lock(m_taskMutex);
        m_taskQueue.push(task);
    }
    m_taskCv.notify_one();
}
} // namespace firebolt::rialto::server::service
