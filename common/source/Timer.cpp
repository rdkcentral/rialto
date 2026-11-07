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
#include <glib.h>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace
{
using firebolt::rialto::common::TimerType;

class CommonTimerLoop
{
public:
    static CommonTimerLoop &instance()
    {
        static CommonTimerLoop instance;
        return instance;
    }

    std::uint64_t addTimer(const std::chrono::milliseconds &timeout, const std::function<void()> &callback,
                           TimerType timerType)
    {
        GSource *source = g_timeout_source_new(static_cast<guint>(timeout.count()));
        std::uint64_t timerId{0};
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            timerId = ++m_lastTimerId;
            m_timers.emplace(timerId, TimerEntry{callback, timerType == TimerType::PERIODIC, source});
        }
        g_source_set_callback(source, &CommonTimerLoop::onTimeout, new std::uint64_t{timerId},
                              [](gpointer data) { delete static_cast<std::uint64_t *>(data); });
        g_source_attach(source, m_context);
        return timerId;
    }

    void cancelTimer(std::uint64_t timerId)
    {
        GSource *source{nullptr};
        {
            std::unique_lock<std::mutex> lock{m_mutex};
            auto timerIt = m_timers.find(timerId);
            if (timerIt != m_timers.end())
            {
                source = timerIt->second.source;
                m_timers.erase(timerIt);
            }
            if (std::this_thread::get_id() != m_thread.get_id())
            {
                m_callbackFinished.wait(lock, [this, timerId]() { return m_runningTimerId != timerId; });
            }
        }
        if (source)
        {
            // Destroying the source by pointer is safe even if GLib has already removed it.
            g_source_destroy(source);
            g_source_unref(source);
        }
    }

    bool isTimerActive(std::uint64_t timerId)
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        return m_timers.find(timerId) != m_timers.end();
    }

private:
    struct TimerEntry
    {
        std::function<void()> callback;
        bool isPeriodic;
        GSource *source;
    };

    CommonTimerLoop() : m_context{g_main_context_new()}, m_loop{g_main_loop_new(m_context, FALSE)}
    {
        m_thread = std::thread(
            [this]()
            {
                g_main_context_push_thread_default(m_context);
                g_main_loop_run(m_loop);
                g_main_context_pop_thread_default(m_context);
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
        g_main_context_unref(m_context);
    }

    static gboolean onTimeout(gpointer data)
    {
        return instance().handleTimeout(*static_cast<std::uint64_t *>(data)) ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
    }

    bool handleTimeout(std::uint64_t timerId)
    {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            auto timerIt = m_timers.find(timerId);
            if (timerIt == m_timers.end())
            {
                return false;
            }
            callback = timerIt->second.callback;
            m_runningTimerId = timerId;
        }

        if (callback)
        {
            callback();
        }

        bool keepSource{false};
        GSource *finishedSource{nullptr};
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_runningTimerId = kNoTimerId;
            auto timerIt = m_timers.find(timerId);
            if (timerIt != m_timers.end())
            {
                if (timerIt->second.isPeriodic)
                {
                    keepSource = true;
                }
                else
                {
                    finishedSource = timerIt->second.source;
                    m_timers.erase(timerIt);
                }
            }
        }
        m_callbackFinished.notify_all();

        if (finishedSource)
        {
            g_source_unref(finishedSource);
        }
        return keepSource;
    }

    static constexpr std::uint64_t kNoTimerId{0};

    GMainContext *m_context;
    GMainLoop *m_loop;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_callbackFinished;
    std::uint64_t m_lastTimerId{kNoTimerId};
    std::uint64_t m_runningTimerId{kNoTimerId};
    std::unordered_map<std::uint64_t, TimerEntry> m_timers;
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
    : m_timerId{CommonTimerLoop::instance().addTimer(timeout, callback, timerType)}
{
}

Timer::~Timer()
{
    cancel();
}

void Timer::cancel()
{
    CommonTimerLoop::instance().cancelTimer(m_timerId);
}

bool Timer::isActive() const
{
    return CommonTimerLoop::instance().isTimerActive(m_timerId);
}
} // namespace firebolt::rialto::common
