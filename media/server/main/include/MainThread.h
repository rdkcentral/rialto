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

#ifndef FIREBOLT_RIALTO_SERVER_MAIN_THREAD_H_
#define FIREBOLT_RIALTO_SERVER_MAIN_THREAD_H_

#include "IMainThread.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>

namespace firebolt::rialto::server
{
/**
 * @brief IMainThread factory class definition.
 */
class MainThreadFactory : public IMainThreadFactory
{
public:
    MainThreadFactory() = default;
    ~MainThreadFactory() override = default;

    std::shared_ptr<IMainThread> getMainThread() const override;

protected:
    /**
     * @brief Weak pointer to the singleton main thread object.
     */
    static std::weak_ptr<IMainThread> m_mainThread;

    /**
     * @brief Mutex protection for creation of the MainThread object.
     */
    static std::mutex m_creationMutex;
};

/**
 * @brief The definition of the MainThread.
 */
class MainThread : public IMainThread
{
public:
    /**
     * @brief Constructor with optional timeout parameter for testing.
     *
     * @param[in]  taskTimeout : Timeout for blocking task enqueues (default 30s).
     */
    explicit MainThread(std::chrono::milliseconds taskTimeout = std::chrono::milliseconds{30000});
    virtual ~MainThread();

    int32_t registerClient() override;
    void unregisterClient(uint32_t clientId) override;

    void enqueueTask(uint32_t clientId, const Task &task) override;
    bool enqueueTaskAndWait(uint32_t clientId, const Task &task) override;
    bool enqueuePriorityTaskAndWait(uint32_t clientId, const Task &task) override;

private:
    /**
     * @brief Status of a task in the queue.
     */
    enum class TaskStatus
    {
        Pending,   /**< Task is queued, not yet running */
        Running,   /**< Task is currently executing */
        Done,      /**< Task completed successfully */
        Failed,    /**< Task threw an exception */
        Cancelled  /**< Task was cancelled (timed out, not yet started) */
    };

    /**
     * @brief Information of a task with completion state.
     */
    struct TaskInfo
    {
        uint32_t clientId;              /**< The id of the client creating the task */
        Task task;                      /**< The task to execute */
        std::mutex mutex;               /**< Protects state, status, and exception */
        std::condition_variable cv;     /**< Signals state changes */
        TaskStatus status{TaskStatus::Pending};  /**< Current task status */
        std::exception_ptr exception;   /**< Exception thrown by task, if any */

        TaskInfo() : clientId(0) {}
    };

    /**
     * @brief Helper to enqueue a task and wait with optional priority.
     *
     * @param[in]  clientId : The client id.
     * @param[in]  task     : The task to run.
     * @param[in]  priority : Whether to push to front (priority) or back.
     * @retval true if task completed, false on timeout/shutdown/error.
     */
    bool enqueueAndWait(uint32_t clientId, const Task &task, bool priority);

    /**
     * @brief Starts a loop that listens for enqueued tasks.
     */
    void mainThreadLoop();

    /**
     * @brief Waits for tasks to enter the queue and returns the next task.
     *
     * @retval The next task in the queue, or nullptr on shutdown.
     */
    const std::shared_ptr<TaskInfo> waitForTask();

    /**
     * @brief Default timeout for blocking task enqueues (30 seconds).
     */
    static constexpr std::chrono::milliseconds kDefaultTaskTimeout{30000};

    /**
     * @brief The main thread's timeout for blocking enqueues.
     */
    std::chrono::milliseconds m_taskTimeout;

    /**
     * @brief Whether the main thread is running.
     */
    bool m_isMainThreadRunning;

    /**
     * @brief The main thread.
     */
    std::thread m_thread;

    /**
     * @brief Thread id of the main thread.
     */
    std::atomic<std::thread::id> m_threadId;

    /**
     * @brief A mutex protecting access to the task queue.
     */
    std::mutex m_taskQueueMutex;

    /**
     * @brief A condition variable used to notify of tasks entering the task queue.
     */
    std::condition_variable m_taskQueueCv;

    /**
     * @brief The queue of tasks and their information.
     */
    std::deque<std::shared_ptr<TaskInfo>> m_taskQueue;

    /**
     * @brief Flag indicating the main thread is shutting down.
     */
    bool m_shutdown;

    /**
     * @brief The main thread object's client id.
     */
    const uint32_t m_mainThreadClientId;

    /**
     * @brief The next client id to be given to a registering client.
     */
    std::atomic<uint32_t> m_nextClientId;

    /**
     * @brief Mutex protecting access to registered clients set.
     */
    std::mutex m_registeredClientsMutex;

    /**
     * @brief Clients registered on this thread.
     */
    std::set<uint32_t> m_registeredClients;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MAIN_THREAD_H_
