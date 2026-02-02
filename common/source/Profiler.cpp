#include "Profiler.h"
#include "RialtoCommonLogging.h"

#include <algorithm>

namespace firebolt::rialto::common
{
Profiler::Profiler(std::string_view module)
    : m_module(module)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_records.clear();
}

std::optional<Profiler::RecordId> Profiler::record(std::string stage)
{
    auto now = Clock::now();
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_records.size() >= kMaxRecords)
    {
        return std::nullopt;
    }

    const Profiler::RecordId id = m_id++;

    m_records.push_back(Record{
        std::string(m_module),
        id,
        std::move(stage),
        std::string{},
        now
    });

    return id;
}

std::optional<Profiler::RecordId> Profiler::record(std::string stage, std::string info)
{
    auto now = Clock::now();
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_records.size() >= kMaxRecords)
    {
        return std::nullopt;
    }

    const Profiler::RecordId id = m_id++;

    m_records.push_back(Record{
        std::string(m_module),
        id,
        std::move(stage),
        std::move(info),
        now
    });

    return id;
}

std::optional<Profiler::RecordId> Profiler::find(std::string stage)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& record : m_records)
        if (record.stage == stage)
            return record.id;

    return std::nullopt;
}

std::optional<Profiler::RecordId> Profiler::find(std::string stage, std::string info)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& record : m_records)
        if (record.stage == stage && record.info == info)
            return record.id;

    return std::nullopt;
}

void Profiler::log(const RecordId id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto* record = findById(id);
    if(record)
    {
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(record->time.time_since_epoch()).count();
        RIALTO_COMMON_LOG_MIL("PROFILER | RECORD | MODULE[%s] ID[%llu] STAGE[%s] INFO[%s] TIMESTAMP[%lld]",
                            record->module.data(), static_cast<unsigned long long>(record->id),
                            record->stage.c_str(), record->info.c_str(), (long long)us);
    }
}

void Profiler::dump(std::ostream& os) const
{
    // TODO dump()
}

const Profiler::Record* Profiler::findById(Profiler::RecordId id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& record : m_records)
    {
        if (record.id == id)
            return &record;
    }

    return nullptr;
}

}; // namespace firebolt::rialto::common
