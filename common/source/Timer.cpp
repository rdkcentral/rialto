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

#include "Timer.h"
#include "RialtoCommonLogging.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

namespace
{
class CommonTimerLoop
{
public:
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::milliseconds;
    using Callback = std::function<void()>;

    static CommonTimerLoop &instance()
    {
        static CommonTimerLoop instance;
        return instance;
    }

private:
    struct TimerTask
    {
        std::uint64_t id;
        Clock::time_point nextExecution;
        Duration interval;
        Callback callback;
        bool repeat;
        std::atomic<bool> cancelled{false};
    };

    struct Compare
    {
        bool operator()(const std::shared_ptr<TimerTask> &a, const std::shared_ptr<TimerTask> &b) const
        {
            return a->nextExecution > b->nextExecution;
        }
    };

    std::priority_queue<std::shared_ptr<TimerTask>, std::vector<std::shared_ptr<TimerTask>>, Compare> m_tasks;

    std::unordered_map<std::uint64_t, std::shared_ptr<TimerTask>> m_activeTasks;

    std::mutex m_mutex;
    std::condition_variable m_cv;

    std::thread m_worker;
    std::atomic<bool> m_running{true};

    std::atomic<std::uint64_t> m_nextId{1};

    CommonTimerLoop()
    {
        RIALTO_COMMON_LOG_DEBUG("Starting CommonTimerLoop");
        m_worker = std::thread([this]() { run(); });
    }

    ~CommonTimerLoop()
    {
        if (!m_running.exchange(false))
            return;

        m_cv.notify_one();

        if (m_worker.joinable())
            m_worker.join();

        RIALTO_COMMON_LOG_DEBUG("CommonTimerLoop stopped");
    }

    CommonTimerLoop(const CommonTimerLoop &) = delete;
    CommonTimerLoop &operator=(const CommonTimerLoop &) = delete;
    CommonTimerLoop(CommonTimerLoop &&) = delete;
    CommonTimerLoop &operator=(CommonTimerLoop &&) = delete;

public:
    std::uint64_t add(Duration delay, Callback callback, firebolt::rialto::common::TimerType timerType)
    {
        auto task = std::make_shared<TimerTask>();

        task->id = m_nextId++;
        task->nextExecution = Clock::now() + delay;
        task->interval = delay;
        task->callback = std::move(callback);
        task->repeat = timerType == firebolt::rialto::common::TimerType::PERIODIC;

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            m_activeTasks[task->id] = task;
            m_tasks.push(task);
        }

        m_cv.notify_one();

        return task->id;
    }

    void cancel(std::uint64_t id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_activeTasks.find(id);

        if (it != m_activeTasks.end())
        {
            it->second->cancelled = true;
            m_activeTasks.erase(it);
        }

        m_cv.notify_one();
    }

    bool isActive(std::uint64_t id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_activeTasks.find(id) != m_activeTasks.end();
    }

private:
    void run()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        while (m_running)
        {
            // remove cancelled tasks from the top of the queue
            while (!m_tasks.empty() && m_tasks.top()->cancelled)
                m_tasks.pop();

            if (m_tasks.empty())
            {
                m_cv.wait(lock, [this]() { return !m_running || !m_tasks.empty(); });

                continue;
            }

            auto task = m_tasks.top();

            auto now = Clock::now();

            if (task->nextExecution > now)
            {
                m_cv.wait_until(lock, task->nextExecution);
                continue;
            }

            m_tasks.pop();

            if (task->cancelled)
                continue;

            if (!task->repeat)
                m_activeTasks.erase(task->id);

            lock.unlock();

            try
            {
                task->callback();
            }
            catch (...)
            {
                RIALTO_COMMON_LOG_ERROR("Exception occurred while executing timer callback");
            }

            lock.lock();

            if (m_running && task->repeat && !task->cancelled)
            {
                task->nextExecution = Clock::now() + task->interval;
                m_tasks.push(task);
            }
            else
            {
                m_activeTasks.erase(task->id);
            }
        }

        while (!m_tasks.empty())
            m_tasks.pop();

        m_activeTasks.clear();
    }
};
} // namespace

namespace firebolt::rialto::common
{
std::weak_ptr<ITimerFactory> TimerFactory::m_factory;

std::shared_ptr<ITimerFactory> ITimerFactory::getFactory()
{
    std::shared_ptr<ITimerFactory> factory = TimerFactory::m_factory.lock();

    if (!factory)
    {
        try
        {
            factory = std::make_shared<TimerFactory>();
        }
        catch (const std::exception &e)
        {
            RIALTO_COMMON_LOG_ERROR("Failed to create the timer factory, reason: %s", e.what());
        }

        TimerFactory::m_factory = factory;
    }

    return factory;
}

std::unique_ptr<ITimer> TimerFactory::createTimer(const std::chrono::milliseconds &timeout,
                                                  const std::function<void()> &callback, TimerType timerType) const
{
    return std::make_unique<Timer>(timeout, callback, timerType);
}

Timer::Timer(const std::chrono::milliseconds &timeout, const std::function<void()> &callback, TimerType timerType)
{
    if (!callback)
    {
        RIALTO_COMMON_LOG_ERROR("Timer callback is not set");
        return;
    }
    m_id = CommonTimerLoop::instance().add(timeout, callback, timerType);
}

Timer::~Timer()
{
    cancel();
}

void Timer::cancel()
{
    CommonTimerLoop::instance().cancel(m_id);
}

bool Timer::isActive() const
{
    return CommonTimerLoop::instance().isActive(m_id);
}
} // namespace firebolt::rialto::common
