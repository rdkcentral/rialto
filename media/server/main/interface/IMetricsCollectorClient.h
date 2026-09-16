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

#ifndef FIREBOLT_RIALTO_SERVER_I_METRICS_COLLECTOR_CLIENT_H_
#define FIREBOLT_RIALTO_SERVER_I_METRICS_COLLECTOR_CLIENT_H_

#include <cstdint>

namespace firebolt::rialto::server
{
/**
 * @brief Reason for a metrics sample request, mirroring the proto enum.
 */
enum class MetricsSampleReason
{
    UNKNOWN,
    CONNECTED,
    PERIODIC,
    STATE_TRANSITION
};

/**
 * @brief Callback interface used by MetricsCollector (server/main) to send
 *        sample requests back through the IPC layer to the client.
 */
class IMetricsCollectorClient
{
public:
    IMetricsCollectorClient() = default;
    virtual ~IMetricsCollectorClient() = default;

    IMetricsCollectorClient(const IMetricsCollectorClient &) = delete;
    IMetricsCollectorClient(IMetricsCollectorClient &&) = delete;
    IMetricsCollectorClient &operator=(const IMetricsCollectorClient &) = delete;
    IMetricsCollectorClient &operator=(IMetricsCollectorClient &&) = delete;

    /**
     * @brief Request that the client send a metrics sample.
     *
     * @param clientId   The client to request from.
     * @param sampleId   Unique sample identifier for correlation.
     * @param reason     Why the sample is being requested.
     */
    virtual void requestMetricsSample(int clientId, std::uint64_t sampleId, MetricsSampleReason reason) = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_METRICS_COLLECTOR_CLIENT_H_
