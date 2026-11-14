#include "CliTestSupport.h"

#include "CliLog.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace rialto_refplayer_cli
{
using rialto_refplayer::EventType;
using rialto_refplayer::FeederFaultControl;
using rialto_refplayer::FeederFaultStimulus;

namespace
{

bool parseDouble(const std::string &text, double &value)
{
    try
    {
        std::size_t used = 0;
        value = std::stod(text, &used);
        return used == text.size();
    }
    catch (const std::logic_error &)
    {
        return false;
    }
}

bool parseInt64(const std::string &text, int64_t &value)
{
    try
    {
        std::size_t used = 0;
        value = std::stoll(text, &used);
        return used == text.size();
    }
    catch (const std::logic_error &)
    {
        return false;
    }
}

std::optional<MetricType> parseMetricType(const std::string &text)
{
    if (text == "VIDEO_FRAME_DROP")
    {
        return MetricType::VIDEO_FRAME_DROP;
    }
    if (text == "VIDEO_DECODE_ERROR")
    {
        return MetricType::VIDEO_DECODE_ERROR;
    }
    if (text == "VIDEO_FRAME_REPEAT")
    {
        return MetricType::VIDEO_FRAME_REPEAT;
    }
    if (text == "BUFFER_UNDERFLOW")
    {
        return MetricType::BUFFER_UNDERFLOW;
    }
    return std::nullopt;
}

std::optional<FeederFaultControl> parseFeederFault(const std::string &text)
{
    const auto type = parseMetricType(text);
    if (!type)
    {
        return std::nullopt;
    }

    switch (*type)
    {
    case MetricType::VIDEO_FRAME_DROP:
        return FeederFaultControl{FeederFaultStimulus::VideoFrameDrop, 15};
    case MetricType::VIDEO_DECODE_ERROR:
        return FeederFaultControl{FeederFaultStimulus::VideoDecodeError, 15};
    case MetricType::VIDEO_FRAME_REPEAT:
        return FeederFaultControl{FeederFaultStimulus::VideoFrameRepeat, 8};
    case MetricType::BUFFER_UNDERFLOW:
        return FeederFaultControl{FeederFaultStimulus::BufferUnderflow, 8};
    default:
        return std::nullopt;
    }
}

const char *metricPhaseName(firebolt::rialto::MetricPhase phase)
{
    switch (phase)
    {
    case firebolt::rialto::MetricPhase::OCCURRENCE:
        return "OCCURRENCE";
    case firebolt::rialto::MetricPhase::EPISODE_STARTED:
        return "EPISODE_STARTED";
    case firebolt::rialto::MetricPhase::EPISODE_RESOLVED:
        return "EPISODE_RESOLVED";
    default:
        return "UNKNOWN";
    }
}

template <typename T> std::string formatOptional(const std::optional<T> &value)
{
    return value ? std::to_string(*value) : "absent";
}

std::string formatMetricEvent(const firebolt::rialto::MetricEvent &metric)
{
    return "notifyMetric: type=" + std::string{metricTypeName(metric.type)} +
           ", phase=" + metricPhaseName(metric.phase) + ", sourceId=" + std::to_string(metric.sourceId) +
           ", presentationTimeStampNs=" + formatOptional(metric.presentationTimeStampNs) +
           ", count=" + formatOptional(metric.count) + ", durationNs=" + formatOptional(metric.durationNs) +
           ", observedAtMonotonicNs=" + std::to_string(metric.observedAtMonotonicNs) +
           ", observedAtUnixNs=" + std::to_string(metric.observedAtUnixNs);
}

ParseStatus argumentError(const std::string &message)
{
    std::cerr << "[rialto-ref-player] error: " << message << std::endl;
    return ParseStatus::Error;
}

void printUsage(std::ostream &out)
{
    out << "Usage: rialto_reference_player [options]\n"
           "  --url <url>                 Media URL; a local file:// .mp4 (H.264) is demuxed and fed over MSE\n"
           "  --mime <mime>               MIME type for the capability probes (default: video/mp4); phase 1 plays\n"
           "                              only video/mp4 MP4 files, so playback ignores this value\n"
           "  --live                      CLI only: do not wait for end of stream after PLAYING\n"
           "  --web-audio                 Also create an IWebAudioPlayer and validate the PCM API\n"
           "  --negative-runs             Run negative API checks after contract validation\n"
           "  --expect-metric <type>      Require a metric callback of the requested type during playback\n"
           "                              Types: VIDEO_FRAME_DROP, VIDEO_DECODE_ERROR, VIDEO_FRAME_REPEAT, BUFFER_UNDERFLOW\n"
           "  --inject-fault <type>       Stimulate a feeder fault during playback (requires --expect-metric)\n"
           "                              Types: VIDEO_FRAME_DROP, VIDEO_DECODE_ERROR, VIDEO_FRAME_REPEAT, BUFFER_UNDERFLOW\n"
           "  --set-volume <v>            Volume in [0.0, 1.0]; set and read back on a probe pipeline\n"
           "  --seek <ns>                 Seek to a position in nanoseconds; needs --url, not --no-playback\n"
           "  --priority <n>              Audio player priority (default: 1)\n"
           "  --no-playback               Validate contract without calling play()\n"
           "  --no-validate-capabilities  Skip capability tests\n"
           "  --timeout-ms <ms>           Ceiling for each wait on a player state (default: 30000)\n"
           "  --help                      Print this help text\n";
}

} // namespace

const char *metricTypeName(MetricType type)
{
    switch (type)
    {
    case MetricType::VIDEO_FRAME_DROP:
        return "VIDEO_FRAME_DROP";
    case MetricType::AUDIO_FRAME_DROP:
        return "AUDIO_FRAME_DROP";
    case MetricType::VIDEO_DECODE_ERROR:
        return "VIDEO_DECODE_ERROR";
    case MetricType::AUDIO_DECODE_ERROR:
        return "AUDIO_DECODE_ERROR";
    case MetricType::VIDEO_FRAME_REPEAT:
        return "VIDEO_FRAME_REPEAT";
    case MetricType::AUDIO_GAP:
        return "AUDIO_GAP";
    case MetricType::BUFFER_UNDERFLOW:
        return "BUFFER_UNDERFLOW";
    default:
        return "UNKNOWN";
    }
}

ParseStatus parseArguments(int argc, char **argv, CliOptions &options)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--help")
        {
            printUsage(std::cout);
            return ParseStatus::Help;
        }
        if (arg == "--live")
        {
            options.isLive = true;
        }
        else if (arg == "--no-validate-capabilities")
        {
            options.validateCapabilities = false;
        }
        else if (arg == "--web-audio")
        {
            options.useWebAudio = true;
        }
        else if (arg == "--no-playback")
        {
            options.enablePlayback = false;
        }
        else if (arg == "--negative-runs")
        {
            options.negativeRuns = true;
        }
        else if (arg == "--url" || arg == "--mime" || arg == "--set-volume" || arg == "--seek" ||
                 arg == "--expect-metric" || arg == "--inject-fault" || arg == "--priority" ||
                 arg == "--timeout-ms")
        {
            if (i + 1 >= argc)
            {
                return argumentError("missing value for " + arg);
            }
            const std::string value = argv[++i];
            if (arg == "--url")
            {
                options.url = value;
            }
            else if (arg == "--expect-metric")
            {
                options.expectedMetric = parseMetricType(value);
                if (!options.expectedMetric)
                {
                    return argumentError("unsupported metric type '" + value + "' for --expect-metric");
                }
            }
            else if (arg == "--inject-fault")
            {
                options.feederFaultControl = parseFeederFault(value);
                if (!options.feederFaultControl)
                {
                    return argumentError("unsupported fault type '" + value + "' for --inject-fault");
                }
            }
            else if (arg == "--mime")
            {
                options.mimeType = value;
            }
            else if (arg == "--set-volume")
            {
                double volume = 0.0;
                if (!parseDouble(value, volume) || !(volume >= 0.0 && volume <= 1.0))
                {
                    return argumentError("--set-volume must be a number in [0.0, 1.0]");
                }
                options.desiredVolume = volume;
            }
            else if (arg == "--seek")
            {
                int64_t ns = 0;
                if (!parseInt64(value, ns) || ns < 0)
                {
                    return argumentError("--seek must be a non-negative integer in nanoseconds");
                }
                options.seekNs = ns;
            }
            else if (arg == "--priority")
            {
                int64_t priority = 0;
                if (!parseInt64(value, priority) || priority < 0 ||
                    priority > static_cast<int64_t>(std::numeric_limits<uint32_t>::max()))
                {
                    return argumentError("--priority must be a non-negative integer");
                }
                options.priority = static_cast<uint32_t>(priority);
            }
            else
            {
                int64_t timeout = 0;
                if (!parseInt64(value, timeout) || timeout <= 0)
                {
                    return argumentError("--timeout-ms must be a positive integer");
                }
                options.timeoutMs = timeout;
            }
        }
        else
        {
            return argumentError("unknown option " + arg + " (see --help)");
        }
    }

    if (options.seekNs && options.url.empty())
    {
        return argumentError("--seek needs --url");
    }
    if (options.seekNs && !options.enablePlayback)
    {
        return argumentError("--seek cannot be combined with --no-playback");
    }
    if (options.expectedMetric && options.url.empty())
    {
        return argumentError("--expect-metric needs --url");
    }
    if (options.feederFaultControl && options.url.empty())
    {
        return argumentError("--inject-fault needs --url");
    }
    if (options.feederFaultControl && !options.enablePlayback)
    {
        return argumentError("--inject-fault cannot be combined with --no-playback");
    }
    if (options.feederFaultControl && !options.expectedMetric)
    {
        return argumentError("--inject-fault needs --expect-metric to verify the pipeline result");
    }
    return ParseStatus::Run;
}

