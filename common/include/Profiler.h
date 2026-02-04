#ifndef FIREBOLT_RIALTO_COMMON_PROFILER_H_
#define FIREBOLT_RIALTO_COMMON_PROFILER_H_

#include "IProfiler.h"

#include <chrono>
#include <mutex>
#include <string_view>
#include <vector>
#include <optional>

namespace firebolt::rialto::common
{
class ProfilerFactory : public IProfilerFactory
{
public:
    std::unique_ptr<IProfiler> createProfiler(std::string moduleName) const override;
};

class Profiler final : public IProfiler
{
public:
    using Clock = std::chrono::steady_clock;

    struct Record
    {
        std::string module;
        uint64_t id{0};
        std::string stage;
        std::string info;
        Clock::time_point time;
    };

    explicit Profiler(std::string module);

    std::optional<RecordId> record(std::string stage) override;
    std::optional<RecordId> record(std::string stage, std::string info) override;

    std::optional<RecordId> find(std::string stage) override;
    std::optional<RecordId> find(std::string stage, std::string info) override;

    void log(const RecordId id) override;

    bool dump(const std::string& path) const override;

private:
    const Record* findById(RecordId id);

    mutable std::mutex m_mutex;
    std::string m_module;
    RecordId m_id{1};
    std::vector<Record> m_records;
    static constexpr size_t kMaxRecords = 100;
};

}; // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_PROFILER_H_
