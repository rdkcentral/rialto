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

#include "PrivateMetricsModuleService.h"
#include "LogMetricsReporter.h"
#include "RialtoServerLogging.h"
#include <IIpcController.h>
#include <chrono>
#include <cinttypes>
#include <fstream>
#include <optional>
#include <string>
#include <sys/times.h>
#include <unistd.h>
#include <vector>

namespace
{
constexpr std::chrono::seconds kMetricsInterval{15};
} // namespace

namespace firebolt::rialto::server::ipc
{
std::shared_ptr<IPrivateMetricsModuleServiceFactory> IPrivateMetricsModuleServiceFactory::createFactory()
{
    std::shared_ptr<IPrivateMetricsModuleServiceFactory> factory;

    try
    {
        factory = std::make_shared<PrivateMetricsModuleServiceFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the rialto private metrics module service factory, reason: %s",
                                e.what());
    }

    return factory;
}

std::shared_ptr<IPrivateMetricsModuleService> PrivateMetricsModuleServiceFactory::create() const
{
    std::shared_ptr<IPrivateMetricsModuleService> privateMetricsModule;

    try
    {
        privateMetricsModule = std::make_shared<PrivateMetricsModuleService>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the rialto private metrics module service, reason: %s", e.what());
    }

    return privateMetricsModule;
}

PrivateMetricsModuleService::PrivateMetricsModuleService()
    : m_isRunning{true}, m_nextSampleId{1}, m_reporter{std::make_unique<LogMetricsReporter>()},
      m_thresholdChecker{MetricsThresholdConfig{}, m_reporter.get()}
{
    m_metricsThread = std::thread(&PrivateMetricsModuleService::runMetricsSampler, this);
}

PrivateMetricsModuleService::~PrivateMetricsModuleService()
{
    m_isRunning.store(false);
    m_wakeup.notify_all();
    if (m_metricsThread.joinable())
    {
        m_metricsThread.join();
    }
}

void PrivateMetricsModuleService::clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client connected to private metrics module");
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_clients.emplace(ipcClient, ClientMetricsState{});
    }
    ipcClient->exportService(shared_from_this());
}

void PrivateMetricsModuleService::clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client disconnected from private metrics module");
    std::lock_guard<std::mutex> lock{m_mutex};
    m_clients.erase(ipcClient);
}

void PrivateMetricsModuleService::notifyClientReady(::google::protobuf::RpcController *controller,
                                                    const ::firebolt::rialto::NotifyClientReadyRequest *request,
                                                    ::firebolt::rialto::NotifyClientReadyResponse *response,
                                                    ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }

    auto ipcClient{ipcController->getClient()};
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        auto clientIter{m_clients.find(ipcClient)};
        if (m_clients.end() == clientIter)
        {
            RIALTO_SERVER_LOG_WARN("Ignoring private metrics ready notification from unknown client");
            done->Run();
            return;
        }
        clientIter->second.isReady = true;
    }

    RIALTO_SERVER_LOG_MIL("Client ready for private metrics samples");
    done->Run();
    requestMetricsSample(ipcClient, firebolt::rialto::METRICS_SAMPLE_REASON_CONNECTED);
}

void PrivateMetricsModuleService::reportClientMetrics(::google::protobuf::RpcController *controller,
                                                      const ::firebolt::rialto::ReportClientMetricsRequest *request,
                                                      ::firebolt::rialto::ReportClientMetricsResponse *response,
                                                      ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }
    if (!request->has_metrics())
    {
        RIALTO_SERVER_LOG_ERROR("reportClientMetrics request missing metrics");
        controller->SetFailed("Missing metrics");
        done->Run();
        return;
    }

    const auto &metrics{request->metrics()};
    const auto kServerMetrics{getProcessMetricsSample()};
    auto ipcClient{ipcController->getClient()};
    logMetrics(ipcClient, metrics, kServerMetrics);
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        auto &clientState{m_clients[ipcClient]};
        clientState.isReady = true;
        clientState.latestMetrics = MetricsSamplePair{metrics, kServerMetrics};
    }

    done->Run();
}

