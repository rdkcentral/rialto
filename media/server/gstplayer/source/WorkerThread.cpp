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
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
std::unique_ptr<IWorkerThread> WorkerThreadFactory::createWorkerThread() const
{
    std::unique_ptr<IWorkerThread> workerThread;
    try
    {
        workerThread = std::make_unique<WorkerThread>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the worker thread, reason: %s", e.what());
    }
    return workerThread;
}

WorkerThread::WorkerThread()
{
    RIALTO_SERVER_LOG_INFO("Worker thread is starting");
    m_taskThread = std::thread(&WorkerThread::taskHandler, this);
}

WorkerThread::~WorkerThread()
{
    if (m_taskThread.joinable())
    {
        m_taskThread.join();
    }
}

void WorkerThread::stop()
{
    RIALTO_SERVER_LOG_INFO("Stopping worker thread");
    if (m_isTaskThreadActive)
    {
        m_isTaskThreadActive = false;
    }
}

void WorkerThread::join()
{
    if (m_taskThread.joinable())
    {
        m_taskThread.join();
    }
}

void WorkerThread::enqueueTask(std::unique_ptr<IPlayerTask> &&task)
{
    {
        std::unique_lock<std::mutex> lock(m_taskMutex);
        m_taskQueue.push(std::move(task));
    }
    m_taskCV.notify_one();
}

void WorkerThread::taskHandler()
{
    while (m_isTaskThreadActive)
    {
        std::unique_ptr<IPlayerTask> task = waitForTask();
        task->execute();
    }
}

std::unique_ptr<IPlayerTask> WorkerThread::waitForTask()
{
    std::unique_lock<std::mutex> lock(m_taskMutex);
    if (m_taskQueue.empty())
    {
        m_taskCV.wait(lock, [this] { return !m_taskQueue.empty(); });
    }
    std::unique_ptr<IPlayerTask> task = std::move(m_taskQueue.front());
    m_taskQueue.pop();
    return task;
}
} // namespace firebolt::rialto::server
