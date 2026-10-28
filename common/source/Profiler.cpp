/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "Profiler.h"
#include "RialtoCommonLogging.h"

#ifdef RIALTO_ENABLE_AVMONITOR_CLIENT
#include <WPEFramework/avmonitor/avmonitor.h>
#endif

#include <algorithm>
#include <cctype>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

namespace
{
inline constexpr const char *kProfilerEnv{"PROFILER_ENABLED"};
inline constexpr const char *kProfilerDumpFileEnv{"PROFILER_DUMP_FILE_NAME"};
inline constexpr const char *kZenithConnectionEnv{"ZENITH_CONNECTION"};
inline constexpr std::size_t kMaxRecords{100};

bool getEnvAsBool(const char *envName)
{
    const char *value = std::getenv(envName);
    if (!value || (value[0] == '\0'))
        return false;

    std::string stringValue(value);
    std::transform(stringValue.begin(), stringValue.end(), stringValue.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (stringValue == "1" || stringValue == "true" || stringValue == "yes" || stringValue == "on")
        return true;
    if (stringValue == "0" || stringValue == "false" || stringValue == "no" || stringValue == "off")
        return false;

    return false;
}

bool getProfilerEnabled()
{
    return getEnvAsBool(kProfilerEnv);
}

std::optional<std::string> getDumpFileName()
{
    const char *value = std::getenv(kProfilerDumpFileEnv);
    if (!value || value[0] == '\0')
        return std::nullopt;

    return std::string{value};
}

bool getZenithConnectionEnabled()
{
    return getEnvAsBool(kZenithConnectionEnv);
}

#ifdef RIALTO_ENABLE_AVMONITOR_CLIENT
// Sends AVMonitor events from a dedicated thread, so that the blocking COM-RPC call
// is never executed on GStreamer streaming threads.
class AvMonitorSender
{
public:
    struct Event
    {
        std::string name;
        std::string value;
        std::string timestamp;
        std::string id;
    };

    static AvMonitorSender &instance()
    {
        static AvMonitorSender sender;
        return sender;
    }

    void post(Event event)
    {
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            if (m_stop)
                return;
            if (m_queue.size() >= kMaxQueueSize)
            {
                RIALTO_COMMON_LOG_WARN("AVMonitor queue full, dropping event");
                return;
            }
            m_queue.push(std::move(event));
        }
        m_cv.notify_one();
    }

    AvMonitorSender(const AvMonitorSender &) = delete;
    AvMonitorSender &operator=(const AvMonitorSender &) = delete;

private:
    static constexpr std::size_t kMaxQueueSize{256};

    AvMonitorSender() : m_thread{[this] { run(); }} {}

    ~AvMonitorSender()
    {
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_stop = true;
        }
        m_cv.notify_one();
        if (m_thread.joinable())
            m_thread.join();
    }

    void run()
    {
        std::unique_lock<std::mutex> lock{m_mutex};
        while (true)
        {
            m_cv.wait(lock, [this] { return m_stop || !m_queue.empty(); });
            if (m_stop)
                break;

            Event event = std::move(m_queue.front());
            m_queue.pop();

            lock.unlock();
            const auto status = avmonitor_register_event(event.name.c_str(), event.value.c_str(),
                                                         event.timestamp.c_str(), event.id.c_str());
            if (status != AVMONITOR_OK)
            {
                RIALTO_COMMON_LOG_WARN("Failed to send profiler record to AVMonitor client library, status: %u",
                                       status);
            }
            lock.lock();
        }
    }

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<Event> m_queue;
    bool m_stop{false};
    std::thread m_thread;
};
#endif // RIALTO_ENABLE_AVMONITOR_CLIENT
} // namespace

namespace firebolt::rialto::common
{
std::shared_ptr<IProfilerFactory> IProfilerFactory::createFactory()
{
    std::shared_ptr<IProfilerFactory> factory;

    try
    {
        factory = std::make_shared<ProfilerFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_COMMON_LOG_ERROR("Failed to create the profiler factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IProfiler> ProfilerFactory::createProfiler(std::string moduleName) const
{
    return std::make_unique<Profiler>(moduleName);
}

Profiler::Profiler(std::string module)
    : m_module(std::move(module)),
      m_enabled(getProfilerEnabled()),
      m_zenithConnectionEnabled(getZenithConnectionEnabled()),
      m_dumpFileName(getDumpFileName())
{
}

bool Profiler::isEnabled() const noexcept
{
    return m_enabled;
}

std::optional<Profiler::RecordId> Profiler::record(const std::string &stage)
{
    if (!m_enabled)
        return std::nullopt;

    auto now = Clock::now();
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_records.size() >= kMaxRecords)
    {
        return std::nullopt;
    }

    const Profiler::RecordId id = m_id++;

    m_records.push_back(Record{m_module, id, stage, std::string{}, now});

    return id;
}

std::optional<Profiler::RecordId> Profiler::record(const std::string &stage, const std::string &info)
{
    if (!m_enabled)
        return std::nullopt;

    auto now = Clock::now();
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_records.size() >= kMaxRecords)
    {
        return std::nullopt;
    }

    const Profiler::RecordId id = m_id++;

    m_records.push_back(Record{m_module, id, stage, info, now});

    return id;
}

void Profiler::log(const RecordId id)
{
    if (!m_enabled)
        return;

    const auto record = findById(id);
    if (record)
    {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(record->time.time_since_epoch()).count();

        const auto idStr = std::to_string(static_cast<std::uint64_t>(record->id));
        const auto tsStr = std::to_string(static_cast<std::int64_t>(ms));
        const auto payload = std::string{"PROFILER | RECORD | MODULE["} + record->moduleName + "] " + "ID[" +
                     idStr + "] " + "STAGE[" + record->stage + "] " + "INFO[" + record->info + "] " +
                     "TIMESTAMP[" + tsStr + "]";

        RIALTO_COMMON_LOG_MIL("%s", payload.c_str());

#ifdef RIALTO_ENABLE_AVMONITOR_CLIENT
        if (m_zenithConnectionEnabled)
        {
            AvMonitorSender::instance().post({record->stage, record->info, tsStr, idStr});
        }
#endif
    }
}

bool Profiler::dumpToFile() const
{
    if (!m_dumpFileName)
        return false;

    if (!m_enabled)
        return false;

    std::vector<Record> copy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        copy = m_records;
    }

    std::ofstream out(*m_dumpFileName, std::ios::out | std::ios::app);
    if (!out.is_open())
        return false;

    for (const auto &record : copy)
    {
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(record.time.time_since_epoch()).count();

        out << "MODULE[" << record.moduleName << "] " << "ID[" << record.id << "] " << "STAGE[" << record.stage << "] "
            << "INFO[" << record.info << "] " << "TIMESTAMP[" << us << "]" << '\n';
    }

    return static_cast<bool>(out);
}

std::vector<IProfiler::Record> Profiler::getRecords() const
{
    if (!m_enabled)
        return {};

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_records;
}

std::optional<Profiler::Record> Profiler::findById(Profiler::RecordId id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = std::find_if(m_records.begin(), m_records.end(), [&](const auto &record) { return record.id == id; });

    if (it != m_records.end())
        return *it;

    return std::nullopt;
}

}; // namespace firebolt::rialto::common
