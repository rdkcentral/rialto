#ifndef FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_

#include <cstdint>
#include <optional>
#include <utility>

typedef struct _GstMessage GstMessage;

namespace firebolt::rialto::server
{
struct FrameCounters
{
    std::uint64_t rendered{0};
    std::uint64_t dropped{0};
    std::uint64_t corrupted{0};
    std::uint64_t timestamp{0};
};

struct ParsedMetricMessage
{
    std::uint32_t type{0};
    int32_t sourceId{0};
    FrameCounters counters;
};

inline const char *metricTypeToString(std::uint32_t type)
{
    switch (type)
    {
    case 0:
        return "METRIC_VIDEO_FRAME_DROP";
    case 1:
        return "METRIC_AUDIO_FRAME_DROP";
    case 2:
        return "METRIC_VIDEO_DECODE_ERROR";
    case 3:
        return "METRIC_AUDIO_DECODE_ERROR";
    case 4:
        return "METRIC_VIDEO_FRAME_REPEAT";
    case 5:
        return "METRIC_AUDIO_GAP";
    case 6:
        return "METRIC_BUFFER_UNDERFLOW";
    default:
        return "METRIC_UNKNOWN";
    }
}

class RialtoMetrics
{
public:
    virtual ~RialtoMetrics() = default;
    virtual void setSessionId(int32_t sessionId) = 0;
    virtual std::optional<ParsedMetricMessage> onMetricMessage(GstMessage *message) = 0;
    virtual void updateFrameCounters(int32_t sessionId, int32_t sourceId, const FrameCounters &counters) = 0;
    virtual bool getFrameCounters(int32_t sessionId, int32_t sourceId, FrameCounters &counters) const = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_