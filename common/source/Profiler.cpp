#include "Profiler.h"
#include "RialtoCommonLogging.h"

#include <algorithm>

// namespace firebolt::rialto::common
// {
Profiler::Profiler(std::string_view module, uint64_t id)
    : m_module(module), m_id(id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_records.clear();
}

void Profiler::record(std::string stage)
{
    auto now = Clock::now();
    std::lock_guard<std::mutex> lock(m_mutex);

    Record record = Record{m_module, m_id, stage, now};
    m_records.push_back(record);


    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(record.time.time_since_epoch()).count();
    RIALTO_COMMON_LOG_FATAL("RECORD: Module[%s] id[%llu] stage=%s time_since_epoch_ns=%lld",
                            record.module.data(), static_cast<unsigned long long>(record.id), record.stage.c_str(), (long long)ns);
}

std::vector<Profiler::Record> Profiler::snapshot() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_records;
}

void Profiler::dump(std::ostream& os) const
{
    auto copy = snapshot();
    if (copy.empty())
        return;

    std::sort(copy.begin(), copy.end(),
            [](const Record& x, const Record& y) { return x.time < y.time; });

    // TODO dump()
}

// }; // namespace firebolt::rialto::common
