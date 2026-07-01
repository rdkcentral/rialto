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

#include "PrivateMetricsService.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server::service
{
PrivateMetricsService::PrivateMetricsService(
    std::shared_ptr<firebolt::rialto::server::IMetricsCollectorFactory> collectorFactory)
    : m_collectorFactory{std::move(collectorFactory)}
{
}

PrivateMetricsService::~PrivateMetricsService()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    m_collectors.clear();
}

void PrivateMetricsService::clientReady(int clientId,
                                        const std::shared_ptr<firebolt::rialto::server::IMetricsCollectorClient> &client)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto collector = m_collectorFactory->create(clientId, client);
    if (collector)
    {
        m_collectors.emplace(clientId, std::move(collector));
        RIALTO_SERVER_LOG_INFO("MetricsCollector created for client %d", clientId);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create MetricsCollector for client %d", clientId);
    }
}

void PrivateMetricsService::clientDisconnected(int clientId)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto iter = m_collectors.find(clientId);
    if (iter != m_collectors.end())
    {
        m_collectors.erase(iter);
        RIALTO_SERVER_LOG_INFO("MetricsCollector destroyed for client %d", clientId);
    }
}

void PrivateMetricsService::reportMetrics(int clientId, const firebolt::rialto::server::ClientMetricsData &metrics)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto iter = m_collectors.find(clientId);
    if (iter != m_collectors.end())
    {
        iter->second->processMetrics(metrics);
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("reportMetrics for unknown client %d", clientId);
    }
}

void PrivateMetricsService::notifyPlaybackStateChanged(int sessionId, PlaybackState oldState, PlaybackState newState)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    for (auto &[clientId, collector] : m_collectors)
    {
        collector->notifyPlaybackStateChanged(sessionId, oldState, newState);
    }
}

void PrivateMetricsService::notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    for (auto &[clientId, collector] : m_collectors)
    {
        collector->notifyApplicationStateChanged(oldState, newState);
    }
}
} // namespace firebolt::rialto::server::service
