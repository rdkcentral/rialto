#ifndef FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_

#include <gst/gst.h>

#include <cstdint>
#include <optional>
#include <utility>

namespace firebolt::rialto::server
{
enum class MediaMetricType : std::uint32_t
{
    MEDIA_METRIC_VIDEO_FRAME_DROP = 1,
    MEDIA_METRIC_AUDIO_FRAME_DROP = 2,
    MEDIA_METRIC_VIDEO_DECODE_ERROR = 3,
    MEDIA_METRIC_AUDIO_DECODE_ERROR = 4,
    MEDIA_METRIC_VIDEO_FRAME_REPEAT = 5,
    MEDIA_METRIC_AUDIO_GAP = 6,
    MEDIA_METRIC_BUFFER_UNDERFLOW = 7
};

enum class ObservationKind : std::uint32_t
{
    OCCURRENCE = 1,
    EPISODE_STARTED = 2,
    EPISODE_RESOLVED = 3
};

struct FrameCounters
{
    std::uint64_t rendered{0};
    std::uint64_t dropped{0};
    std::uint64_t corrupted{0};
    std::uint64_t timestamp{0};
};

struct ParsedMetricMessage
{
    std::int32_t session_id{0};
    std::uint32_t metric{0};
    std::uint32_t observation_kind{0};
    int32_t source_id{0};
    std::optional<std::int64_t> pts_ns;
    std::optional<std::uint64_t> count;
    std::optional<std::uint64_t> duration_ns;
    std::uint64_t observed_at_monotonic_ns{0};
    std::int64_t observed_at_unix_ns{0};
};

class IRialtoMetrics
{
public:
    virtual ~IRialtoMetrics() = default;
    virtual void setSessionId(int32_t sessionId) = 0;
    virtual std::optional<ParsedMetricMessage> onMetricMessage(
        GstMessage *message, const std::optional<std::uint64_t> &renderedFrames,
        const std::optional<std::uint64_t> &droppedFrames) = 0;
    virtual void updateFrameCounters(int32_t sessionId, int32_t sourceId, const FrameCounters &counters) = 0;
    virtual void updateSinkStats(int32_t sessionId, int32_t sourceId, std::uint64_t renderedFrames,
                                std::uint64_t droppedFrames) = 0;
    virtual bool getFrameCounters(int32_t sessionId, int32_t sourceId, FrameCounters &counters) const = 0;
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

GstFlowReturn ValidateMediaPipelineMetric(const GstStructure* msg, ParsedMetricMessage& out, 
										   GstMessage* gstMessage = nullptr, 
										   ElementToSourceMapper* mapper = nullptr);


#endif // FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_