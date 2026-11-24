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

#ifndef FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_

#include <MediaCommon.h>
#include <gst/gst.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <utility>

namespace firebolt::rialto::server
{
class ElementToSourceMapper;

/**
 * Server-side cache entry: the public frame counters plus the monotonic time they were last updated.
 */
struct CounterSnapshot
{
    VideoFrameCounters counters;
    uint64_t timestamp{0};
};

class IRialtoMetrics
{
public:
    /**
     * Reads the current rendered/dropped counts from a sink. Returns false if they are unavailable.
     */
    using SinkStatsGetter = std::function<bool(const MediaSourceType &mediaSourceType, std::uint64_t &renderedFrames,
                                               std::uint64_t &droppedFrames)>;

    virtual ~IRialtoMetrics() = default;
    virtual void setSessionId(int32_t sessionId) = 0;
    virtual std::optional<MetricEvent> onMetricMessage(GstMessage *message) = 0;
    virtual void updateFrameCounters(int32_t sessionId, int32_t sourceId, const CounterSnapshot &snapshot) = 0;
    virtual void updateSinkStats(int32_t sessionId, int32_t sourceId, std::uint64_t renderedFrames,
                                std::uint64_t droppedFrames) = 0;
    virtual bool getFrameCounters(int32_t sessionId, int32_t sourceId, const MediaSourceType &mediaSourceType,
                                  CounterSnapshot &snapshot) = 0;
    virtual ElementToSourceMapper* getElementToSourceMapper() = 0;
};

class ElementToSourceMapper
{
public:
    virtual ~ElementToSourceMapper() = default;
    virtual void RegisterElement(GstElement* element, uint32_t session_id, uint32_t source_id) = 0;
    virtual bool MapElement(GstElement* element, uint32_t& out_session_id, uint32_t& out_source_id) = 0;
    virtual void UnregisterElement(GstElement* element) = 0;
    virtual void UnregisterAllElements() = 0;
};

GstFlowReturn ValidateMediaPipelineMetric(const GstStructure* msg, MetricEvent& out,
										   GstMessage* gstMessage = nullptr, 
										   ElementToSourceMapper* mapper = nullptr,
										   int32_t* outSessionId = nullptr);

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_