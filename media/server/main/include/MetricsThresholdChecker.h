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

#ifndef FIREBOLT_RIALTO_SERVER_METRICS_THRESHOLD_CHECKER_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_THRESHOLD_CHECKER_H_

#include "IMetricsReporter.h"
#include <cstdint>
#include <string>
#include <vector>

namespace firebolt::rialto::server
{
/**
 * @brief Configuration for a single metric threshold.
 */
struct MetricsThreshold
{
    std::string metricName;
    double warningLevel{0.0};
    double criticalLevel{0.0};
};

/**
 * @brief Complete threshold configuration.
 */
struct MetricsThresholdConfig
{
    MetricsThreshold clientCpu{"client_cpu", 80.0, 95.0};
    MetricsThreshold serverCpu{"server_cpu", 80.0, 95.0};
    MetricsThreshold combinedCpu{"combined_cpu", 150.0, 190.0};
    MetricsThreshold clientMemoryKb{"client_mem_kb", 512000.0, 768000.0};
    MetricsThreshold serverMemoryKb{"server_mem_kb", 512000.0, 768000.0};
    MetricsThreshold cgroupMemoryPercent{"cgroup_mem_pct", 80.0, 95.0};
};

/**
 * @brief Checks metric samples against configured thresholds with debounce.
 *
 * An alert fires when a metric exceeds the threshold.
 * The alert resets (can fire again) only after the metric drops below the threshold
 * for at least kDebounceSamples consecutive samples.
 */
class MetricsThresholdChecker
{
public:
    explicit MetricsThresholdChecker(MetricsThresholdConfig config, IMetricsReporter *reporter);
    ~MetricsThresholdChecker() = default;

    void checkSample(double clientCpu, double serverCpu, double combinedCpu, std::uint64_t clientMemKb,
                     std::uint64_t serverMemKb, std::uint64_t cgroupUsageKb, std::uint64_t cgroupLimitKb);

private:
    static constexpr int kDebounceSamples{2};

    struct ThresholdState
    {
        bool warningFired{false};
        bool criticalFired{false};
        int belowWarningCount{0};
        int belowCriticalCount{0};
    };

    void checkMetric(const MetricsThreshold &threshold, double value, ThresholdState &state);

    MetricsThresholdConfig m_config;
    IMetricsReporter *m_reporter; // non-owning
    ThresholdState m_clientCpuState;
    ThresholdState m_serverCpuState;
    ThresholdState m_combinedCpuState;
    ThresholdState m_clientMemState;
    ThresholdState m_serverMemState;
    ThresholdState m_cgroupMemState;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_METRICS_THRESHOLD_CHECKER_H_
