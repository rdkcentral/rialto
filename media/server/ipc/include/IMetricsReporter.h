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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_I_METRICS_REPORTER_H_
#define FIREBOLT_RIALTO_SERVER_IPC_I_METRICS_REPORTER_H_

#include "StateMetricsAggregator.h"
#include <cstdint>
#include <memory>
#include <string>

namespace firebolt::rialto::server::ipc
{
/**
 * @brief Periodic sample data reported each sampling interval.
 */
struct PeriodicMetricsReport
{
    std::uint64_t sampleId{0};
    std::string reason;
    std::string appName;
    std::uint32_t clientPid{0};
    double clientCpuPercent{0.0};
    double serverCpuPercent{0.0};
    double combinedCpuPercent{0.0};
    std::uint64_t clientCpuTimeMs{0};
    std::uint64_t serverCpuTimeMs{0};
    std::uint64_t clientMemoryKb{0};
    std::uint64_t serverMemoryKb{0};
    std::uint64_t cgroupMemoryUsageKb{0};
    std::uint64_t cgroupMemoryLimitKb{0};
};

/**
 * @brief Report emitted when a state period ends (playback state or application state).
 */
struct StateTransitionReport
{
    std::string context;   // e.g. "session=1" or "global"
    StateMetricsReport metrics;
};

/**
 * @brief Severity level for threshold alerts.
 */
enum class ThresholdSeverity
{
    WARNING,
    CRITICAL
};

/**
 * @brief Alert emitted when a metric exceeds a configured threshold.
 */
struct ThresholdAlert
{
    std::string metricName;
    double currentValue{0.0};
    double thresholdValue{0.0};
    ThresholdSeverity severity{ThresholdSeverity::WARNING};
};

/**
 * @brief Abstract interface for metrics output.
 *        Implementations can log, push to remote telemetry, or both.
 */
class IMetricsReporter
{
public:
    IMetricsReporter() = default;
    virtual ~IMetricsReporter() = default;

    IMetricsReporter(const IMetricsReporter &) = delete;
    IMetricsReporter &operator=(const IMetricsReporter &) = delete;
    IMetricsReporter(IMetricsReporter &&) = delete;
    IMetricsReporter &operator=(IMetricsReporter &&) = delete;

    virtual void reportPeriodicSample(const PeriodicMetricsReport &report) = 0;
    virtual void reportStateTransition(const StateTransitionReport &report) = 0;
    virtual void reportThresholdExceeded(const ThresholdAlert &alert) = 0;
};
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_I_METRICS_REPORTER_H_
