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

#ifndef FIREBOLT_RIALTO_SERVER_I_METRICS_COLLECTOR_H_
#define FIREBOLT_RIALTO_SERVER_I_METRICS_COLLECTOR_H_

#include "ControlCommon.h"
#include "IMetricsCollectorClient.h"
#include "MediaCommon.h"
#include <cstdint>
#include <memory>
#include <string>

namespace firebolt::rialto::server
{
/**
 * @brief Client-reported metrics data (mirrors proto ClientProcessMetrics).
 */
struct ClientMetricsData
{
    std::uint64_t sampleId{0};
    MetricsSampleReason reason{MetricsSampleReason::UNKNOWN};
    std::string appName;
    std::uint32_t processId{0};
    std::uint64_t monotonicTimeMs{0};
    std::uint64_t epochTimeMs{0};
    std::uint64_t processCpuTimeMs{0};
    std::uint64_t processMemoryKb{0};
};

class IMetricsCollector;

/**
 * @brief Factory for creating MetricsCollector instances.
 */
class IMetricsCollectorFactory
{
public:
    IMetricsCollectorFactory() = default;
    virtual ~IMetricsCollectorFactory() = default;

    static std::shared_ptr<IMetricsCollectorFactory> createFactory();

    /**
     * @brief Create a new MetricsCollector for a connected client.
     *
     * @param clientId  Unique client identifier.
     * @param client    Callback interface for requesting samples from the client.
     *
     * @return The new MetricsCollector instance, or nullptr on failure.
     */
    virtual std::unique_ptr<IMetricsCollector> create(int clientId,
                                                      const std::shared_ptr<IMetricsCollectorClient> &client) = 0;
};

/**
 * @brief Collects, aggregates, and reports metrics for a single connected client.
 *
 * Each instance owns an ITimer (periodic) that drives sampling, and holds
 * the aggregation and threshold-checking framework classes.
 */
class IMetricsCollector
{
public:
    IMetricsCollector() = default;
    virtual ~IMetricsCollector() = default;

    IMetricsCollector(const IMetricsCollector &) = delete;
    IMetricsCollector(IMetricsCollector &&) = delete;
    IMetricsCollector &operator=(const IMetricsCollector &) = delete;
    IMetricsCollector &operator=(IMetricsCollector &&) = delete;

    /**
     * @brief Process a metrics report received from the client.
     *
     * Computes CPU percentages from deltas, feeds aggregators, checks thresholds.
     *
     * @param metrics  The client-reported metrics data.
     */
    virtual void processMetrics(const ClientMetricsData &metrics) = 0;

    /**
     * @brief Notify that a media pipeline's playback state has changed.
     *
     * Finalizes the old state's aggregator and begins a new one.
     */
    virtual void notifyPlaybackStateChanged(int sessionId, PlaybackState oldState, PlaybackState newState) = 0;

    /**
     * @brief Notify that the application state has changed (RUNNING/INACTIVE).
     *
     * Finalizes the RUNNING aggregator on transition to INACTIVE.
     */
    virtual void notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState) = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_METRICS_COLLECTOR_H_
