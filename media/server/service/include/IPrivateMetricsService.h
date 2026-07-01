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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_I_PRIVATE_METRICS_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_I_PRIVATE_METRICS_SERVICE_H_

#include "ControlCommon.h"
#include "IMetricsCollector.h"
#include "IMetricsCollectorClient.h"
#include "MediaCommon.h"
#include <memory>

namespace firebolt::rialto::server::service
{
class IPrivateMetricsService
{
public:
    IPrivateMetricsService() = default;
    virtual ~IPrivateMetricsService() = default;

    IPrivateMetricsService(const IPrivateMetricsService &) = delete;
    IPrivateMetricsService(IPrivateMetricsService &&) = delete;
    IPrivateMetricsService &operator=(const IPrivateMetricsService &) = delete;
    IPrivateMetricsService &operator=(IPrivateMetricsService &&) = delete;

    /**
     * @brief A client has signalled readiness for metrics collection.
     *
     * Creates a MetricsCollector instance for this client.
     *
     * @param clientId  Unique client identifier.
     * @param client    Callback interface for requesting samples from the client.
     */
    virtual void clientReady(int clientId, const std::shared_ptr<firebolt::rialto::server::IMetricsCollectorClient> &client) = 0;

    /**
     * @brief A client has disconnected.
     *
     * Destroys the MetricsCollector instance associated with this client.
     *
     * @param clientId  The client that disconnected.
     */
    virtual void clientDisconnected(int clientId) = 0;

    /**
     * @brief Process metrics data received from a client.
     *
     * Routes the data to the appropriate MetricsCollector.
     *
     * @param clientId  The reporting client.
     * @param metrics   The client-reported metrics data.
     */
    virtual void reportMetrics(int clientId, const firebolt::rialto::server::ClientMetricsData &metrics) = 0;

    /**
     * @brief Notify that a media pipeline's playback state has changed.
     *
     * Routes to all active MetricsCollector instances.
     */
    virtual void notifyPlaybackStateChanged(int sessionId, PlaybackState oldState, PlaybackState newState) = 0;

    /**
     * @brief Notify that the application state has changed (RUNNING/INACTIVE).
     *
     * Routes to all active MetricsCollector instances.
     */
    virtual void notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState) = 0;
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_I_PRIVATE_METRICS_SERVICE_H_
