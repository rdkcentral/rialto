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
#include <cinttypes>

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

std::shared_ptr<IPrivateMetricsModuleService>
PrivateMetricsModuleServiceFactory::create(service::IPrivateMetricsService &metricsService) const
{
    std::shared_ptr<IPrivateMetricsModuleService> privateMetricsModule;

    try
    {
        privateMetricsModule = std::make_shared<PrivateMetricsModuleService>(metricsService);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the rialto private metrics module service, reason: %s", e.what());
    }

    return privateMetricsModule;
}

PrivateMetricsModuleService::PrivateMetricsModuleService(service::IPrivateMetricsService &metricsService)
    : m_metricsService{metricsService}
{
}

PrivateMetricsModuleService::~PrivateMetricsModuleService() = default;

void PrivateMetricsModuleService::clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client connected to private metrics module");
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        // Don't assign a clientId yet — wait for notifyClientReady
    }
    ipcClient->exportService(shared_from_this());
}

void PrivateMetricsModuleService::clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client disconnected from private metrics module");
    int clientId{0};
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        auto iter = m_clientIds.find(ipcClient);
        if (iter != m_clientIds.end())
        {
            clientId = iter->second;
            m_clientIds.erase(iter);
            m_ipcClients.erase(clientId);
        }
    }
    if (clientId != 0)
    {
        m_metricsService.clientDisconnected(clientId);
    }
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
    const int kClientId{m_nextClientId.fetch_add(1)};
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_clientIds[ipcClient] = kClientId;
        m_ipcClients[kClientId] = ipcClient;
    }

    RIALTO_SERVER_LOG_MIL("Client ready for private metrics samples, assigned clientId=%d", kClientId);
    done->Run();

    // Create a shared_ptr to this as IMetricsCollectorClient, aliasing with shared_from_this()
    // so the IPC layer stays alive as long as the MetricsCollector holds a reference.
    auto self = shared_from_this();
    std::shared_ptr<firebolt::rialto::server::IMetricsCollectorClient> clientInterface(
        self, static_cast<firebolt::rialto::server::IMetricsCollectorClient *>(this));
    m_metricsService.clientReady(kClientId, clientInterface);
}

void PrivateMetricsModuleService::reportClientMetrics(
    ::google::protobuf::RpcController *controller, const ::firebolt::rialto::ReportClientMetricsRequest *request,
    ::firebolt::rialto::ReportClientMetricsResponse *response, ::google::protobuf::Closure *done)
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

    auto ipcClient{ipcController->getClient()};
    int clientId{0};
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        auto iter = m_clientIds.find(ipcClient);
        if (iter != m_clientIds.end())
        {
            clientId = iter->second;
        }
    }

    if (clientId == 0)
    {
        RIALTO_SERVER_LOG_WARN("reportClientMetrics from unknown client");
        done->Run();
        return;
    }

    const auto &protoMetrics{request->metrics()};
    firebolt::rialto::server::ClientMetricsData metrics;
    metrics.sampleId = protoMetrics.sample_id();
    metrics.appName = protoMetrics.app_name();
    metrics.processId = protoMetrics.process_id();
    metrics.monotonicTimeMs = protoMetrics.monotonic_time_ms();
    metrics.epochTimeMs = protoMetrics.epoch_time_ms();
    metrics.processCpuTimeMs = protoMetrics.process_cpu_time_ms();
    metrics.processMemoryKb = protoMetrics.process_memory_kb();

    // Convert proto reason to our enum
    switch (protoMetrics.reason())
    {
    case firebolt::rialto::METRICS_SAMPLE_REASON_CONNECTED:
        metrics.reason = firebolt::rialto::server::MetricsSampleReason::CONNECTED;
        break;
    case firebolt::rialto::METRICS_SAMPLE_REASON_PERIODIC:
        metrics.reason = firebolt::rialto::server::MetricsSampleReason::PERIODIC;
        break;
    case firebolt::rialto::METRICS_SAMPLE_REASON_STATE_TRANSITION:
        metrics.reason = firebolt::rialto::server::MetricsSampleReason::STATE_TRANSITION;
        break;
    default:
        metrics.reason = firebolt::rialto::server::MetricsSampleReason::UNKNOWN;
        break;
    }

    done->Run();

    m_metricsService.reportMetrics(clientId, metrics);
}

void PrivateMetricsModuleService::notifyPlaybackStateChanged(int sessionId, PlaybackState oldState,
                                                             PlaybackState newState)
{
    m_metricsService.notifyPlaybackStateChanged(sessionId, oldState, newState);
}

void PrivateMetricsModuleService::notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState)
{
    m_metricsService.notifyApplicationStateChanged(oldState, newState);
}

void PrivateMetricsModuleService::requestMetricsSample(int clientId, std::uint64_t sampleId,
                                                       firebolt::rialto::server::MetricsSampleReason reason)
{
    std::shared_ptr<::firebolt::rialto::ipc::IClient> ipcClient;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        auto iter = m_ipcClients.find(clientId);
        if (iter == m_ipcClients.end())
        {
            return;
        }
        ipcClient = iter->second;
    }

    if (!ipcClient || !ipcClient->isConnected())
    {
        return;
    }

    auto event{std::make_shared<firebolt::rialto::MetricsSampleRequestEvent>()};
    event->set_sample_id(sampleId);

    // Convert our enum to proto enum
    switch (reason)
    {
    case firebolt::rialto::server::MetricsSampleReason::CONNECTED:
        event->set_reason(firebolt::rialto::METRICS_SAMPLE_REASON_CONNECTED);
        break;
    case firebolt::rialto::server::MetricsSampleReason::PERIODIC:
        event->set_reason(firebolt::rialto::METRICS_SAMPLE_REASON_PERIODIC);
        break;
    case firebolt::rialto::server::MetricsSampleReason::STATE_TRANSITION:
        event->set_reason(firebolt::rialto::METRICS_SAMPLE_REASON_STATE_TRANSITION);
        break;
    default:
        event->set_reason(firebolt::rialto::METRICS_SAMPLE_REASON_UNKNOWN);
        break;
    }

    RIALTO_SERVER_LOG_DEBUG("Requesting metrics sample=%" PRIu64 " from client %d", sampleId, clientId);

    if (!ipcClient->sendEvent(event))
    {
        RIALTO_SERVER_LOG_WARN("Failed to request client metrics sample=%" PRIu64 " from client %d", sampleId,
                               clientId);
    }
}
} // namespace firebolt::rialto::server::ipc
