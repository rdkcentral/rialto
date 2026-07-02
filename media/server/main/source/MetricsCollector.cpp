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

#include "MetricsCollector.h"
#include "LogMetricsReporter.h"
#include "RialtoServerLogging.h"
#include <chrono>
#include <cinttypes>
#include <fstream>
#include <string>
#include <sys/times.h>
#include <unistd.h>

namespace
{
constexpr std::chrono::seconds kMetricsInterval{15};
constexpr std::uint64_t kMinElapsedMs{100};
} // namespace

namespace firebolt::rialto::server
{
std::shared_ptr<IMetricsCollectorFactory> IMetricsCollectorFactory::createFactory()
{
    std::shared_ptr<IMetricsCollectorFactory> factory;
    try
    {
        factory = std::make_shared<MetricsCollectorFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create MetricsCollectorFactory, reason: %s", e.what());
    }
    return factory;
}

std::unique_ptr<IMetricsCollector>
MetricsCollectorFactory::create(int clientId, const std::shared_ptr<IMetricsCollectorClient> &client)
{
    std::unique_ptr<IMetricsCollector> collector;
    try
    {
        auto timerFactory = firebolt::rialto::common::ITimerFactory::getFactory();
        collector = std::make_unique<MetricsCollector>(clientId, client, timerFactory);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create MetricsCollector for client %d, reason: %s", clientId, e.what());
    }
    return collector;
}

MetricsCollector::MetricsCollector(int clientId, const std::shared_ptr<IMetricsCollectorClient> &client,
                                   const std::shared_ptr<firebolt::rialto::common::ITimerFactory> &timerFactory)
    : m_clientId{clientId}, m_client{client}, m_reporter{std::make_unique<LogMetricsReporter>()},
      m_thresholdChecker{MetricsThresholdConfig{}, m_reporter.get()}
{
    m_timer = timerFactory->createTimer(kMetricsInterval, [this]() { onTimerFired(); },
                                        firebolt::rialto::common::TimerType::PERIODIC);

    // Request initial baseline sample
    m_client->requestMetricsSample(m_clientId, m_nextSampleId++, MetricsSampleReason::CONNECTED);
}

MetricsCollector::~MetricsCollector()
{
    if (m_timer)
    {
        m_timer->cancel();
    }
}

void MetricsCollector::onTimerFired()
{
    RIALTO_SERVER_LOG_MIL("Metrics: periodic timer fired for client %d, requesting sample=%" PRIu64, m_clientId,
                          m_nextSampleId);
    m_client->requestMetricsSample(m_clientId, m_nextSampleId++, MetricsSampleReason::PERIODIC);
}

void MetricsCollector::processMetrics(const ClientMetricsData &metrics)
{
    const auto kServerMetrics{getServerMetrics()};

    std::optional<PreviousSample> previous;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        previous = m_previousSample;
    }

    if (!previous.has_value())
    {
        // Baseline sample — store and return
        RIALTO_SERVER_LOG_MIL("Metrics baseline: sample=%" PRIu64 ", reason=%s, app='%s', client_pid=%u, "
                              "client_cpu_ms=%" PRIu64 ", server_cpu_ms=%" PRIu64 ", "
                              "client_mem_kb=%" PRIu64 ", server_mem_kb=%" PRIu64 ", "
                              "cgroup_mem_kb=%" PRIu64 "/%" PRIu64,
                              metrics.sampleId, sampleReasonToString(metrics.reason), metrics.appName.c_str(),
                              metrics.processId, metrics.processCpuTimeMs, kServerMetrics.processCpuTimeMs,
                              metrics.processMemoryKb, kServerMetrics.processMemoryKb,
                              kServerMetrics.cgroupMemoryUsageKb, kServerMetrics.cgroupMemoryLimitKb);

        std::lock_guard<std::mutex> lock{m_mutex};
        m_previousSample = PreviousSample{metrics.monotonicTimeMs, metrics.processCpuTimeMs, metrics.processMemoryKb,
                                          kServerMetrics};
        return;
    }

    const auto &prev{previous.value()};
    const double kClientCpuPercentage{
        calculateCpuPercentage(metrics.processCpuTimeMs, prev.clientCpuTimeMs, metrics.monotonicTimeMs,
                               prev.clientMonotonicTimeMs)};
    const double kServerCpuPercentage{calculateCpuPercentage(kServerMetrics.processCpuTimeMs,
                                                             prev.serverMetrics.processCpuTimeMs,
                                                             kServerMetrics.monotonicTimeMs,
                                                             prev.serverMetrics.monotonicTimeMs)};
    const double kCombinedCpuPercentage{
        calculateCpuPercentage(metrics.processCpuTimeMs + kServerMetrics.processCpuTimeMs,
                               prev.clientCpuTimeMs + prev.serverMetrics.processCpuTimeMs, kServerMetrics.monotonicTimeMs,
                               prev.serverMetrics.monotonicTimeMs)};

