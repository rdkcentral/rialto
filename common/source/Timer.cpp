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
#include <limits>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace
{
// guint is 32-bit, but std::chrono::milliseconds::rep is typically 64-bit; saturate instead of
// silently wrapping (which would otherwise fire much earlier than the caller requested).
[[maybe_unused]] guint toGuintMilliseconds(const std::chrono::milliseconds &timeout)
{
    constexpr auto kMaxMs = static_cast<std::chrono::milliseconds::rep>(std::numeric_limits<guint>::max());
    const auto count = timeout.count();
    if (count < 0)
    {
        return 0;
    }
    if (count > kMaxMs)
    {
        RIALTO_COMMON_LOG_WARN("Timer duration %lld ms exceeds guint range, clamping to %u ms",
                               static_cast<long long>(count), std::numeric_limits<guint>::max());
        return std::numeric_limits<guint>::max();
    }
    return static_cast<guint>(count);
}

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

    // Distinct from getTimerCallback(): an empty std::function is still a registered timer.
    bool hasTimerCallback(const firebolt::rialto::common::Timer *timer)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_activeTimers.find(timer) != m_activeTimers.end();
    }

    // Timer sources must attach here, not to the global-default context, so they only ever
    // dispatch on m_thread.
    GMainContext *context() const { return m_context; }

private:
    CommonTimerLoop()
    {
        //m_loop = g_main_loop_new(nullptr, FALSE);
        //m_thread = std::thread([this]() { g_main_loop_run(m_loop); });
	m_context = g_main_context_new();
        m_loop = g_main_loop_new(m_context, FALSE);
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

    GMainContext *m_context;
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
    : m_active{true}, m_timeout{timeout}, m_callback{callback}
{
    m_thread = std::thread(
        [this, timerType]()
        {
            do
            {
<<<<<<< HEAD
                std::unique_lock<std::mutex> lock{m_mutex};
                if (!m_cv.wait_for(lock, m_timeout, [this]() { return !m_active; }))
                {
                    if (m_active && m_callback)
                    {
                        lock.unlock();
                        m_callback();
                    }
                }
            } while (timerType == TimerType::PERIODIC && m_active);
            m_active = false;
        });
=======
                //auto callback = CommonTimerLoop::instance().getTimerCallback(static_cast<firebolt::rialto::common::Timer *>(data));
		auto *timer = static_cast<firebolt::rialto::common::Timer *>(data);
                auto callback = CommonTimerLoop::instance().getTimerCallback(timer);
                if (callback)
                {
                    callback();
                    //return CommonTimerLoop::instance().getTimerCallback(static_cast<firebolt::rialto::common::Timer *>(data)) ? TRUE : FALSE;

                }
                //return FALSE;
		return CommonTimerLoop::instance().hasTimerCallback(timer) ? TRUE : FALSE;
            },
            this, nullptr);
    }
    else
    {
        //m_timerId = g_timeout_add(
            //static_cast<guint>(timeout.count()),
            //[](gpointer data) -> gboolean
	g_source_set_callback(
            source,
            [](gpointer data) -> gboolean
            {
                //auto callback = CommonTimerLoop::instance().getTimerCallback(static_cast<firebolt::rialto::common::Timer *>(data));
		auto *timer = static_cast<firebolt::rialto::common::Timer *>(data);
                auto callback = CommonTimerLoop::instance().getTimerCallback(timer);
                if (callback)
                {
                    callback();
                    //CommonTimerLoop::instance().removeTimerCallback(static_cast<firebolt::rialto::common::Timer *>(data));
                }
		CommonTimerLoop::instance().removeTimerCallback(timer);
                return FALSE;
            },
            this, nullptr);
    }
    // Attached to our private context (not the global-default one), so this only ever fires on m_thread.
    // Keep our own ref alongside the context's so cancel() can still destroy it by pointer later.
    g_source_attach(source, CommonTimerLoop::instance().context());
    m_source = source;
>>>>>>> a3a9f9ea (RDKEMW-24084: Single thread timer)
}

Timer::~Timer()
{
    cancel();
}

void Timer::cancel()
{
    m_active = false;

    if (std::this_thread::get_id() != m_thread.get_id() && m_thread.joinable())
    {
        m_cv.notify_one();
        m_thread.join();
    }
}

bool Timer::isActive() const
{
<<<<<<< HEAD
    return m_active;
=======
    //return CommonTimerLoop::instance().getTimerCallback(this) != nullptr;
    return CommonTimerLoop::instance().hasTimerCallback(this);
>>>>>>> a3a9f9ea (RDKEMW-24084: Single thread timer)
}
} // namespace firebolt::rialto::common
