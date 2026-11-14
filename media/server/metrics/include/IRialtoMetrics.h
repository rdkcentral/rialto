#ifndef FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_

#include <MediaCommon.h>
#include <gst/gst.h>

#include <cstdint>
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
    virtual ~IRialtoMetrics() = default;
    virtual void setSessionId(int32_t sessionId) = 0;
    virtual std::optional<MetricEvent> onMetricMessage(
        GstMessage *message, const std::optional<std::uint64_t> &renderedFrames,
        const std::optional<std::uint64_t> &droppedFrames) = 0;
    virtual void updateFrameCounters(int32_t sessionId, int32_t sourceId, const CounterSnapshot &snapshot) = 0;
    virtual void updateSinkStats(int32_t sessionId, int32_t sourceId, std::uint64_t renderedFrames,
                                std::uint64_t droppedFrames) = 0;
    virtual bool getFrameCounters(int32_t sessionId, int32_t sourceId, CounterSnapshot &snapshot) const = 0;
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
										   ElementToSourceMapper* mapper = nullptr);

}

#endif // FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_