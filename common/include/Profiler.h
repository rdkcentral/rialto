#ifndef FIREBOLT_RIALTO_COMMON_PROFILER_H_
#define FIREBOLT_RIALTO_COMMON_PROFILER_H_

#include <chrono>
#include <mutex>
#include <string_view>
#include <vector>

// namespace firebolt::rialto::common
// {
class Profiler final
{
public:
    using Clock = std::chrono::steady_clock;

    struct Record
    {
        std::string_view module;
        uint64_t id{0};
        std::string stage;
        Clock::time_point time;
    };

    Profiler() = delete;
    ~Profiler() = default;

    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    Profiler(std::string_view module, uint64_t id);

    void record(std::string stage);

    std::vector<Record> snapshot() const;

    void dump(std::ostream& os) const;

private:
    mutable std::mutex m_mutex;
    std::string_view m_module{};
    uint64_t m_id{0};
    std::vector<Record> m_records;
};

// }; // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_PROFILER_H_
