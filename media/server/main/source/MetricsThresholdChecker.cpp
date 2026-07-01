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

#include "MetricsThresholdChecker.h"

namespace firebolt::rialto::server
{
MetricsThresholdChecker::MetricsThresholdChecker(MetricsThresholdConfig config, IMetricsReporter *reporter)
    : m_config{std::move(config)}, m_reporter{reporter}
{
}

void MetricsThresholdChecker::checkSample(double clientCpu, double serverCpu, double combinedCpu,
                                           std::uint64_t clientMemKb, std::uint64_t serverMemKb,
                                           std::uint64_t cgroupUsageKb, std::uint64_t cgroupLimitKb)
{
    if (!m_reporter)
    {
        return;
    }

    checkMetric(m_config.clientCpu, clientCpu, m_clientCpuState);
    checkMetric(m_config.serverCpu, serverCpu, m_serverCpuState);
    checkMetric(m_config.combinedCpu, combinedCpu, m_combinedCpuState);
    checkMetric(m_config.clientMemoryKb, static_cast<double>(clientMemKb), m_clientMemState);
    checkMetric(m_config.serverMemoryKb, static_cast<double>(serverMemKb), m_serverMemState);

    // Cgroup memory as percentage of limit
    if (cgroupLimitKb > 0)
    {
        const double cgroupPct{(static_cast<double>(cgroupUsageKb) / static_cast<double>(cgroupLimitKb)) * 100.0};
        checkMetric(m_config.cgroupMemoryPercent, cgroupPct, m_cgroupMemState);
    }
}

void MetricsThresholdChecker::checkMetric(const MetricsThreshold &threshold, double value, ThresholdState &state)
{
    // Critical check
    if (value >= threshold.criticalLevel)
    {
        state.belowCriticalCount = 0;
        if (!state.criticalFired)
        {
            state.criticalFired = true;
            ThresholdAlert alert;
            alert.metricName = threshold.metricName;
            alert.currentValue = value;
            alert.thresholdValue = threshold.criticalLevel;
            alert.severity = ThresholdSeverity::CRITICAL;
            m_reporter->reportThresholdExceeded(alert);
        }
    }
    else
    {
        ++state.belowCriticalCount;
        if (state.belowCriticalCount >= kDebounceSamples)
        {
            state.criticalFired = false;
        }
    }

    // Warning check
    if (value >= threshold.warningLevel)
    {
        state.belowWarningCount = 0;
        if (!state.warningFired)
        {
            state.warningFired = true;
            ThresholdAlert alert;
            alert.metricName = threshold.metricName;
            alert.currentValue = value;
            alert.thresholdValue = threshold.warningLevel;
            alert.severity = ThresholdSeverity::WARNING;
            m_reporter->reportThresholdExceeded(alert);
        }
    }
    else
    {
        ++state.belowWarningCount;
        if (state.belowWarningCount >= kDebounceSamples)
        {
            state.warningFired = false;
        }
    }
}
} // namespace firebolt::rialto::server