void PrivateMetricsModuleService::runMetricsSampler()
{
    while (m_isRunning.load())
    {
        std::vector<std::shared_ptr<::firebolt::rialto::ipc::IClient>> clients;
        {
            std::unique_lock<std::mutex> lock{m_mutex};
            m_wakeup.wait_for(lock, kMetricsInterval, [this]() { return !m_isRunning.load(); });
            if (!m_isRunning.load())
            {
                break;
            }
            for (const auto &client : m_clients)
            {
                if (client.second.isReady)
                {
                    clients.push_back(client.first);
                }
            }
        }

        for (const auto &client : clients)
        {
            requestMetricsSample(client, firebolt::rialto::METRICS_SAMPLE_REASON_PERIODIC);
        }
    }
}

void PrivateMetricsModuleService::requestMetricsSample(
    const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient,
    ::firebolt::rialto::MetricsSampleReason reason)
{
    if (!ipcClient || !ipcClient->isConnected())
    {
        return;
    }

    auto event{std::make_shared<firebolt::rialto::MetricsSampleRequestEvent>()};
    const auto kSampleId{m_nextSampleId.fetch_add(1)};
    event->set_sample_id(kSampleId);
    event->set_reason(reason);

    RIALTO_SERVER_LOG_MIL("Requesting metrics sample=%" PRIu64 ", reason=%s", kSampleId, sampleReasonToString(reason));

    if (!ipcClient->sendEvent(event))
    {
        RIALTO_SERVER_LOG_WARN("Failed to request client metrics sample=%" PRIu64 ", reason=%s", kSampleId,
                               sampleReasonToString(reason));
    }
}

PrivateMetricsModuleService::ProcessMetricsSample PrivateMetricsModuleService::getProcessMetricsSample() const
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;
    using std::chrono::system_clock;

    struct tms processTimes{};
    const clock_t kCurrentTicks{times(&processTimes)};
    const long kTicksPerSecond{sysconf(_SC_CLK_TCK)};
    std::uint64_t processCpuTimeMs{0};
    if ((static_cast<clock_t>(-1) != kCurrentTicks) && (kTicksPerSecond > 0))
    {
        const auto kProcessTicks{processTimes.tms_utime + processTimes.tms_stime};
        processCpuTimeMs = static_cast<std::uint64_t>((static_cast<double>(kProcessTicks) * 1000.0) /
                                                      static_cast<double>(kTicksPerSecond));
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("Failed to sample server process CPU usage");
    }

    std::uint64_t processMemoryKb{0};
    {
        std::ifstream status{"/proc/self/status"};
        std::string line;
        while (std::getline(status, line))
        {
            if (line.rfind("VmRSS:", 0) == 0)
            {
                if (std::sscanf(line.c_str(), "VmRSS: %" SCNu64, &processMemoryKb) != 1)
                {
                    RIALTO_SERVER_LOG_WARN("Failed to parse server process memory usage");
                }
                break;
            }
        }
    }

    std::uint64_t cgroupMemoryUsageKb{0};
    std::uint64_t cgroupMemoryLimitKb{0};
    {
        auto readFileValue = [](const std::string &path) -> std::uint64_t
        {
            std::ifstream file{path};
            if (!file.is_open())
            {
                return 0;
            }
            std::string content;
            if (!std::getline(file, content) || content.empty() || content == "max")
            {
                return 0;
            }
            std::uint64_t value{0};
            if (std::sscanf(content.c_str(), "%" SCNu64, &value) == 1)
            {
                return value;
            }
            return 0;
        };

        // Resolve the process's cgroup path from /proc/self/cgroup
        // cgroup v2 format: "0::<relative-path>"
        auto getCgroupBasePath = [&readFileValue]() -> std::string
        {
            std::ifstream cgroupFile{"/proc/self/cgroup"};
            if (!cgroupFile.is_open())
            {
                return {};
            }
            std::string line;
            while (std::getline(cgroupFile, line))
            {
                // cgroup v2 line starts with "0::"
                if (line.rfind("0::", 0) == 0)
                {
                    std::string relativePath{line.substr(3)};
                    if (!relativePath.empty() && relativePath != "/")
                    {
                        return "/sys/fs/cgroup" + relativePath;
                    }
                    return "/sys/fs/cgroup";
                }
            }
            return {};
        };

        std::uint64_t usageBytes{0};
        std::uint64_t limitBytes{0};

        // cgroup v2: read from process's own cgroup path
        std::string cgroupBase{getCgroupBasePath()};
        if (!cgroupBase.empty())
        {
            usageBytes = readFileValue(cgroupBase + "/memory.current");
            limitBytes = readFileValue(cgroupBase + "/memory.max");
        }

        if (usageBytes == 0)
        {
            // cgroup v1 fallback
            usageBytes = readFileValue("/sys/fs/cgroup/memory/memory.usage_in_bytes");
            limitBytes = readFileValue("/sys/fs/cgroup/memory/memory.limit_in_bytes");
        }

        cgroupMemoryUsageKb = usageBytes / 1024;
        cgroupMemoryLimitKb = limitBytes / 1024;
    }

    return ProcessMetricsSample{
        static_cast<std::uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()),
        static_cast<std::uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()),
        processCpuTimeMs,
        processMemoryKb,
        cgroupMemoryUsageKb,
        cgroupMemoryLimitKb};
}

