/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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
#include <string>

namespace firebolt::rialto::common
{
/**
 * @brief IProfilerFactory factory class definition.
 */
class ProfilerFactory : public IProfilerFactory
{
public:
    /**
     * @brief Weak pointer to the singleton factory object.
     */
    static std::weak_ptr<IProfilerFactory> m_factory;

    std::unique_ptr<IProfiler> createProfiler(const std::string &name) const override;
};

class Profiler : public IProfiler
{
public:
    explicit Profiler(const std::string &name);
    ~Profiler() = default;
    Profiler(const Profiler &) = delete;
    Profiler(Profiler &&) = delete;
    Profiler &operator=(const Profiler &) = delete;
    Profiler &operator=(Profiler &&) = delete;

    void start() override;
    double stop() override;
    void reset() override;

private:
    std::string m_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
    bool m_isRunning;
};
} // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_PROFILER_H_
