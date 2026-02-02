#ifndef FIREBOLT_RIALTO_COMMON_PROFILER_H_
#define FIREBOLT_RIALTO_COMMON_PROFILER_H_

#include <chrono>
#include <mutex>
#include <string_view>
#include <vector>
#include <optional>

namespace firebolt::rialto::common
{
class Profiler final
{
public:
    using RecordId = std::uint64_t;
    using Clock = std::chrono::steady_clock;

    struct Record
    {
        std::string_view module;
        uint64_t id{0};
        std::string stage;
        std::string info;
        Clock::time_point time;
    };

    Profiler() = delete;
    ~Profiler() = default;

    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    Profiler(std::string_view module);

    std::optional<RecordId> record(std::string stage);
    std::optional<RecordId> record(std::string stage, std::string info);

    std::optional<RecordId> find(std::string stage);
    std::optional<RecordId> find(std::string stage, std::string info);

    void log(const RecordId id);

    void dump(std::ostream& os) const;

private:
    const Record* findById(RecordId id);

    mutable std::mutex m_mutex;
    std::string_view m_module{};
    RecordId m_id{1};
    std::vector<Record> m_records;
    static constexpr size_t kMaxRecords = 100;
};

}; // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_PROFILER_H_
