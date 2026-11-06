#ifndef FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_

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
    std::int32_t sessionId{0};
    std::uint32_t type{0};
    std::uint32_t phase{0};
    int32_t sourceId{0};
    std::uint64_t ptsNs{0};
    FrameCounters counters;
    std::uint64_t count{0};
    std::uint64_t durationNs{0};
};

const char *metricTypeToString(std::uint32_t type);

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
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_METRICS_I_RIALTO_METRICS_H_