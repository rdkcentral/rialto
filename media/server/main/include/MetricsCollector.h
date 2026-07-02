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

#ifndef FIREBOLT_RIALTO_SERVER_METRICS_COLLECTOR_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_COLLECTOR_H_

#include "IMetricsCollector.h"
#include "IMetricsReporter.h"
#include "ITimer.h"
#include "MetricsThresholdChecker.h"
#include "StateMetricsAggregator.h"
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace firebolt::rialto::server
{
class MetricsCollectorFactory : public IMetricsCollectorFactory
{
public:
    MetricsCollectorFactory() = default;
    ~MetricsCollectorFactory() override = default;

    std::unique_ptr<IMetricsCollector> create(int clientId,
                                              const std::shared_ptr<IMetricsCollectorClient> &client) override;
};

class MetricsCollector : public IMetricsCollector
{
public:
    MetricsCollector(int clientId, const std::shared_ptr<IMetricsCollectorClient> &client,
                     const std::shared_ptr<firebolt::rialto::common::ITimerFactory> &timerFactory);
    ~MetricsCollector() override;

    void processMetrics(const ClientMetricsData &metrics) override;
    void notifyPlaybackStateChanged(int sessionId, PlaybackState oldState, PlaybackState newState) override;
    void notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState) override;

private:
    struct ProcessMetricsSample
    {
        std::uint64_t monotonicTimeMs{0};
        std::uint64_t epochTimeMs{0};
        std::uint64_t processCpuTimeMs{0};
        std::uint64_t processMemoryKb{0};
        std::uint64_t cgroupMemoryUsageKb{0};
        std::uint64_t cgroupMemoryLimitKb{0};
        std::uint64_t shmMemoryKb{0};
    };

    struct PreviousSample
    {
        std::uint64_t clientMonotonicTimeMs{0};
        std::uint64_t clientCpuTimeMs{0};
        std::uint64_t clientMemoryKb{0};
        ProcessMetricsSample serverMetrics;
    };

    struct SessionMetricsState
    {
        PlaybackState currentPlaybackState{PlaybackState::UNKNOWN};
        StateMetricsAggregator aggregator;
    };

    void onTimerFired();
    ProcessMetricsSample getServerMetrics() const;
    double calculateCpuPercentage(std::uint64_t currentCpuTimeMs, std::uint64_t previousCpuTimeMs,
                                  std::uint64_t currentMonotonicTimeMs, std::uint64_t previousMonotonicTimeMs) const;
    static const char *sampleReasonToString(MetricsSampleReason reason);
    static const char *playbackStateToString(PlaybackState state);
    static const char *applicationStateToString(ApplicationState state);

    const int m_clientId;
    std::shared_ptr<IMetricsCollectorClient> m_client;
    std::unique_ptr<firebolt::rialto::common::ITimer> m_timer;
    std::uint64_t m_nextSampleId{1};

    std::mutex m_mutex;
    std::optional<PreviousSample> m_previousSample;

    // Per-session state tracking (sessionId -> session state)
    std::map<int, SessionMetricsState> m_sessionStates;

    // Global aggregator (active across all sessions while RUNNING)
    StateMetricsAggregator m_globalAggregator;
    ApplicationState m_currentApplicationState{ApplicationState::UNKNOWN};

    // Pluggable metrics reporter (log, telemetry, or composite)
    std::unique_ptr<IMetricsReporter> m_reporter;

    // Threshold checker
    MetricsThresholdChecker m_thresholdChecker;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_METRICS_COLLECTOR_H_
