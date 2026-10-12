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
#include <mutex>
#include <thread>
#include <unordered_map>

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
        std::lock_guard<std::mutex> lock(m_mutex);
        m_activeTimers.erase(timer);
    }

    std::function<void()> getTimerCallback(const firebolt::rialto::common::Timer *timer)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_activeTimers.find(timer);
        return it != m_activeTimers.end() ? it->second : nullptr;
    }

private:
    CommonTimerLoop()
    {
        m_loop = g_main_loop_new(nullptr, FALSE);
        m_thread = std::thread([this]() { g_main_loop_run(m_loop); });
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
    std::mutex m_mutex;
    std::unordered_map<const firebolt::rialto::common::Timer *, std::function<void()>> m_activeTimers;
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
                auto callback = CommonTimerLoop::instance().getTimerCallback(static_cast<Timer *>(data));
                if (callback)
                {
                    callback();
                    return CommonTimerLoop::instance().getTimerCallback(static_cast<Timer *>(data)) ? TRUE : FALSE;
                }
                return FALSE;
            },
            this);
    }
    else
    {
        m_timerId = g_timeout_add_once(
            static_cast<guint>(timeout.count()),
            [](gpointer data)
            {
                auto callback = CommonTimerLoop::instance().getTimerCallback(static_cast<Timer *>(data));
                if (callback)
                {
                    callback();
                    CommonTimerLoop::instance().removeTimerCallback(static_cast<Timer *>(data));
                }
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
    return CommonTimerLoop::instance().getTimerCallback(this) != nullptr;
}
} // namespace firebolt::rialto::common
