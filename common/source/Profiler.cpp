#include "Profiler.h"

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

    m_records.push_back(Record{m_module, m_id, stage, now});
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
