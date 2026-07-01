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

#include "IMetricsCollectorClient.h"
#include "IPrivateMetricsModuleService.h"
#include "IPrivateMetricsService.h"
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>

namespace firebolt::rialto::server::ipc
{
class PrivateMetricsModuleServiceFactory : public IPrivateMetricsModuleServiceFactory
{
public:
    PrivateMetricsModuleServiceFactory() = default;
    ~PrivateMetricsModuleServiceFactory() override = default;

    std::shared_ptr<IPrivateMetricsModuleService>
    create(service::IPrivateMetricsService &metricsService) const override;
};

class PrivateMetricsModuleService : public IPrivateMetricsModuleService,
                                    public firebolt::rialto::server::IMetricsCollectorClient
{
public:
    explicit PrivateMetricsModuleService(service::IPrivateMetricsService &metricsService);
    ~PrivateMetricsModuleService() override;

    // IPrivateMetricsModuleService
    void clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) override;
    void clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) override;

    void notifyPlaybackStateChanged(int sessionId, PlaybackState oldState, PlaybackState newState) override;
    void notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState) override;

    // PrivateMetricsModule RPC handlers
    void reportClientMetrics(::google::protobuf::RpcController *controller,
                             const ::firebolt::rialto::ReportClientMetricsRequest *request,
                             ::firebolt::rialto::ReportClientMetricsResponse *response,
                             ::google::protobuf::Closure *done) override;
    void notifyClientReady(::google::protobuf::RpcController *controller,
                           const ::firebolt::rialto::NotifyClientReadyRequest *request,
                           ::firebolt::rialto::NotifyClientReadyResponse *response,
                           ::google::protobuf::Closure *done) override;

    // IMetricsCollectorClient
    void requestMetricsSample(int clientId, std::uint64_t sampleId,
                              firebolt::rialto::server::MetricsSampleReason reason) override;

private:
    service::IPrivateMetricsService &m_metricsService;
    std::atomic<int> m_nextClientId{1};
    std::mutex m_mutex;

    // Map IPC client pointer → client ID
    std::map<std::shared_ptr<::firebolt::rialto::ipc::IClient>, int> m_clientIds;
    // Map client ID → IPC client pointer (for sending events back)
    std::map<int, std::shared_ptr<::firebolt::rialto::ipc::IClient>> m_ipcClients;
};
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_PRIVATE_METRICS_MODULE_SERVICE_H_
