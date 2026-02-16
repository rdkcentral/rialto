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

#ifndef FIREBOLT_RIALTO_COMMON_I_PROFILER_H_
#define FIREBOLT_RIALTO_COMMON_I_PROFILER_H_

#include <memory>
#include <string>

namespace firebolt::rialto::common
{
class IProfiler;

/**
 * @brief IProfiler factory class, returns a concrete implementation of IProfiler
 */
class IProfilerFactory
{
public:
    IProfilerFactory() = default;
    virtual ~IProfilerFactory() = default;

    /**
     * @brief Gets the IProfilerFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IProfilerFactory> getFactory();

    /**
     * @brief Creates an IProfiler object.
     *
     * @param[in] name : Name of the profiler instance
     *
     * @retval the new profiler instance or null on error.
     */
    virtual std::unique_ptr<IProfiler> createProfiler(const std::string &name) const = 0;
};

class IProfiler
{
public:
    IProfiler() = default;
    virtual ~IProfiler() = default;

    IProfiler(const IProfiler &) = delete;
    IProfiler &operator=(const IProfiler &) = delete;
    IProfiler(IProfiler &&) = delete;
    IProfiler &operator=(IProfiler &&) = delete;

    /**
     * @brief Starts profiling
     */
    virtual void start() = 0;

    /**
     * @brief Stops profiling and returns the elapsed time in milliseconds
     *
     * @retval elapsed time in milliseconds
     */
    virtual double stop() = 0;

    /**
     * @brief Resets the profiler
     */
    virtual void reset() = 0;
};

} // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_I_PROFILER_H_
