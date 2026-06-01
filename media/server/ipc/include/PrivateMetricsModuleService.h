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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_PRIVATE_METRICS_MODULE_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_IPC_PRIVATE_METRICS_MODULE_SERVICE_H_

#include "IMetricsReporter.h"
#include "IPrivateMetricsModuleService.h"
#include "MetricsThresholdChecker.h"
#include "StateMetricsAggregator.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace firebolt::rialto::server::ipc
{
class PrivateMetricsModuleServiceFactory : public IPrivateMetricsModuleServiceFactory
{
public:
    PrivateMetricsModuleServiceFactory() = default;
    ~PrivateMetricsModuleServiceFactory() override = default;

    std::shared_ptr<IPrivateMetricsModuleService> create() const override;
};

class PrivateMetricsModuleService : public IPrivateMetricsModuleService
{
public:
    PrivateMetricsModuleService();
    ~PrivateMetricsModuleService() override;

    void clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) override;
    void clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) override;

    void notifyPlaybackStateChanged(int sessionId, PlaybackState oldState, PlaybackState newState) override;
    void notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState) override;

    void reportClientMetrics(::google::protobuf::RpcController *controller,
                             const ::firebolt::rialto::ReportClientMetricsRequest *request,
                             ::firebolt::rialto::ReportClientMetricsResponse *response,
                             ::google::protobuf::Closure *done) override;
    void notifyClientReady(::google::protobuf::RpcController *controller,
                           const ::firebolt::rialto::NotifyClientReadyRequest *request,
                           ::firebolt::rialto::NotifyClientReadyResponse *response,
                           ::google::protobuf::Closure *done) override;

private:
    struct ProcessMetricsSample
    {
        std::uint64_t monotonicTimeMs;
        std::uint64_t epochTimeMs;
        std::uint64_t processCpuTimeMs;
        std::uint64_t processMemoryKb;
        std::uint64_t cgroupMemoryUsageKb;
        std::uint64_t cgroupMemoryLimitKb;
    };

    struct MetricsSamplePair
    {
        ::firebolt::rialto::ClientProcessMetrics clientMetrics;
        ProcessMetricsSample serverMetrics;
    };

    struct ClientMetricsState
    {
        bool isReady{false};
        std::optional<MetricsSamplePair> latestMetrics;
    };

    struct SessionMetricsState
    {
        PlaybackState currentPlaybackState{PlaybackState::UNKNOWN};
        StateMetricsAggregator aggregator;
    };

    void runMetricsSampler();
    void requestMetricsSample(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient,
                              ::firebolt::rialto::MetricsSampleReason reason);
    ProcessMetricsSample getProcessMetricsSample() const;
    void logMetrics(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient,
                    const ::firebolt::rialto::ClientProcessMetrics &clientMetrics,
                    const ProcessMetricsSample &serverMetrics);
    void logStateReport(const StateMetricsReport &report, const std::string &context);
    const char *sampleReasonToString(::firebolt::rialto::MetricsSampleReason reason) const;
    static const char *playbackStateToString(PlaybackState state);
    static const char *applicationStateToString(ApplicationState state);
    double calculateCpuPercentage(std::uint64_t currentCpuTimeMs, std::uint64_t previousCpuTimeMs,
                                  std::uint64_t currentMonotonicTimeMs, std::uint64_t previousMonotonicTimeMs) const;

private:
    std::atomic<bool> m_isRunning;
    std::atomic<std::uint64_t> m_nextSampleId;
    std::thread m_metricsThread;
    std::condition_variable m_wakeup;
    std::mutex m_mutex;
    std::map<std::shared_ptr<::firebolt::rialto::ipc::IClient>, ClientMetricsState> m_clients;

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
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_PRIVATE_METRICS_MODULE_SERVICE_H_
