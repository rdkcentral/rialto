/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#ifndef FIREBOLT_RIALTO_COMMON_PROFILER_H_
#define FIREBOLT_RIALTO_COMMON_PROFILER_H_

#include "IProfiler.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

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

    bool enabled() const noexcept override;

    std::optional<RecordId> record(std::string stage) override;
    std::optional<RecordId> record(std::string stage, std::string info) override;

    std::optional<RecordId> find(std::string stage) override;
    std::optional<RecordId> find(std::string stage, std::string info) override;

    void log(const RecordId id) override;

    bool dump(const std::string &path) const override;

private:
    static bool parseEnv(const char *value, bool defaultValue);
    const Record *findById(RecordId id);

    std::string m_module;
    const bool m_enabled;

    mutable std::mutex m_mutex;
    RecordId m_id{1};
    std::vector<Record> m_records;

    static constexpr const char *kProfilerEnv = "PROFILER_ENABLED";
    static constexpr size_t kMaxRecords = 100;
};

}; // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_PROFILER_H_
