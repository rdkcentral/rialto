/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_RIALTO_METRICS_H_
#define FIREBOLT_RIALTO_SERVER_RIALTO_METRICS_H_

#include "IRialtoMetrics.h"
#include <map>
#include <memory>
#include <mutex>

namespace firebolt::rialto::server
{
MetricPhase convertObservationKind(std::uint32_t observationKind);

class RialtoMetrics final : public IRialtoMetrics
{
public:
    RialtoMetrics(SinkStatsGetter getSinkStats = nullptr);
    ~RialtoMetrics() override;
    void setSessionId(int32_t sessionId) override;
    std::optional<MetricEvent> onMetricMessage(GstMessage *message) override;
    void updateFrameCounters(int32_t sessionId, int32_t sourceId, const CounterSnapshot &snapshot) override;
    void updateSinkStats(int32_t sessionId, int32_t sourceId, std::uint64_t renderedFrames,
                         std::uint64_t droppedFrames) override;
    bool getFrameCounters(int32_t sessionId, int32_t sourceId, const MediaSourceType &mediaSourceType,
                          CounterSnapshot &snapshot) override;
    ElementToSourceMapper *getElementToSourceMapper() override;

private:
    using CounterKey = std::pair<int32_t, int32_t>;

    bool refreshFrameCounters(int32_t sessionId, int32_t sourceId, const MediaSourceType &mediaSourceType);

    mutable std::mutex m_counterMutex;
    int32_t m_sessionId{-1};
    std::map<CounterKey, CounterSnapshot> m_counterCache;
    std::unique_ptr<ElementToSourceMapper> m_elementToSourceMapper;
    SinkStatsGetter m_getSinkStats;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_RIALTO_METRICS_H_
