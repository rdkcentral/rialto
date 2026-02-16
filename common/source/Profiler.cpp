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

#include "Profiler.h"
#include "RialtoCommonLogging.h"

namespace firebolt::rialto::common
{
std::weak_ptr<IProfilerFactory> ProfilerFactory::m_factory;

std::shared_ptr<IProfilerFactory> IProfilerFactory::getFactory()
{
    std::shared_ptr<IProfilerFactory> factory = ProfilerFactory::m_factory.lock();

    if (!factory)
    {
        try
        {
            factory = std::make_shared<ProfilerFactory>();
        }
        catch (const std::exception &e)
        {
            RIALTO_COMMON_LOG_ERROR("Failed to create the profiler factory, reason: %s", e.what());
        }

        ProfilerFactory::m_factory = factory;
    }

    return factory;
}

std::unique_ptr<IProfiler> ProfilerFactory::createProfiler(const std::string &name) const
{
    return std::make_unique<Profiler>(name);
}

Profiler::Profiler(const std::string &name) : m_name(name), m_isRunning(false)
{
}

void Profiler::start()
{
    m_startTime = std::chrono::high_resolution_clock::now();
    m_isRunning = true;
}

double Profiler::stop()
{
    if (!m_isRunning)
    {
        return 0.0;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = endTime - m_startTime;
    m_isRunning = false;

    return elapsed.count();
}

void Profiler::reset()
{
    m_isRunning = false;
}
} // namespace firebolt::rialto::common
