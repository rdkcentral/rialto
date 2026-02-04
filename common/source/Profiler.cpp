#include "Profiler.h"
#include "RialtoCommonLogging.h"

#include <algorithm>
#include <fstream>

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
    : m_module(std::move(module))
{

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
        m_module,
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
        m_module,
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
                            record->module.c_str(), static_cast<unsigned long long>(record->id),
                            record->stage.c_str(), record->info.c_str(), (long long)us);
    }
}

bool Profiler::dump(const std::string& path) const
{
    std::vector<Record> copy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        copy = m_records;
    }

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.is_open())
        return false;

    for (const auto& record : copy)
    {
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                                record.time.time_since_epoch()).count();

        out << "MODULE[" << record.module << "] "
            << "ID[" << record.id << "] "
            << "STAGE[" << record.stage << "] "
            << "INFO[" << record.info << "] "
            << "TIMESTAMP[" << us << "]"
            << '\n';
    }

    return static_cast<bool>(out);
}

const Profiler::Record* Profiler::findById(Profiler::RecordId id)
{
    for (const auto& record : m_records)
    {
        if (record.id == id)
            return &record;
    }

    return nullptr;
}

}; // namespace firebolt::rialto::common
