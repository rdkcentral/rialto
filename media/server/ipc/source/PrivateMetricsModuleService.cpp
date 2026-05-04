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
#include "RialtoServerLogging.h"
#include <IIpcController.h>
#include <chrono>
#include <cinttypes>
#include <optional>
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

PrivateMetricsModuleService::PrivateMetricsModuleService() : m_isRunning{true}, m_nextSampleId{1}
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

    struct tms processTimes
    {};
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

    return ProcessMetricsSample{
        static_cast<std::uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()),
        static_cast<std::uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()),
        processCpuTimeMs};
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
                              "client_cpu_ms=%" PRIu64 ", server_cpu_ms=%" PRIu64,
                              clientMetrics.sample_id(), sampleReasonToString(clientMetrics.reason()),
                              clientMetrics.app_name().c_str(), clientMetrics.process_id(),
                              clientMetrics.process_cpu_time_ms(), serverMetrics.processCpuTimeMs);
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

    RIALTO_SERVER_LOG_MIL("Metrics sample=%" PRIu64 ", reason=%s, app='%s', client_pid=%u, client_cpu=%.2f%%, "
                          "server_cpu=%.2f%%, combined_cpu=%.2f%%, client_cpu_ms=%" PRIu64 ", "
                          "server_cpu_ms=%" PRIu64,
                          clientMetrics.sample_id(), sampleReasonToString(clientMetrics.reason()),
                          clientMetrics.app_name().c_str(), clientMetrics.process_id(), kClientCpuPercentage,
                          kServerCpuPercentage, kCombinedCpuPercentage, clientMetrics.process_cpu_time_ms(),
                          serverMetrics.processCpuTimeMs);
}

const char *PrivateMetricsModuleService::sampleReasonToString(::firebolt::rialto::MetricsSampleReason reason) const
{
    switch (reason)
    {
    case firebolt::rialto::METRICS_SAMPLE_REASON_CONNECTED:
        return "CONNECTED";
    case firebolt::rialto::METRICS_SAMPLE_REASON_PERIODIC:
        return "PERIODIC";
    case firebolt::rialto::METRICS_SAMPLE_REASON_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

double PrivateMetricsModuleService::calculateCpuPercentage(std::uint64_t currentCpuTimeMs,
                                                           std::uint64_t previousCpuTimeMs,
                                                           std::uint64_t currentMonotonicTimeMs,
                                                           std::uint64_t previousMonotonicTimeMs) const
{
    if ((currentCpuTimeMs < previousCpuTimeMs) || (currentMonotonicTimeMs <= previousMonotonicTimeMs))
    {
        return 0.0;
    }

    return (static_cast<double>(currentCpuTimeMs - previousCpuTimeMs) /
            static_cast<double>(currentMonotonicTimeMs - previousMonotonicTimeMs)) *
           100.0;
}
} // namespace firebolt::rialto::server::ipc
