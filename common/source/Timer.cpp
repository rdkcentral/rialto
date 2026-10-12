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
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace
{
class CommonTimerLoop
{
public:
    static CommonTimerLoop &instance()
    {
        static CommonTimerLoop instance;
        return instance;
    }

    void storeTimerCallback(const firebolt::rialto::common::Timer *timer, const std::function<void()> &callback)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_activeTimers[timer] = callback;
    }

    void removeTimerCallback(const firebolt::rialto::common::Timer *timer)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_activeTimers.erase(timer);
        if (std::this_thread::get_id() != m_loopThreadId)
        {
            m_dispatchFinished.wait(lock, [this, timer]()
                                     { return m_dispatchingTimers.find(timer) == m_dispatchingTimers.end(); });
        }
    }

    std::function<void()> getTimerCallback(const firebolt::rialto::common::Timer *timer)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_activeTimers.find(timer);
        if (it == m_activeTimers.end())
        {
            return nullptr;
        }
        m_dispatchingTimers.insert(timer);
        return it->second;
    }

    bool isActive(const firebolt::rialto::common::Timer *timer)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_activeTimers.find(timer) != m_activeTimers.end();
    }

    void dispatchFinished(const firebolt::rialto::common::Timer *timer, bool eraseRegistration)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (eraseRegistration)
        {
            m_activeTimers.erase(timer);
        }
        m_dispatchingTimers.erase(timer);
        m_dispatchFinished.notify_all();
    }

private:
    CommonTimerLoop()
    {
        m_loop = g_main_loop_new(nullptr, FALSE);
        m_thread = std::thread(
            [this]()
            {
                m_loopThreadId = std::this_thread::get_id();
                g_main_loop_run(m_loop);
            });
    }

    ~CommonTimerLoop()
    {
        g_main_loop_quit(m_loop);
        if (m_thread.joinable())
        {
            m_thread.join();
        }
        g_main_loop_unref(m_loop);
    }

    GMainLoop *m_loop;
    std::thread m_thread;
    std::thread::id m_loopThreadId;
    std::mutex m_mutex;
    std::condition_variable m_dispatchFinished;
    std::unordered_map<const firebolt::rialto::common::Timer *, std::function<void()>> m_activeTimers;
    std::unordered_set<const firebolt::rialto::common::Timer *> m_dispatchingTimers;
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
    CommonTimerLoop::instance().storeTimerCallback(this, callback);
    if (timerType == TimerType::PERIODIC)
    {
        m_timerId = g_timeout_add(
            static_cast<guint>(timeout.count()),
            [](gpointer data) -> gboolean
            {
                Timer *timer = static_cast<Timer *>(data);
                auto timerCallback = CommonTimerLoop::instance().getTimerCallback(timer);
                if (timerCallback)
                {
                    timerCallback();
                    bool stillActive = CommonTimerLoop::instance().isActive(timer);
                    if (!stillActive)
                    {
                        timer->m_timerId = 0;
                    }
                    CommonTimerLoop::instance().dispatchFinished(timer, false);
                    return stillActive ? TRUE : FALSE;
                }
                return FALSE;
            },
            this);
    }
    else
    {
        m_timerId = g_timeout_add(
            static_cast<guint>(timeout.count()),
            [](gpointer data) -> gboolean
            {
                Timer *timer = static_cast<Timer *>(data);
                auto timerCallback = CommonTimerLoop::instance().getTimerCallback(timer);
                if (timerCallback)
                {
                    timer->m_timerId = 0;
                    timerCallback();
                    CommonTimerLoop::instance().dispatchFinished(timer, true);
                }
                return G_SOURCE_REMOVE;
            },
            this);
    }
}

Timer::~Timer()
{
    cancel();
}

void Timer::cancel()
{
    CommonTimerLoop::instance().removeTimerCallback(this);
    if (m_timerId != 0)
    {
        g_source_remove(m_timerId);
        m_timerId = 0;
    }
}

bool Timer::isActive() const
{
    return CommonTimerLoop::instance().isActive(this);
}
} // namespace firebolt::rialto::common
