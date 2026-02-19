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

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>

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
    : m_module(std::move(module)), m_enabled(parseEnv(std::getenv(kProfilerEnv), false))
{
}

bool Profiler::enabled() const noexcept
{
    return m_enabled;
}

std::optional<Profiler::RecordId> Profiler::record(std::string stage)
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

    m_records.push_back(Record{m_module, id, std::move(stage), std::string{}, now});

    return id;
}

std::optional<Profiler::RecordId> Profiler::record(std::string stage, std::string info)
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

    m_records.push_back(Record{m_module, id, std::move(stage), std::move(info), now});

    return id;
}

std::optional<Profiler::RecordId> Profiler::find(std::string stage)
{
    if (!m_enabled)
        return std::nullopt;

    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it =
        std::find_if(m_records.begin(), m_records.end(), [&](const auto &record) { return record.stage == stage; });

    if (it != m_records.end())
        return it->id;

    return std::nullopt;
}

std::optional<Profiler::RecordId> Profiler::find(std::string stage, std::string info)
{
    if (!m_enabled)
        return std::nullopt;

    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = std::find_if(m_records.begin(), m_records.end(),
                                 [&](const auto &record) { return record.stage == stage && record.info == info; });
    if (it != m_records.end())
        return it->id;

    return std::nullopt;
}

void Profiler::log(const RecordId id)
{
    if (!m_enabled)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    const auto *record = findById(id);
    if (record)
    {
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(record->time.time_since_epoch()).count();

        const auto idStr = std::to_string(static_cast<std::uint64_t>(record->id));
        const auto tsStr = std::to_string(static_cast<std::int64_t>(us));

        RIALTO_COMMON_LOG_MIL("PROFILER | RECORD | MODULE[%s] ID[%s] STAGE[%s] INFO[%s] TIMESTAMP[%s]",
                              record->module.c_str(), idStr.c_str(), record->stage.c_str(), record->info.c_str(),
                              tsStr.c_str());
    }
}

bool Profiler::dump(const std::string &path) const
{
    if (!m_enabled)
        return false;

    std::vector<Record> copy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        copy = m_records;
    }

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.is_open())
        return false;

    for (const auto &record : copy)
    {
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(record.time.time_since_epoch()).count();

        out << "MODULE[" << record.module << "] " << "ID[" << record.id << "] " << "STAGE[" << record.stage << "] "
            << "INFO[" << record.info << "] " << "TIMESTAMP[" << us << "]" << '\n';
    }

    return static_cast<bool>(out);
}

bool Profiler::parseEnv(const char *value, bool defaultValue)
{
    if (!value || (value[0] == '\0'))
        return defaultValue;

    std::string stringValue(value);
    std::transform(stringValue.begin(), stringValue.end(), stringValue.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (stringValue == "1" || stringValue == "true" || stringValue == "yes" || stringValue == "on")
        return true;
    if (stringValue == "0" || stringValue == "false" || stringValue == "no" || stringValue == "off")
        return false;

    return defaultValue;
}

const Profiler::Record *Profiler::findById(Profiler::RecordId id)
{
    const auto it = std::find_if(m_records.begin(), m_records.end(), [&](const auto &record) { return record.id == id; });

    if (it != m_records.end())
        return &(*it);

    return nullptr;
}

}; // namespace firebolt::rialto::common
