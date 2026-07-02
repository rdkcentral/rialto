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

#include "LogMetricsReporter.h"
#include "RialtoServerLogging.h"
#include <cinttypes>

namespace firebolt::rialto::server
{
void LogMetricsReporter::reportPeriodicSample(const PeriodicMetricsReport &report)
{
    RIALTO_SERVER_LOG_MIL("Metrics sample=%" PRIu64 ", reason=%s, app='%s', client_pid=%u, client_cpu=%.2f%%, "
                          "server_cpu=%.2f%%, combined_cpu=%.2f%%, client_cpu_ms=%" PRIu64 ", "
                          "server_cpu_ms=%" PRIu64 ", client_mem_kb=%" PRIu64 ", server_mem_kb=%" PRIu64 ", "
                          "shm_mem_kb=%" PRIu64 ", cgroup_mem_kb=%" PRIu64 "/%" PRIu64,
                          report.sampleId, report.reason.c_str(), report.appName.c_str(), report.clientPid,
                          report.clientCpuPercent, report.serverCpuPercent, report.combinedCpuPercent,
                          report.clientCpuTimeMs, report.serverCpuTimeMs, report.clientMemoryKb,
                          report.serverMemoryKb, report.shmMemoryKb, report.cgroupMemoryUsageKb, report.cgroupMemoryLimitKb);
}

void LogMetricsReporter::reportStateTransition(const StateTransitionReport &report)
{
    const auto &r{report.metrics};
    RIALTO_SERVER_LOG_MIL("Metrics state report [%s] state='%s', duration_ms=%" PRIu64 ", samples=%" PRIu64 ", "
                          "client_cpu={min=%.2f, max=%.2f, mean=%.2f, stddev=%.2f}%%, "
                          "server_cpu={min=%.2f, max=%.2f, mean=%.2f, stddev=%.2f}%%, "
                          "combined_cpu={min=%.2f, max=%.2f, mean=%.2f, stddev=%.2f}%%, "
                          "client_mem_kb={min=%.0f, max=%.0f, mean=%.0f}, "
                          "server_mem_kb={min=%.0f, max=%.0f, mean=%.0f}, "
                          "cgroup_mem_kb={min=%.0f, max=%.0f, mean=%.0f}",
                          report.context.c_str(), r.stateName.c_str(), r.durationMs, r.clientCpu.count,
                          r.clientCpu.min, r.clientCpu.max, r.clientCpu.mean, r.clientCpu.stddev, r.serverCpu.min,
                          r.serverCpu.max, r.serverCpu.mean, r.serverCpu.stddev, r.combinedCpu.min, r.combinedCpu.max,
                          r.combinedCpu.mean, r.combinedCpu.stddev, r.clientMemoryKb.min, r.clientMemoryKb.max,
                          r.clientMemoryKb.mean, r.serverMemoryKb.min, r.serverMemoryKb.max, r.serverMemoryKb.mean,
                          r.cgroupMemoryUsageKb.min, r.cgroupMemoryUsageKb.max, r.cgroupMemoryUsageKb.mean);
}

void LogMetricsReporter::reportThresholdExceeded(const ThresholdAlert &alert)
{
    const char *severity = (alert.severity == ThresholdSeverity::CRITICAL) ? "CRITICAL" : "WARNING";
    RIALTO_SERVER_LOG_WARN("Metrics threshold %s: %s=%.2f exceeds %.2f", severity, alert.metricName.c_str(),
                           alert.currentValue, alert.thresholdValue);
}
} // namespace firebolt::rialto::server