    // Report via pluggable reporter
    if (m_reporter)
    {
        PeriodicMetricsReport periodicReport;
        periodicReport.sampleId = metrics.sampleId;
        periodicReport.reason = sampleReasonToString(metrics.reason);
        periodicReport.appName = metrics.appName;
        periodicReport.clientPid = metrics.processId;
        periodicReport.clientCpuPercent = kClientCpuPercentage;
        periodicReport.serverCpuPercent = kServerCpuPercentage;
        periodicReport.combinedCpuPercent = kCombinedCpuPercentage;
        periodicReport.clientCpuTimeMs = metrics.processCpuTimeMs;
        periodicReport.serverCpuTimeMs = kServerMetrics.processCpuTimeMs;
        periodicReport.clientMemoryKb = metrics.processMemoryKb;
        periodicReport.serverMemoryKb = kServerMetrics.processMemoryKb;
        periodicReport.cgroupMemoryUsageKb = kServerMetrics.cgroupMemoryUsageKb;
        periodicReport.cgroupMemoryLimitKb = kServerMetrics.cgroupMemoryLimitKb;
        periodicReport.shmMemoryKb = kServerMetrics.shmMemoryKb;
        m_reporter->reportPeriodicSample(periodicReport);
    }

    // Only feed PERIODIC samples into aggregators — STATE_TRANSITION samples have
    // unreliable CPU percentages due to tiny time deltas between rapid samples.
    if (metrics.reason == MetricsSampleReason::PERIODIC)
    {
        MetricsSample sample;
        sample.clientCpuPercent = kClientCpuPercentage;
        sample.serverCpuPercent = kServerCpuPercentage;
        sample.combinedCpuPercent = kCombinedCpuPercentage;
        sample.clientMemoryKb = metrics.processMemoryKb;
        sample.serverMemoryKb = kServerMetrics.processMemoryKb;
        sample.cgroupMemoryUsageKb = kServerMetrics.cgroupMemoryUsageKb;
        sample.cgroupMemoryLimitKb = kServerMetrics.cgroupMemoryLimitKb;

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

        // Check thresholds
        m_thresholdChecker.checkSample(kClientCpuPercentage, kServerCpuPercentage, kCombinedCpuPercentage,
                                       metrics.processMemoryKb, kServerMetrics.processMemoryKb,
                                       kServerMetrics.cgroupMemoryUsageKb, kServerMetrics.cgroupMemoryLimitKb);
    }

    // Update previous sample
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_previousSample =
            PreviousSample{metrics.monotonicTimeMs, metrics.processCpuTimeMs, metrics.processMemoryKb, kServerMetrics};
    }
}

void MetricsCollector::notifyPlaybackStateChanged(int sessionId, PlaybackState oldState, PlaybackState newState)
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
    if (sessionState.aggregator.hasData() && m_reporter)
    {
        auto report{sessionState.aggregator.finalize(kNowMs)};
        StateTransitionReport transitionReport;
        transitionReport.context = "session=" + std::to_string(sessionId);
        transitionReport.metrics = report;
        m_reporter->reportStateTransition(transitionReport);
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
    m_client->requestMetricsSample(m_clientId, m_nextSampleId++, MetricsSampleReason::STATE_TRANSITION);
}

void MetricsCollector::notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState)
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
        if (m_globalAggregator.hasData() && m_reporter)
        {
            auto report{m_globalAggregator.finalize(kNowMs)};
            StateTransitionReport transitionReport;
            transitionReport.context = "global";
            transitionReport.metrics = report;
            m_reporter->reportStateTransition(transitionReport);
        }
        m_globalAggregator.reset();
    }

    if (newState == ApplicationState::RUNNING && oldState != ApplicationState::RUNNING)
    {
        // Entering RUNNING — start fresh global accumulation
        m_globalAggregator.begin(applicationStateToString(newState), kNowMs);
    }

    // Request immediate sample for clean boundary
    m_client->requestMetricsSample(m_clientId, m_nextSampleId++, MetricsSampleReason::STATE_TRANSITION);
}

MetricsCollector::ProcessMetricsSample MetricsCollector::getServerMetrics() const
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;
    using std::chrono::system_clock;

    struct tms processTimes
    {
    };
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
        auto getCgroupBasePath = []() -> std::string
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

    std::uint64_t shmMemoryKb{0};
    {
        std::ifstream smaps{"/proc/self/smaps_rollup"};
        std::string sline;
        while (std::getline(smaps, sline))
        {
            if (sline.rfind("Pss_Shmem:", 0) == 0)
            {
                std::sscanf(sline.c_str(), "Pss_Shmem: %" SCNu64, &shmMemoryKb);
                break;
            }
        }
    }

    return ProcessMetricsSample{
        static_cast<std::uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()),
        static_cast<std::uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()),
        processCpuTimeMs, processMemoryKb, cgroupMemoryUsageKb, cgroupMemoryLimitKb, shmMemoryKb};
}

double MetricsCollector::calculateCpuPercentage(std::uint64_t currentCpuTimeMs, std::uint64_t previousCpuTimeMs,
                                                std::uint64_t currentMonotonicTimeMs,
                                                std::uint64_t previousMonotonicTimeMs) const
{
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

const char *MetricsCollector::sampleReasonToString(MetricsSampleReason reason)
{
    switch (reason)
    {
    case MetricsSampleReason::CONNECTED:
        return "CONNECTED";
    case MetricsSampleReason::PERIODIC:
        return "PERIODIC";
    case MetricsSampleReason::STATE_TRANSITION:
        return "STATE_TRANSITION";
    case MetricsSampleReason::UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

const char *MetricsCollector::playbackStateToString(PlaybackState state)
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

const char *MetricsCollector::applicationStateToString(ApplicationState state)
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
} // namespace firebolt::rialto::server
