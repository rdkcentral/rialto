#pragma once

#include "rialto_refplayer/IRefPlayer.h"

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace rialto_refplayer_cli
{

using firebolt::rialto::MetricType;
using rialto_refplayer::FeederFaultControl;

constexpr int64_t kDefaultTimeoutMs = 30000;

struct CliOptions
{
    std::string url;
    std::string mimeType{"video/mp4"};
    bool isLive{false};
    bool validateCapabilities{true};
    bool useWebAudio{false};
    bool enablePlayback{true};
    bool negativeRuns{false};
    std::optional<MetricType> expectedMetric;
    std::optional<FeederFaultControl> feederFaultControl;
    std::optional<double> desiredVolume;
    std::optional<int64_t> seekNs;
    uint32_t priority{1};
    int64_t timeoutMs{kDefaultTimeoutMs};
};

enum class ParseStatus
{
    Run,
    Help,
    Error
};

ParseStatus parseArguments(int argc, char **argv, CliOptions &options);
const char *metricTypeName(MetricType type);

class RecordingClient final : public rialto_refplayer::IRefPlayerClient
{
public:
    enum class Wait
    {
        Reached,
        Failed,
        TimedOut
    };

    enum class MetricWait
    {
        Received,
        PlaybackEnded,
        Failed,
        TimedOut
    };

    void onEvent(const rialto_refplayer::Event &event) override;
    void expectMetric(MetricType expectedMetric);
    Wait waitForState(rialto_refplayer::PlayerState target, std::chrono::milliseconds ceiling);
    MetricWait waitForExpectedMetricOrPlaybackEnd(std::chrono::milliseconds ceiling);
    int64_t eventCount() const;
    std::vector<firebolt::rialto::MetricEvent> metricEvents() const;

private:
    bool seen(rialto_refplayer::PlayerState state) const;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::array<bool, 16> m_seen{};
    std::vector<firebolt::rialto::MetricEvent> m_metrics;
    std::optional<MetricType> m_expectedMetric;
    bool m_expectedMetricReceived{false};
    bool m_failed{false};
    int64_t m_events{0};
};

} // namespace rialto_refplayer_cli