void PrivateMetricsModuleService::logMetrics(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient,
                                             const ::firebolt::rialto::ClientProcessMetrics &clientMetrics,
                                             const ProcessMetricsSample &serverMetrics)
{
    std::optional<MetricsSamplePair> previousSample;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        const auto kClientIter{m_clients.find(ipcClient)};
        if ((m_clients.end() != kClientIter) && kClientIter->second.latestMetrics.has_value())
        {
            previousSample = kClientIter->second.latestMetrics;
        }
    }

    if (!previousSample.has_value() || !previousSample->clientMetrics.has_process_cpu_time_ms())
    {
        RIALTO_SERVER_LOG_MIL("Metrics baseline: sample=%" PRIu64 ", reason=%s, app='%s', client_pid=%u, "
                              "client_cpu_ms=%" PRIu64 ", server_cpu_ms=%" PRIu64 ", "
                              "client_mem_kb=%" PRIu64 ", server_mem_kb=%" PRIu64 ", "
                              "cgroup_mem_kb=%" PRIu64 "/%" PRIu64,
                              clientMetrics.sample_id(), sampleReasonToString(clientMetrics.reason()),
                              clientMetrics.app_name().c_str(), clientMetrics.process_id(),
                              clientMetrics.process_cpu_time_ms(), serverMetrics.processCpuTimeMs,
                              clientMetrics.process_memory_kb(), serverMetrics.processMemoryKb,
                              serverMetrics.cgroupMemoryUsageKb, serverMetrics.cgroupMemoryLimitKb);
        return;
    }

    const auto &previousClientMetrics{previousSample->clientMetrics};
    const auto &previousServerMetrics{previousSample->serverMetrics};
    const double kClientCpuPercentage{calculateCpuPercentage(
        clientMetrics.process_cpu_time_ms(), previousClientMetrics.process_cpu_time_ms(),
        clientMetrics.monotonic_time_ms(), previousClientMetrics.monotonic_time_ms())};
    const double kServerCpuPercentage{calculateCpuPercentage(serverMetrics.processCpuTimeMs,
                                                            previousServerMetrics.processCpuTimeMs,
                                                            serverMetrics.monotonicTimeMs,
                                                            previousServerMetrics.monotonicTimeMs)};
    const double kCombinedCpuPercentage{calculateCpuPercentage(
        clientMetrics.process_cpu_time_ms() + serverMetrics.processCpuTimeMs,
        previousClientMetrics.process_cpu_time_ms() + previousServerMetrics.processCpuTimeMs,
        serverMetrics.monotonicTimeMs, previousServerMetrics.monotonicTimeMs)};

    // Report via pluggable reporter
    if (m_reporter)
    {
        PeriodicMetricsReport periodicReport;
        periodicReport.sampleId = clientMetrics.sample_id();
        periodicReport.reason = sampleReasonToString(clientMetrics.reason());
        periodicReport.appName = clientMetrics.app_name();
        periodicReport.clientPid = clientMetrics.process_id();
        periodicReport.clientCpuPercent = kClientCpuPercentage;
        periodicReport.serverCpuPercent = kServerCpuPercentage;
        periodicReport.combinedCpuPercent = kCombinedCpuPercentage;
        periodicReport.clientCpuTimeMs = clientMetrics.process_cpu_time_ms();
        periodicReport.serverCpuTimeMs = serverMetrics.processCpuTimeMs;
        periodicReport.clientMemoryKb = clientMetrics.process_memory_kb();
        periodicReport.serverMemoryKb = serverMetrics.processMemoryKb;
        periodicReport.cgroupMemoryUsageKb = serverMetrics.cgroupMemoryUsageKb;
        periodicReport.cgroupMemoryLimitKb = serverMetrics.cgroupMemoryLimitKb;
        m_reporter->reportPeriodicSample(periodicReport);
    }

    // Only feed PERIODIC samples into aggregators — STATE_TRANSITION samples have
    // unreliable CPU percentages due to tiny time deltas between rapid samples.
    if (clientMetrics.reason() == firebolt::rialto::METRICS_SAMPLE_REASON_PERIODIC)
    {
        MetricsSample sample;
        sample.clientCpuPercent = kClientCpuPercentage;
        sample.serverCpuPercent = kServerCpuPercentage;
        sample.combinedCpuPercent = kCombinedCpuPercentage;
        sample.clientMemoryKb = clientMetrics.process_memory_kb();
        sample.serverMemoryKb = serverMetrics.processMemoryKb;
        sample.cgroupMemoryUsageKb = serverMetrics.cgroupMemoryUsageKb;
        sample.cgroupMemoryLimitKb = serverMetrics.cgroupMemoryLimitKb;

        {
            std::lock_guard<std::mutex> lock{m_mutex};

            // Feed into per-session aggregators
            for (auto &[sessionId, sessionState] : m_sessionStates)
            {
                sessionState.aggregator.addSample(sample);
            }

            // Feed into global aggregator
            if (m_currentApplicationState == ApplicationState::RUNNING)
            {
                m_globalAggregator.addSample(sample);
            }
        }
    }

    // Check thresholds (only for PERIODIC samples with reliable CPU data)
    if (clientMetrics.reason() == firebolt::rialto::METRICS_SAMPLE_REASON_PERIODIC)
    {
        m_thresholdChecker.checkSample(kClientCpuPercentage, kServerCpuPercentage, kCombinedCpuPercentage,
                                       clientMetrics.process_memory_kb(), serverMetrics.processMemoryKb,
                                       serverMetrics.cgroupMemoryUsageKb, serverMetrics.cgroupMemoryLimitKb);
    }
}

