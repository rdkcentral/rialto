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

#ifndef FIREBOLT_RIALTO_SERVER_STATE_METRICS_AGGREGATOR_H_
#define FIREBOLT_RIALTO_SERVER_STATE_METRICS_AGGREGATOR_H_

#include "MetricsAccumulator.h"
#include <cstdint>
#include <string>

namespace firebolt::rialto::server
{
/**
 * @brief A single metrics sample to be fed into the aggregator.
 */
struct MetricsSample
{
    double clientCpuPercent{0.0};
    double serverCpuPercent{0.0};
    double combinedCpuPercent{0.0};
    std::uint64_t clientMemoryKb{0};
    std::uint64_t serverMemoryKb{0};
    std::uint64_t cgroupMemoryUsageKb{0};
    std::uint64_t cgroupMemoryLimitKb{0};
};

/**
 * @brief Aggregated statistics report produced when a state is finalized.
 */
struct StateMetricsReport
{
    std::string stateName;
    std::uint64_t durationMs{0};
    MetricsStatistics clientCpu;
    MetricsStatistics serverCpu;
    MetricsStatistics combinedCpu;
    MetricsStatistics clientMemoryKb;
    MetricsStatistics serverMemoryKb;
    MetricsStatistics cgroupMemoryUsageKb;
    MetricsStatistics cgroupMemoryLimitKb;
};

/**
 * @brief Accumulates metrics samples for a single state period and produces a
 *        statistical report on finalization.
 */
class StateMetricsAggregator
{
public:
    StateMetricsAggregator() = default;
    ~StateMetricsAggregator() = default;

    void begin(const std::string &stateName, std::uint64_t monotonicTimeMs)
    {
        reset();
        m_stateName = stateName;
        m_startTimeMs = monotonicTimeMs;
    }

    void addSample(const MetricsSample &sample)
    {
        m_clientCpu.addSample(sample.clientCpuPercent);
        m_serverCpu.addSample(sample.serverCpuPercent);
        m_combinedCpu.addSample(sample.combinedCpuPercent);
        m_clientMemory.addSample(static_cast<double>(sample.clientMemoryKb));
        m_serverMemory.addSample(static_cast<double>(sample.serverMemoryKb));
        m_cgroupUsage.addSample(static_cast<double>(sample.cgroupMemoryUsageKb));
        m_cgroupLimit.addSample(static_cast<double>(sample.cgroupMemoryLimitKb));
    }

    StateMetricsReport finalize(std::uint64_t monotonicTimeMs) const
    {
        StateMetricsReport report;
        report.stateName = m_stateName;
        report.durationMs = (monotonicTimeMs > m_startTimeMs) ? (monotonicTimeMs - m_startTimeMs) : 0;
        report.clientCpu = m_clientCpu.getStats();
        report.serverCpu = m_serverCpu.getStats();
        report.combinedCpu = m_combinedCpu.getStats();
        report.clientMemoryKb = m_clientMemory.getStats();
        report.serverMemoryKb = m_serverMemory.getStats();
        report.cgroupMemoryUsageKb = m_cgroupUsage.getStats();
        report.cgroupMemoryLimitKb = m_cgroupLimit.getStats();
        return report;
    }

    void reset()
    {
        m_stateName.clear();
        m_startTimeMs = 0;
        m_clientCpu.reset();
        m_serverCpu.reset();
        m_combinedCpu.reset();
        m_clientMemory.reset();
        m_serverMemory.reset();
        m_cgroupUsage.reset();
        m_cgroupLimit.reset();
    }

    bool hasData() const { return m_clientCpu.getCount() > 0; }
    const std::string &getStateName() const { return m_stateName; }

private:
    std::string m_stateName;
    std::uint64_t m_startTimeMs{0};
    MetricsAccumulator m_clientCpu;
    MetricsAccumulator m_serverCpu;
    MetricsAccumulator m_combinedCpu;
    MetricsAccumulator m_clientMemory;
    MetricsAccumulator m_serverMemory;
    MetricsAccumulator m_cgroupUsage;
    MetricsAccumulator m_cgroupLimit;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_STATE_METRICS_AGGREGATOR_H_
