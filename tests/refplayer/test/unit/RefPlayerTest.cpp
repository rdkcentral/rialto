// Host test for the player library against the scripted Rialto mock. It needs no
// device and no MP4 file: an injected demuxer supplies a track model, and the mock
// pipeline answers the way a server would, on its own thread.
//
// Build and run: see "Host test" in files/README.md.

#include "RefPlayer.h"

#include "IDemuxer.h"
#include "IMediaPipeline.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace rr = rialto_refplayer;
using rr::PlayerState;
using rr::Result;

namespace
{

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                                                  \
    do                                                                                               \
    {                                                                                                \
        ++g_checks;                                                                                  \
        if (!(cond))                                                                                 \
        {                                                                                            \
            ++g_failures;                                                                            \
            std::fprintf(stderr, "CHECK FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);             \
        }                                                                                            \
    } while (0)

// Collects every event in arrival order.
class Recorder final : public rr::IRefPlayerClient
{
public:
    void onEvent(const rr::Event &event) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_events.push_back(event);
    }

    std::vector<rr::Event> snapshot()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_events;
    }

    std::vector<PlayerState> states()
    {
        std::vector<PlayerState> out;
        for (const auto &event : snapshot())
        {
            if (event.type == rr::EventType::StateChanged)
            {
                out.push_back(event.state);
            }
        }
        return out;
    }

    std::vector<std::string> failures()
    {
        std::vector<std::string> out;
        for (const auto &event : snapshot())
        {
            if (event.type == rr::EventType::Failed)
            {
                out.push_back(event.description);
            }
        }
        return out;
    }

private:
    std::mutex m_mutex;
    std::vector<rr::Event> m_events;
};

// A two-second-free stand-in for the MP4 parser: 40 video samples of 16 bytes each.
constexpr int kSamples = 40;
constexpr uint32_t kSampleBytes = 16;

class FakeDemuxer final : public rialto_refplayer::demux::IDemuxer
{
public:
    bool parse(std::istream &input, rialto_refplayer::demux::TrackModel &model, std::string &error) override
    {
        (void)input;
        (void)error;
        model.video.width = 640;
        model.video.height = 360;
        model.video.codecData = {1, 2, 3};
        for (int i = 0; i < kSamples; ++i)
        {
            rialto_refplayer::demux::Sample sample;
            sample.offset = static_cast<uint64_t>(i) * kSampleBytes;
            sample.size = kSampleBytes;
            sample.ptsNs = static_cast<int64_t>(i) * 33000000;
            sample.durationNs = 33000000;
            model.video.samples.push_back(sample);
        }
        model.hasAudio = false;
        return true;
    }
};

rr::RefPlayer::Dependencies fakeDependencies(bool demuxAvailable)
{
    rr::RefPlayer::Dependencies deps;
    deps.source.createDemuxer = [demuxAvailable](const std::string &url, const std::string &mime)
        -> std::unique_ptr<rialto_refplayer::demux::IDemuxer> {
        (void)url;
        (void)mime;
        if (!demuxAvailable)
        {
            return nullptr;
        }
        return std::make_unique<FakeDemuxer>();
    };
    deps.source.openStream = [](const std::string &url) -> std::unique_ptr<std::istream> {
        (void)url;
        return std::make_unique<std::istringstream>(std::string(kSamples * kSampleBytes, '\0'));
    };
    return deps;
}

rr::Options options()
{
    rr::Options o;
    o.progressIntervalMs = 20;
    return o;
}

bool waitFor(const std::function<bool()> &predicate, int timeoutMs = 5000)
{
    for (int waited = 0; waited < timeoutMs; waited += 1)
    {
        if (predicate())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return predicate();
}

bool sawStates(const std::vector<PlayerState> &seen, const std::vector<PlayerState> &wanted)
{
    size_t next = 0;
    for (const PlayerState s : seen)
    {
        if (next < wanted.size() && s == wanted[next])
        {
            ++next;
        }
    }
    return next == wanted.size();
}

// Per-test counters. livePipelines is not reset: a leaked pipeline must show up in later tests.
void resetMock()
{
    auto &counters = firebolt::rialto::g_mockPipelineCounters;
    counters.requestBudget = -1;
    counters.segmentsAccepted = 0;
    counters.eosAcknowledged = 0;
    counters.stopsIssued = 0;
}

// Load, play to PLAYING, pause, seek, resume, stop. Checks the sequence in the design (§4.3, §4.6, §4.7).
void testPlayPauseSeekStopReleases()
{
    resetMock();
    rr::RefPlayer concrete(fakeDependencies(true)); // injected demuxer: no MP4 file needed
    auto recorder = std::make_shared<Recorder>();
    concrete.setClient(recorder);
    CHECK(concrete.initialize(options()) == Result::Ok);
    CHECK(concrete.state() == PlayerState::Idle);
    CHECK(firebolt::rialto::g_mockPipelineCounters.livePipelines == 1); // created at initialize, not at load

    // Park the server in PLAYING: three needData requests, then silence.
    firebolt::rialto::g_mockPipelineCounters.requestBudget = 3;
    CHECK(concrete.load("file:///fake/a.mp4", true) == Result::Ok);
    CHECK(concrete.state() == PlayerState::Initializing || concrete.state() == PlayerState::Playing ||
          concrete.state() == PlayerState::Prepared || concrete.state() == PlayerState::Initialized ||
          concrete.state() == PlayerState::Preparing);
    CHECK(waitFor([&] { return concrete.state() == PlayerState::Playing; }));

    CHECK(concrete.pause() == Result::Ok);
    CHECK(waitFor([&] { return concrete.state() == PlayerState::Paused; }));

    CHECK(concrete.seek(1500) == Result::Ok);
    CHECK(waitFor([&] { return concrete.state() == PlayerState::Paused; })); // seek from PAUSED resolves to PAUSED

    CHECK(concrete.play() == Result::Ok);
    CHECK(waitFor([&] { return concrete.state() == PlayerState::Playing; }));
    CHECK(concrete.positionMs() >= 0);

    CHECK(concrete.stop() == Result::Ok);
    CHECK(waitFor([&] { return concrete.state() == PlayerState::Idle; }));
    CHECK(firebolt::rialto::g_mockPipelineCounters.livePipelines == 0); // stop releases the pipeline

    const auto seen = recorder->states();
    CHECK(sawStates(seen, {PlayerState::Initializing, PlayerState::Initialized, PlayerState::Preparing,
                           PlayerState::Prepared, PlayerState::Playing, PlayerState::Paused, PlayerState::Seeking,
                           PlayerState::Paused, PlayerState::Playing, PlayerState::Stopping, PlayerState::Stopped,
                           PlayerState::Released, PlayerState::Idle}));
    CHECK(recorder->failures().empty());
    concrete.destroy();
}

// Run to end of stream. The state reaches COMPLETE and every end-of-stream is acknowledged.
void testEndOfStreamReachesComplete()
{
    resetMock();
    rr::RefPlayer player(fakeDependencies(true));
    auto recorder = std::make_shared<Recorder>();
    player.setClient(recorder);
    CHECK(player.initialize(options()) == Result::Ok);
    CHECK(player.load("file:///fake/eos.mp4", true) == Result::Ok);
    CHECK(waitFor([&] { return player.state() == PlayerState::Complete; }));
    CHECK(firebolt::rialto::g_mockPipelineCounters.eosAcknowledged >= 1);
    CHECK(firebolt::rialto::g_mockPipelineCounters.segmentsAccepted == kSamples);
    CHECK(player.stop() == Result::Ok);
    CHECK(waitFor([&] { return player.state() == PlayerState::Idle; }));
    CHECK(firebolt::rialto::g_mockPipelineCounters.livePipelines == 0);
    player.destroy();
}

// A session can be loaded again on the same object, with no pipeline leaked.
void testReloadWithoutStop()
{
    resetMock();
    rr::RefPlayer player(fakeDependencies(true));
    auto recorder = std::make_shared<Recorder>();
    player.setClient(recorder);
    CHECK(player.initialize(options()) == Result::Ok);

    firebolt::rialto::g_mockPipelineCounters.requestBudget = 2;
    CHECK(player.load("file:///fake/one.mp4", true) == Result::Ok);
    CHECK(waitFor([&] { return player.state() == PlayerState::Playing; }));

    firebolt::rialto::g_mockPipelineCounters.requestBudget = 2;
    CHECK(player.load("file:///fake/two.mp4", true) == Result::Ok);
    CHECK(waitFor([&] { return player.state() == PlayerState::Playing; }));
    CHECK(firebolt::rialto::g_mockPipelineCounters.livePipelines == 1); // the first pipeline went away

    CHECK(player.stop() == Result::Ok);
    CHECK(waitFor([&] { return player.state() == PlayerState::Idle; }));
    CHECK(firebolt::rialto::g_mockPipelineCounters.livePipelines == 0);
    CHECK(recorder->failures().empty());
    player.destroy();
}

// An unsupported source fails with an event and leaves no pipeline behind.
void testUnsupportedSourceFails()
{
    resetMock();
    rr::RefPlayer player(fakeDependencies(false));
    auto recorder = std::make_shared<Recorder>();
    player.setClient(recorder);
    CHECK(player.initialize(options()) == Result::Ok);
    CHECK(player.load("https://example.invalid/stream.mpd", true) == Result::Ok);
    CHECK(waitFor([&] { return player.state() == PlayerState::Error; }));
    const auto failed = recorder->failures();
    CHECK(failed.size() == 1);
    CHECK(!failed.empty() && failed.front().find("no demuxer") != std::string::npos);
    CHECK(firebolt::rialto::g_mockPipelineCounters.livePipelines == 0);
    player.destroy();
}

// Commands that are illegal in the actual state fail with an event, not an exception.
void testIllegalCommandsRaiseEvents()
{
    resetMock();
    rr::RefPlayer player(fakeDependencies(true));
    auto recorder = std::make_shared<Recorder>();
    player.setClient(recorder);
    CHECK(player.initialize(options()) == Result::Ok);
    CHECK(player.play() == Result::Ok); // accepted; the worker rejects it
    CHECK(waitFor([&] { return recorder->failures().size() == 1; }));
    CHECK(player.state() == PlayerState::Idle); // a rejected command does not change the state
    CHECK(player.stop() == Result::Ok);         // stop with nothing loaded is a no-op
    player.destroy();
}

// Phase-2 and phase-4 calls are declared and answer NotSupported.
void testUnbackedCommandsReportNotSupported()
{
    rr::RefPlayer player(fakeDependencies(true));
    CHECK(player.setPlaybackRate(2.0) == Result::NotSupported);
    CHECK(player.setVolume(0.5) == Result::NotSupported);
    CHECK(player.setVideoWindow(0, 0, 1, 1) == Result::NotSupported);
    CHECK(player.setVideoMute(true) == Result::NotSupported);
    CHECK(player.setTextTrack(0) == Result::NotSupported);
    CHECK(player.setClosedCaptions(true) == Result::NotSupported);
    CHECK(!player.volume().has_value());
    CHECK(player.supportedSpeeds().size() == 1 && player.supportedSpeeds().front() == 1.0);
    CHECK(!player.textTracksJson().has_value());
    CHECK(!player.textTrack().has_value());
}

// destroy() is idempotent, releases a playing pipeline, and blocks later control calls.
void testDestroyWhilePlaying()
{
    resetMock();
    rr::RefPlayer player(fakeDependencies(true));
    auto recorder = std::make_shared<Recorder>();
    player.setClient(recorder);
    CHECK(player.initialize(options()) == Result::Ok);
    firebolt::rialto::g_mockPipelineCounters.requestBudget = 2;
    CHECK(player.load("file:///fake/d.mp4", true) == Result::Ok);
    CHECK(waitFor([&] { return player.state() == PlayerState::Playing; }));
    player.destroy();
    player.destroy();
    CHECK(firebolt::rialto::g_mockPipelineCounters.livePipelines == 0);
    CHECK(player.load("file:///fake/e.mp4", true) == Result::InvalidState);
}

// stop() followed at once by load(): the load must not be blocked by the stop's STOPPED wait
// (that wait only times out if the STOPPED callback is dropped). Both sessions reach PLAYING.
void testStopThenLoadDoesNotWaitForTimeout()
{
    resetMock();
    rr::RefPlayer player(fakeDependencies(true));
    auto recorder = std::make_shared<Recorder>();
    player.setClient(recorder);
    CHECK(player.initialize(options()) == Result::Ok);

    const auto countOf = [&recorder](PlayerState wanted) {
        int n = 0;
        for (const PlayerState s : recorder->states())
        {
            n += s == wanted ? 1 : 0;
        }
        return n;
    };

    firebolt::rialto::g_mockPipelineCounters.requestBudget = 2;
    CHECK(player.load("file:///fake/first.mp4", true) == Result::Ok);
    CHECK(waitFor([&] { return countOf(PlayerState::Playing) == 1; }));

    // A 100 ms server acknowledgement keeps the stop in progress while the load arrives.
    firebolt::rialto::g_mockPipelineCounters.stopDelayMs = 100;
    firebolt::rialto::g_mockPipelineCounters.requestBudget = 2;
    const auto start = std::chrono::steady_clock::now();
    CHECK(player.stop() == Result::Ok);
    CHECK(waitFor([&] { return player.state() == PlayerState::Stopping; })); // the worker is inside the stop
    CHECK(player.load("file:///fake/second.mp4", true) == Result::Ok);
    CHECK(waitFor([&] { return countOf(PlayerState::Playing) == 2; }));
    const auto elapsed = std::chrono::steady_clock::now() - start;
    firebolt::rialto::g_mockPipelineCounters.stopDelayMs = 0;
    CHECK(elapsed < std::chrono::milliseconds(1000));

    CHECK(player.stop() == Result::Ok);
    CHECK(waitFor([&] { return player.state() == PlayerState::Idle; }));
    CHECK(firebolt::rialto::g_mockPipelineCounters.livePipelines == 0);
    CHECK(recorder->failures().empty());
    player.destroy();
}

// Rapid load and stop from one thread. Every stop must complete its release, and nothing leaks.
// Waiting on the event stream rather than on the state: the state can read IDLE between
// queued commands, so only one RELEASED and one IDLE per stop proves the cycle finished.
void testRapidLoadStop()
{
    resetMock();
    rr::RefPlayer player(fakeDependencies(true));
    auto recorder = std::make_shared<Recorder>();
    player.setClient(recorder);
    CHECK(player.initialize(options()) == Result::Ok);
    constexpr int kCycles = 25;
    for (int i = 0; i < kCycles; ++i)
    {
        CHECK(player.load("file:///fake/r.mp4", true) == Result::Ok);
        CHECK(player.stop() == Result::Ok);
    }
    const auto count = [&recorder](PlayerState wanted) {
        int n = 0;
        for (const PlayerState s : recorder->states())
        {
            n += s == wanted ? 1 : 0;
        }
        return n;
    };
    CHECK(waitFor([&] { return count(PlayerState::Released) == kCycles; }, 10000));
    CHECK(waitFor([&] { return count(PlayerState::Idle) == kCycles; }, 10000));
    CHECK(firebolt::rialto::g_mockPipelineCounters.livePipelines == 0);
    CHECK(recorder->failures().empty());
    player.destroy();
}

} // namespace

int main()
{
    testPlayPauseSeekStopReleases();
    testEndOfStreamReachesComplete();
    testReloadWithoutStop();
    testUnsupportedSourceFails();
    testIllegalCommandsRaiseEvents();
    testUnbackedCommandsReportNotSupported();
    testDestroyWhilePlaying();
    testRapidLoadStop();
    testStopThenLoadDoesNotWaitForTimeout();

    std::printf("refplayer host test: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
