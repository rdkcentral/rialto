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

namespace firebolt::rialto::server
{
std::weak_ptr<IMainThread> MainThreadFactory::m_mainThread;
std::mutex MainThreadFactory::m_creationMutex;

std::shared_ptr<IMainThreadFactory> IMainThreadFactory::createFactory()
{
    std::shared_ptr<IMainThreadFactory> factory;
    try
    {
        factory = std::make_shared<MainThreadFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the main thread factory, reason: %s", e.what());
    }

    return factory;
}

std::shared_ptr<IMainThread> MainThreadFactory::getMainThread() const
{
    std::lock_guard<std::mutex> lock{m_creationMutex};

    std::shared_ptr<IMainThread> mainThread = m_mainThread.lock();
    if (!mainThread)
    {
        try
        {
            mainThread = std::make_shared<MainThread>();
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the main thread, reason: %s", e.what());
        }

        m_mainThread = mainThread;
    }

    return mainThread;
}

MainThread::MainThread(std::chrono::milliseconds taskTimeout)
    : m_taskTimeout(taskTimeout), m_isMainThreadRunning{true}, m_shutdown{false},
      m_mainThreadClientId{0}, m_nextClientId{1}
{
    RIALTO_SERVER_LOG_DEBUG("MainThread is constructed with timeout %lld ms", taskTimeout.count());
    m_thread = std::thread(std::bind(&MainThread::mainThreadLoop, this));

    // Register itself
    {
        std::lock_guard<std::mutex> lock(m_registeredClientsMutex);
        m_registeredClients.insert(m_mainThreadClientId);
    }
}

MainThread::~MainThread()
{
    RIALTO_SERVER_LOG_DEBUG("MainThread is destructed");

    // Check if we're being destroyed on the main thread itself
    if (std::this_thread::get_id() == m_threadId)
    {
        RIALTO_SERVER_LOG_DEBUG("MainThread destructor called on main thread, detaching");
        m_isMainThreadRunning = false;
        m_thread.detach();
    }
    else
    {
        // Set shutdown flag and drain pending tasks
        {
            std::unique_lock<std::mutex> lock(m_taskQueueMutex);
            m_shutdown = true;

            // Extract and fail all pending tasks
            std::vector<std::shared_ptr<TaskInfo>> pendingTasks;
            while (!m_taskQueue.empty())
            {
                pendingTasks.push_back(std::move(m_taskQueue.front()));
                m_taskQueue.pop_front();
            }

            // Unlock before notifying to avoid holding lock during notifications
            lock.unlock();

            // Fail all pending tasks
            for (auto &taskInfo : pendingTasks)
            {
                std::unique_lock<std::mutex> taskLock(taskInfo->mutex);
                if (taskInfo->status == TaskStatus::Pending)
                {
                    taskInfo->status = TaskStatus::Cancelled;
                }
                taskLock.unlock();
                taskInfo->cv.notify_all();
            }

            // Relock to notify the loop
            lock.lock();
        }

        m_taskQueueCv.notify_one();
        m_thread.join();
    }
}

void MainThread::mainThreadLoop()
{
    // Store this thread's id
    m_threadId = std::this_thread::get_id();

    while (m_isMainThreadRunning)
    {
        const std::shared_ptr<TaskInfo> kTaskInfo = waitForTask();
        if (!kTaskInfo)
        {
            // Shutdown signal received
            break;
        }

        // Check if client is registered
        bool clientRegistered = false;
        {
            std::lock_guard<std::mutex> lock(m_registeredClientsMutex);
            clientRegistered = (m_registeredClients.find(kTaskInfo->clientId) != m_registeredClients.end());
        }

        if (!clientRegistered)
        {
            RIALTO_SERVER_LOG_WARN("Task ignored, client '%u' not registered", kTaskInfo->clientId);
            std::unique_lock<std::mutex> taskLock(kTaskInfo->mutex);
            kTaskInfo->status = TaskStatus::Failed;
            taskLock.unlock();
            kTaskInfo->cv.notify_all();
            continue;
        }

        // Run the task with exception handling
        try
        {
            {
                std::unique_lock<std::mutex> taskLock(kTaskInfo->mutex);
                kTaskInfo->status = TaskStatus::Running;
            }

            // Execute the task (outside the lock)
            kTaskInfo->task();

            // Mark as done
            {
                std::unique_lock<std::mutex> taskLock(kTaskInfo->mutex);
                if (kTaskInfo->status == TaskStatus::Running)
                {
                    kTaskInfo->status = TaskStatus::Done;
                }
            }
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Task threw exception: %s", e.what());
            std::unique_lock<std::mutex> taskLock(kTaskInfo->mutex);
            kTaskInfo->status = TaskStatus::Failed;
            kTaskInfo->exception = std::current_exception();
        }
        catch (...)
        {
            RIALTO_SERVER_LOG_ERROR("Task threw unknown exception");
            std::unique_lock<std::mutex> taskLock(kTaskInfo->mutex);
            kTaskInfo->status = TaskStatus::Failed;
            kTaskInfo->exception = std::current_exception();
        }

        kTaskInfo->cv.notify_all();
    }
}

const std::shared_ptr<MainThread::TaskInfo> MainThread::waitForTask()
{
    std::unique_lock<std::mutex> lock(m_taskQueueMutex);
    m_taskQueueCv.wait(lock, [this] { return !m_taskQueue.empty() || m_shutdown; });

    if (m_taskQueue.empty())
    {
        return nullptr; // Shutdown signal
    }

    auto taskInfo = std::move(m_taskQueue.front());
    m_taskQueue.pop_front();
    return taskInfo;
}

int32_t MainThread::registerClient()
{
    uint32_t clientId = m_nextClientId++;

    auto task = [this, clientId]()
    {
        RIALTO_SERVER_LOG_INFO("Registering client '%u'", clientId);
        std::lock_guard<std::mutex> lock(m_registeredClientsMutex);
        m_registeredClients.insert(clientId);
    };
    enqueueTask(m_mainThreadClientId, task);

    return clientId;
}

void MainThread::unregisterClient(uint32_t clientId)
{
    RIALTO_SERVER_LOG_INFO("Unregistering client '%u'", clientId);
    std::lock_guard<std::mutex> lock(m_registeredClientsMutex);
    m_registeredClients.erase(clientId);
}

void MainThread::enqueueTask(uint32_t clientId, const Task &task)
{
    std::shared_ptr<TaskInfo> newTask = std::make_shared<TaskInfo>();
    newTask->clientId = clientId;
    newTask->task = task;

    try
    {
        {
            std::unique_lock<std::mutex> lock(m_taskQueueMutex);
            if (m_shutdown)
            {
                RIALTO_SERVER_LOG_WARN("Cannot enqueue task, main thread is shutting down");
                return;
            }
            m_taskQueue.push_back(std::move(newTask));
        }
        m_taskQueueCv.notify_one();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to enqueue task: %s", e.what());
    }
}

bool MainThread::enqueueTaskAndWait(uint32_t clientId, const Task &task)
{
    return enqueueAndWait(clientId, task, false /* priority */);
}

bool MainThread::enqueuePriorityTaskAndWait(uint32_t clientId, const Task &task)
{
    return enqueueAndWait(clientId, task, true /* priority */);
}

bool MainThread::enqueueAndWait(uint32_t clientId, const Task &task, bool priority)
{
    // Check if we're on the main thread
    if (std::this_thread::get_id() == m_threadId)
    {
        RIALTO_SERVER_LOG_DEBUG("Self-call detected, executing task inline");

        // Check if client is registered
        {
            std::lock_guard<std::mutex> lock(m_registeredClientsMutex);
            if (m_registeredClients.find(clientId) == m_registeredClients.end())
            {
                RIALTO_SERVER_LOG_WARN("Cannot execute task, client '%u' not registered", clientId);
                return false;
            }
        }

        // Execute inline
        try
        {
            task();
            return true;
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Inline task threw exception: %s", e.what());
            return false;
        }
        catch (...)
        {
            RIALTO_SERVER_LOG_ERROR("Inline task threw unknown exception");
            return false;
        }
    }

    std::shared_ptr<TaskInfo> newTask = std::make_shared<TaskInfo>();
    newTask->clientId = clientId;
    newTask->task = task;

    {
        std::unique_lock<std::mutex> lock(m_taskQueueMutex);

        if (m_shutdown)
        {
            RIALTO_SERVER_LOG_DEBUG("Cannot enqueue task, main thread is shutting down");
            return false;
        }

        if (priority)
        {
            m_taskQueue.push_front(newTask);
        }
        else
        {
            m_taskQueue.push_back(newTask);
        }
    }

    m_taskQueueCv.notify_one();

    // Wait for task completion with timeout
    std::unique_lock<std::mutex> taskLock(newTask->mutex);

    bool completed =
        newTask->cv.wait_for(taskLock, m_taskTimeout, [&newTask]() { return newTask->status != TaskStatus::Pending; });

    if (!completed)
    {
        // Timeout: task is still pending, cancel it
        if (newTask->status == TaskStatus::Pending)
        {
            RIALTO_SERVER_LOG_ERROR("Task timeout after %lld ms, cancelling", m_taskTimeout.count());
            newTask->status = TaskStatus::Cancelled;
            return false;
        }
    }

    // Task has reached a terminal state
    if (newTask->status == TaskStatus::Done)
    {
        return true;
    }

    if (newTask->status == TaskStatus::Failed)
    {
        RIALTO_SERVER_LOG_ERROR("Task failed");
        if (newTask->exception)
        {
            try
            {
                std::rethrow_exception(newTask->exception);
            }
            catch (const std::exception &e)
            {
                RIALTO_SERVER_LOG_ERROR("Task exception: %s", e.what());
            }
        }
        return false;
    }

    // Cancelled or other terminal state
    RIALTO_SERVER_LOG_DEBUG("Task did not complete successfully, status: %d", static_cast<int>(newTask->status));
    return false;
}
} // namespace firebolt::rialto::server

