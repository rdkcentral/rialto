#ifndef FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_

#include <cstdint>
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

class RialtoMetrics
{
public:
    virtual ~RialtoMetrics() = default;
    virtual void onMetricMessage(GstMessage *message) = 0;
    virtual void updateFrameCounters(int32_t sessionId, int32_t sourceId, const FrameCounters &counters) = 0;
    virtual bool getFrameCounters(int32_t sessionId, int32_t sourceId, FrameCounters &counters) const = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_