void RecordingClient::onEvent(const rialto_refplayer::Event &event)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_events;
        if (event.type == EventType::StateChanged)
        {
            const auto index = static_cast<std::size_t>(event.state);
            if (index < m_seen.size())
            {
                m_seen[index] = true;
            }
            cliLog(std::string("state: ") + rialto_refplayer::toString(event.state));
        }
        else if (event.type == EventType::Failed)
        {
            m_failed = true;
            cliLog("playbackFailed: " + event.description);
        }
        else if (event.type == EventType::MetricReceived)
        {
            m_metrics.push_back(event.metric);
            if (m_expectedMetric && event.metric.type == *m_expectedMetric)
            {
                m_expectedMetricReceived = true;
            }
            cliLog(formatMetricEvent(event.metric));
        }
    }
    m_cv.notify_all();
}

void RecordingClient::expectMetric(MetricType expectedMetric)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_expectedMetric = expectedMetric;
    m_expectedMetricReceived = std::any_of(m_metrics.begin(), m_metrics.end(),
                                           [expectedMetric](const auto &metric) {
                                               return metric.type == expectedMetric;
                                           });
}

RecordingClient::Wait RecordingClient::waitForState(rialto_refplayer::PlayerState target,
                                                    std::chrono::milliseconds ceiling)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait_for(lock, ceiling,
                  [this, target] { return seen(target) || seen(rialto_refplayer::PlayerState::Error) || m_failed; });
    if (seen(target))
    {
        return Wait::Reached;
    }
    return (m_failed || seen(rialto_refplayer::PlayerState::Error)) ? Wait::Failed : Wait::TimedOut;
}

RecordingClient::MetricWait RecordingClient::waitForExpectedMetricOrPlaybackEnd(std::chrono::milliseconds ceiling)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait_for(lock, ceiling, [this] {
        return m_expectedMetricReceived || seen(rialto_refplayer::PlayerState::Complete) ||
               seen(rialto_refplayer::PlayerState::Error) || m_failed;
    });
    if (m_expectedMetricReceived)
    {
        return MetricWait::Received;
    }
    if (seen(rialto_refplayer::PlayerState::Complete))
    {
        return MetricWait::PlaybackEnded;
    }
    return (m_failed || seen(rialto_refplayer::PlayerState::Error)) ? MetricWait::Failed : MetricWait::TimedOut;
}

int64_t RecordingClient::eventCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_events;
}

std::vector<firebolt::rialto::MetricEvent> RecordingClient::metricEvents() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metrics;
}

bool RecordingClient::seen(rialto_refplayer::PlayerState state) const
{
    const auto index = static_cast<std::size_t>(state);
    return index < m_seen.size() && m_seen[index];
}

} // namespace rialto_refplayer_cli