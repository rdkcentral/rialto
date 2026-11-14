#include "CliTestSupport.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{

int g_failures = 0;
int g_checks = 0;

#define CHECK(condition)                                                                              \
    do                                                                                               \
    {                                                                                                \
        ++g_checks;                                                                                  \
        if (!(condition))                                                                            \
        {                                                                                            \
            ++g_failures;                                                                            \
            std::fprintf(stderr, "CHECK FAILED %s:%d: %s\n", __FILE__, __LINE__, #condition);       \
        }                                                                                            \
    } while (0)

using rialto_refplayer_cli::CliOptions;
using rialto_refplayer_cli::MetricType;
using rialto_refplayer_cli::ParseStatus;
using rialto_refplayer_cli::RecordingClient;
using rialto_refplayer::FeederFaultStimulus;

class CoutCapture
{
public:
    CoutCapture() : m_previous{std::cout.rdbuf(m_output.rdbuf())} {}
    ~CoutCapture() { std::cout.rdbuf(m_previous); }

    std::string str() const { return m_output.str(); }

private:
    std::ostringstream m_output;
    std::streambuf *m_previous;
};

ParseStatus parse(std::initializer_list<std::string> arguments, CliOptions &options)
{
    std::vector<std::string> storage(arguments);
    std::vector<char *> argv;
    argv.reserve(storage.size());
    for (std::string &argument : storage)
    {
        argv.push_back(argument.data());
    }
    return rialto_refplayer_cli::parseArguments(static_cast<int>(argv.size()), argv.data(), options);
}

rialto_refplayer::Event metricEvent(MetricType type)
{
    rialto_refplayer::Event event;
    event.type = rialto_refplayer::EventType::MetricReceived;
    event.metric.type = type;
    return event;
}

rialto_refplayer::Event playbackCompleteEvent()
{
    rialto_refplayer::Event event;
    event.type = rialto_refplayer::EventType::StateChanged;
    event.state = rialto_refplayer::PlayerState::Complete;
    return event;
}

rialto_refplayer::Event playbackErrorEvent()
{
    rialto_refplayer::Event event;
    event.type = rialto_refplayer::EventType::StateChanged;
    event.state = rialto_refplayer::PlayerState::Error;
    return event;
}

void testExpectedTypesParse()
{
    const std::vector<std::pair<std::string, MetricType>> validTypes{
        {"VIDEO_FRAME_DROP", MetricType::VIDEO_FRAME_DROP},
        {"VIDEO_DECODE_ERROR", MetricType::VIDEO_DECODE_ERROR},
        {"VIDEO_FRAME_REPEAT", MetricType::VIDEO_FRAME_REPEAT},
        {"BUFFER_UNDERFLOW", MetricType::BUFFER_UNDERFLOW},
    };
    for (const auto &[name, type] : validTypes)
    {
        CliOptions options;
        CHECK(parse({"rialto_reference_player", "--expect-metric", name, "--url", "file:///clip.mp4"}, options) ==
              ParseStatus::Run);
        CHECK(options.expectedMetric == type);
    }
}

void testInvalidMetricArgumentsAreRejected()
{
    CliOptions unknownType;
    CHECK(parse({"rialto_reference_player", "--url", "file:///clip.mp4", "--expect-metric", "UNKNOWN"},
                unknownType) == ParseStatus::Error);

    CliOptions missingUrl;
    CHECK(parse({"rialto_reference_player", "--expect-metric", "VIDEO_FRAME_DROP"}, missingUrl) ==
          ParseStatus::Error);

    CliOptions missingValue;
    CHECK(parse({"rialto_reference_player", "--expect-metric"}, missingValue) == ParseStatus::Error);
}

void testFaultInjectionArguments()
{
    struct FaultCase
    {
        const char *name;
        FeederFaultStimulus stimulus;
        uint32_t targetSample;
    };
    const std::vector<FaultCase> faults{{"VIDEO_FRAME_DROP", FeederFaultStimulus::VideoFrameDrop, 15},
                                        {"VIDEO_DECODE_ERROR", FeederFaultStimulus::VideoDecodeError, 15},
                                        {"VIDEO_FRAME_REPEAT", FeederFaultStimulus::VideoFrameRepeat, 8},
                                        {"BUFFER_UNDERFLOW", FeederFaultStimulus::BufferUnderflow, 8}};
    for (const auto &fault : faults)
    {
        CliOptions options;
        CHECK(parse({"rialto_reference_player", "--url", "file:///clip.mp4", "--inject-fault", fault.name,
                     "--expect-metric", fault.name},
                    options) == ParseStatus::Run);
        CHECK(options.feederFaultControl.has_value());
        CHECK(options.feederFaultControl->stimulus == fault.stimulus);
        CHECK(options.feederFaultControl->targetVideoSampleIndex == fault.targetSample);
    }

    CliOptions unknownType;
    CHECK(parse({"rialto_reference_player", "--url", "file:///clip.mp4", "--inject-fault", "UNKNOWN",
                 "--expect-metric", "VIDEO_FRAME_DROP"},
                unknownType) == ParseStatus::Error);

    CliOptions missingUrl;
    CHECK(parse({"rialto_reference_player", "--inject-fault", "VIDEO_FRAME_DROP", "--expect-metric",
                 "VIDEO_FRAME_DROP"},
                missingUrl) == ParseStatus::Error);

    CliOptions noPlayback;
    CHECK(parse({"rialto_reference_player", "--url", "file:///clip.mp4", "--no-playback", "--inject-fault",
                 "VIDEO_FRAME_DROP", "--expect-metric", "VIDEO_FRAME_DROP"},
                noPlayback) == ParseStatus::Error);

    CliOptions missingExpectation;
    CHECK(parse({"rialto_reference_player", "--url", "file:///clip.mp4", "--inject-fault", "VIDEO_FRAME_DROP"},
                missingExpectation) == ParseStatus::Error);

    CliOptions mismatchedExpectation;
    CHECK(parse({"rialto_reference_player", "--url", "file:///clip.mp4", "--inject-fault", "VIDEO_FRAME_DROP",
                 "--expect-metric", "VIDEO_DECODE_ERROR"},
                mismatchedExpectation) == ParseStatus::Run);
}

void testMatchingMetricPasses()
{
    RecordingClient client;
    client.expectMetric(MetricType::VIDEO_FRAME_DROP);
    CoutCapture output;
    client.onEvent(metricEvent(MetricType::VIDEO_FRAME_DROP));
    CHECK(client.waitForExpectedMetricOrPlaybackEnd(std::chrono::milliseconds{0}) ==
          RecordingClient::MetricWait::Received);
    CHECK(output.str().find("notifyMetric: type=VIDEO_FRAME_DROP") != std::string::npos);
}

void testMissingMetricFailsAtPlaybackEnd()
{
    RecordingClient client;
    client.expectMetric(MetricType::VIDEO_FRAME_DROP);
    client.onEvent(playbackCompleteEvent());
    CHECK(client.waitForExpectedMetricOrPlaybackEnd(std::chrono::seconds{1}) ==
          RecordingClient::MetricWait::PlaybackEnded);
}

    void testPlaybackTerminationIsDistinctFromMetricTimeout()
    {
        RecordingClient failedClient;
        failedClient.expectMetric(MetricType::VIDEO_DECODE_ERROR);
        failedClient.onEvent(playbackErrorEvent());
        CHECK(failedClient.waitForExpectedMetricOrPlaybackEnd(std::chrono::milliseconds{0}) ==
            RecordingClient::MetricWait::Failed);

        RecordingClient timedOutClient;
        timedOutClient.expectMetric(MetricType::VIDEO_DECODE_ERROR);
        CHECK(timedOutClient.waitForExpectedMetricOrPlaybackEnd(std::chrono::milliseconds{0}) ==
            RecordingClient::MetricWait::TimedOut);
    }

void testUnrelatedMetricDoesNotSatisfyExpectation()
{
    RecordingClient client;
    client.expectMetric(MetricType::VIDEO_FRAME_DROP);
    client.onEvent(metricEvent(MetricType::VIDEO_DECODE_ERROR));
    client.onEvent(playbackCompleteEvent());
    CHECK(client.waitForExpectedMetricOrPlaybackEnd(std::chrono::seconds{1}) ==
          RecordingClient::MetricWait::PlaybackEnded);
}

void testMetricCallbackLogsCompletePayloadForUnmatchedEvent()
{
    RecordingClient client;
    client.expectMetric(MetricType::BUFFER_UNDERFLOW);
    auto event = metricEvent(MetricType::VIDEO_DECODE_ERROR);
    event.metric.phase = firebolt::rialto::MetricPhase::EPISODE_STARTED;
    event.metric.sourceId = 7;
    event.metric.presentationTimeStampNs = 123456;
    event.metric.count = 4;
    event.metric.durationNs = 98765;
    event.metric.observedAtMonotonicNs = 1000000;
    event.metric.observedAtUnixNs = 2000000;

    CoutCapture output;
    client.onEvent(event);
    const std::string line = output.str();
    CHECK(line.find("notifyMetric: type=VIDEO_DECODE_ERROR") != std::string::npos);
    CHECK(line.find("phase=EPISODE_STARTED") != std::string::npos);
    CHECK(line.find("sourceId=7") != std::string::npos);
    CHECK(line.find("presentationTimeStampNs=123456") != std::string::npos);
    CHECK(line.find("count=4") != std::string::npos);
    CHECK(line.find("durationNs=98765") != std::string::npos);
    CHECK(line.find("observedAtMonotonicNs=1000000") != std::string::npos);
    CHECK(line.find("observedAtUnixNs=2000000") != std::string::npos);

    client.onEvent(playbackCompleteEvent());
    CHECK(client.waitForExpectedMetricOrPlaybackEnd(std::chrono::milliseconds{0}) ==
          RecordingClient::MetricWait::PlaybackEnded);
}

void testMetricCallbackLogsAbsentOptionalFields()
{
    RecordingClient client;
    CoutCapture output;
    client.onEvent(metricEvent(MetricType::VIDEO_FRAME_DROP));

    const std::string line = output.str();
    CHECK(line.find("presentationTimeStampNs=absent") != std::string::npos);
    CHECK(line.find("count=absent") != std::string::npos);
    CHECK(line.find("durationNs=absent") != std::string::npos);
}

void testAsynchronousMetricCallbackPasses()
{
    RecordingClient client;
    client.expectMetric(MetricType::VIDEO_FRAME_REPEAT);
    std::thread callback([&client] { client.onEvent(metricEvent(MetricType::VIDEO_FRAME_REPEAT)); });
    const auto result = client.waitForExpectedMetricOrPlaybackEnd(std::chrono::seconds{1});
    callback.join();
    CHECK(result == RecordingClient::MetricWait::Received);
}

} // namespace

int main()
{
    testExpectedTypesParse();
    testInvalidMetricArgumentsAreRejected();
    testFaultInjectionArguments();
    testMatchingMetricPasses();
    testMissingMetricFailsAtPlaybackEnd();
    testPlaybackTerminationIsDistinctFromMetricTimeout();
    testUnrelatedMetricDoesNotSatisfyExpectation();
    testMetricCallbackLogsCompletePayloadForUnmatchedEvent();
    testMetricCallbackLogsAbsentOptionalFields();
    testAsynchronousMetricCallbackPasses();

    std::printf("refplayer CLI test: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}