const char *PrivateMetricsModuleService::sampleReasonToString(::firebolt::rialto::MetricsSampleReason reason) const
{
    switch (reason)
    {
    case firebolt::rialto::METRICS_SAMPLE_REASON_CONNECTED:
        return "CONNECTED";
    case firebolt::rialto::METRICS_SAMPLE_REASON_PERIODIC:
        return "PERIODIC";
    case firebolt::rialto::METRICS_SAMPLE_REASON_STATE_TRANSITION:
        return "STATE_TRANSITION";
    case firebolt::rialto::METRICS_SAMPLE_REASON_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

const char *PrivateMetricsModuleService::playbackStateToString(PlaybackState state)
{
    switch (state)
    {
    case PlaybackState::IDLE:
        return "IDLE";
    case PlaybackState::PLAYING:
        return "PLAYING";
    case PlaybackState::PAUSED:
        return "PAUSED";
    case PlaybackState::SEEKING:
        return "SEEKING";
    case PlaybackState::SEEK_DONE:
        return "SEEK_DONE";
    case PlaybackState::STOPPED:
        return "STOPPED";
    case PlaybackState::END_OF_STREAM:
        return "END_OF_STREAM";
    case PlaybackState::FAILURE:
        return "FAILURE";
    case PlaybackState::UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

const char *PrivateMetricsModuleService::applicationStateToString(ApplicationState state)
{
    switch (state)
    {
    case ApplicationState::RUNNING:
        return "RUNNING";
    case ApplicationState::INACTIVE:
        return "INACTIVE";
    case ApplicationState::UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

void PrivateMetricsModuleService::notifyPlaybackStateChanged(int sessionId, PlaybackState oldState,
                                                             PlaybackState newState)
{
    RIALTO_SERVER_LOG_MIL("Metrics: PlaybackState changed session=%d, %s -> %s", sessionId,
                          playbackStateToString(oldState), playbackStateToString(newState));

    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;
    const auto kNowMs{
        static_cast<std::uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count())};

    std::lock_guard<std::mutex> lock{m_mutex};
    auto sessionIter{m_sessionStates.find(sessionId)};
    if (m_sessionStates.end() == sessionIter)
    {
        // First state notification for this session — create entry
        SessionMetricsState sessionState;
        sessionState.currentPlaybackState = newState;
        sessionState.aggregator.begin(playbackStateToString(newState), kNowMs);
        m_sessionStates.emplace(sessionId, std::move(sessionState));
        return;
    }

    auto &sessionState{sessionIter->second};

    // Finalize old state and emit report
    if (sessionState.aggregator.hasData())
    {
        auto report{sessionState.aggregator.finalize(kNowMs)};
        logStateReport(report, "session=" + std::to_string(sessionId));
    }

    if (newState == PlaybackState::STOPPED || newState == PlaybackState::END_OF_STREAM ||
        newState == PlaybackState::FAILURE)
    {
        // Terminal state — remove session tracking
        m_sessionStates.erase(sessionIter);
    }
    else
    {
        // Begin accumulating for new state
        sessionState.currentPlaybackState = newState;
        sessionState.aggregator.begin(playbackStateToString(newState), kNowMs);
    }

    // Request immediate sample for clean boundary
    for (const auto &client : m_clients)
    {
        if (client.second.isReady)
        {
            requestMetricsSample(client.first, firebolt::rialto::METRICS_SAMPLE_REASON_STATE_TRANSITION);
        }
    }
}

void PrivateMetricsModuleService::notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState)
{
    RIALTO_SERVER_LOG_MIL("Metrics: ApplicationState changed %s -> %s", applicationStateToString(oldState),
                          applicationStateToString(newState));

    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;
    const auto kNowMs{
        static_cast<std::uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count())};

    std::lock_guard<std::mutex> lock{m_mutex};
    m_currentApplicationState = newState;

    if (oldState == ApplicationState::RUNNING && newState != ApplicationState::RUNNING)
    {
        // Leaving RUNNING — finalize global aggregator
        if (m_globalAggregator.hasData())
        {
            auto report{m_globalAggregator.finalize(kNowMs)};
            logStateReport(report, "global");
        }
        m_globalAggregator.reset();
    }

    if (newState == ApplicationState::RUNNING && oldState != ApplicationState::RUNNING)
    {
        // Entering RUNNING — start fresh global accumulation
        m_globalAggregator.begin(applicationStateToString(newState), kNowMs);
    }

    // Request immediate sample for clean boundary
    for (const auto &client : m_clients)
    {
        if (client.second.isReady)
        {
            requestMetricsSample(client.first, firebolt::rialto::METRICS_SAMPLE_REASON_STATE_TRANSITION);
        }
    }
}

void PrivateMetricsModuleService::logStateReport(const StateMetricsReport &report, const std::string &context)
{
    if (m_reporter)
    {
        StateTransitionReport transitionReport;
        transitionReport.context = context;
        transitionReport.metrics = report;
        m_reporter->reportStateTransition(transitionReport);
    }
}

double PrivateMetricsModuleService::calculateCpuPercentage(std::uint64_t currentCpuTimeMs,
                                                           std::uint64_t previousCpuTimeMs,
                                                           std::uint64_t currentMonotonicTimeMs,
                                                           std::uint64_t previousMonotonicTimeMs) const
{
    constexpr std::uint64_t kMinElapsedMs{100};
    if ((currentCpuTimeMs < previousCpuTimeMs) || (currentMonotonicTimeMs <= previousMonotonicTimeMs))
    {
        return 0.0;
    }

    const auto kElapsedMs{currentMonotonicTimeMs - previousMonotonicTimeMs};
    if (kElapsedMs < kMinElapsedMs)
    {
        // Time delta too small for meaningful CPU percentage
        return 0.0;
    }

    return (static_cast<double>(currentCpuTimeMs - previousCpuTimeMs) / static_cast<double>(kElapsedMs)) * 100.0;
}
} // namespace firebolt::rialto::server::ipc
