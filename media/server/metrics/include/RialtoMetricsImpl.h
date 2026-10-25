#ifndef FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_IMPL_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_IMPL_H_

#include "RialtoMetrics.h"
#include <map>
#include <mutex>

namespace firebolt::rialto::server
{
class RialtoMetricsImpl final : public RialtoMetrics
{
public:
    ~RialtoMetricsImpl() override = default;
    void onMetricMessage(GstMessage *message) override;
    void updateFrameCounters(int32_t sessionId, int32_t sourceId, const FrameCounters &counters) override;
    bool getFrameCounters(int32_t sessionId, int32_t sourceId, FrameCounters &counters) const override;

private:
    using CounterKey = std::pair<int32_t, int32_t>;

    mutable std::mutex m_counterMutex;
    std::map<CounterKey, FrameCounters> m_counterCache;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_IMPL_H_