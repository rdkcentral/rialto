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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_PRIVATE_METRICS_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_PRIVATE_METRICS_SERVICE_H_

#include "IMetricsCollector.h"
#include "IPrivateMetricsService.h"
#include <map>
#include <memory>
#include <mutex>

namespace firebolt::rialto::server::service
{
class PrivateMetricsService : public IPrivateMetricsService
{
public:
    explicit PrivateMetricsService(std::shared_ptr<firebolt::rialto::server::IMetricsCollectorFactory> collectorFactory);
    ~PrivateMetricsService() override;

    void clientReady(int clientId, const std::shared_ptr<firebolt::rialto::server::IMetricsCollectorClient> &client) override;
    void clientDisconnected(int clientId) override;
    void reportMetrics(int clientId, const firebolt::rialto::server::ClientMetricsData &metrics) override;
    void notifyPlaybackStateChanged(int sessionId, PlaybackState oldState, PlaybackState newState) override;
    void notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState) override;

private:
    std::shared_ptr<firebolt::rialto::server::IMetricsCollectorFactory> m_collectorFactory;
    std::mutex m_mutex;
    std::map<int, std::unique_ptr<firebolt::rialto::server::IMetricsCollector>> m_collectors;
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_PRIVATE_METRICS_SERVICE_H_
