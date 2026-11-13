#ifndef FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_

#include "IRialtoMetrics.h"
#include <map>
#include <mutex>
#include <memory>

namespace firebolt::rialto::server
{
class RialtoMetrics final : public IRialtoMetrics
{
public:
    ~RialtoMetrics() override;
    void setSessionId(int32_t sessionId) override;
    std::optional<ParsedMetricMessage> onMetricMessage(
        GstMessage *message, const std::optional<std::uint64_t> &renderedFrames,
        const std::optional<std::uint64_t> &droppedFrames) override;
    void updateFrameCounters(int32_t sessionId, int32_t sourceId, const FrameCounters &counters) override;
    void updateSinkStats(int32_t sessionId, int32_t sourceId, std::uint64_t renderedFrames,
                         std::uint64_t droppedFrames) override;
    bool getFrameCounters(int32_t sessionId, int32_t sourceId, FrameCounters &counters) const override;
    ElementToSourceMapper* getElementToSourceMapper() override;

private:
    using CounterKey = std::pair<int32_t, int32_t>;

    mutable std::mutex m_counterMutex;
    int32_t m_sessionId{-1};
    std::map<CounterKey, FrameCounters> m_counterCache;
    std::unique_ptr<ElementToSourceMapper> m_elementToSourceMapper;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_