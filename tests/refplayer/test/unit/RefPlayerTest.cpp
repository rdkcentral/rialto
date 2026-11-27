// Host test for the player library against the scripted Rialto mock. It needs no
// device and no MP4 file: an injected demuxer supplies a track model, and the mock
// pipeline answers the way a server would, on its own thread.
//
// Build and run: see "Host test" in files/README.md.

#include "RefPlayer.h"

#include "IDemuxer.h"
#include "IMediaPipeline.h"
#include "Mp4Demuxer.h"
#include "SourceManager.h"

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

// runOnPipeline() runs the task on the worker against the loaded pipeline, with the attached
// source ids, and refuses when nothing is loaded.
void testRunOnPipelineUsesLivePipeline()
{
    resetMock();
    rr::RefPlayer player(fakeDependencies(true));
    auto recorder = std::make_shared<Recorder>();
    player.setClient(recorder);
    const auto timeout = std::chrono::milliseconds(5000);

    int ran = 0;
    const rr::PipelineTask task = [&](firebolt::rialto::IMediaPipeline &pipeline, const rr::PipelineSources &sources) {
        ++ran;
        bool sync = false;
        CHECK(pipeline.setSync(true)); // the real, live pipeline object answers
        CHECK(pipeline.getSync(sync));
        bool immediate = false;
        uint64_t rendered = 0;
        uint64_t dropped = 0;
        uint32_t queued = 0;
        CHECK(pipeline.setImmediateOutput(sources.videoSourceId, true));
        CHECK(pipeline.getImmediateOutput(sources.videoSourceId, immediate) && immediate);
        CHECK(pipeline.setReportDecodeErrors(sources.videoSourceId, true));
        CHECK(pipeline.getStats(sources.videoSourceId, rendered, dropped));
        CHECK(pipeline.getQueuedFrames(sources.videoSourceId, queued));
        CHECK(pipeline.renderFrame());
        CHECK(!pipeline.getStats(sources.videoSourceId + 100, rendered, dropped)); // unknown source
        CHECK(sources.videoSourceId != 0);
        CHECK(sources.audioSourceId == 0); // the fake media has no audio
    };

    CHECK(player.durationMs() == 0);
    CHECK(player.runOnPipeline(task, timeout) == Result::InvalidState); // not initialized
    CHECK(player.initialize(options()) == Result::Ok);
    CHECK(player.runOnPipeline(task, timeout) == Result::InvalidState); // nothing loaded yet
    CHECK(ran == 0);

    firebolt::rialto::g_mockPipelineCounters.requestBudget = 3;
    CHECK(player.load("file:///fake/r.mp4", true) == Result::Ok);
    CHECK(waitFor([&] { return player.state() == PlayerState::Playing; }));
    CHECK(player.runOnPipeline(task, timeout) == Result::Ok);
    CHECK(ran == 1);
    // The mock server sends no duration, so it comes from the demuxer: 40 samples of 33 ms.
    CHECK(player.durationMs() == 1320);

    CHECK(player.stop() == Result::Ok);
    CHECK(waitFor([&] { return player.state() == PlayerState::Idle; }));
    CHECK(player.runOnPipeline(task, timeout) == Result::InvalidState); // released by stop
    CHECK(ran == 1);
    CHECK(recorder->failures().empty());
    player.destroy();
}

} // namespace


// ---- Codec support: MP4 parsing and the source description sent to the server ----

using Bytes = std::vector<uint8_t>;

void put32(Bytes &out, uint32_t v)
{
    for (int shift = 24; shift >= 0; shift -= 8)
    {
        out.push_back(static_cast<uint8_t>((v >> shift) & 0xFF));
    }
}

void put16(Bytes &out, uint16_t v)
{
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v & 0xFF));
}

Bytes box(const char (&type)[5], const Bytes &payload)
{
    Bytes out;
    put32(out, static_cast<uint32_t>(8 + payload.size()));
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

Bytes cat(std::initializer_list<Bytes> parts)
{
    Bytes out;
    for (const auto &part : parts)
    {
        out.insert(out.end(), part.begin(), part.end());
    }
    return out;
}

Bytes fullBoxPayload(const Bytes &body)
{
    Bytes out(4, 0); // version and flags
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

// Visual sample entry: the 78-byte header, then the codec configuration box.
Bytes visualEntry(const char (&type)[5], const Bytes &configBox)
{
    Bytes payload(24, 0); // reserved, data reference index, pre-defined
    put16(payload, 640);
    put16(payload, 360);
    payload.resize(78, 0);
    return box(type, cat({payload, configBox}));
}

// Audio sample entry (version 0): 28-byte header, then the codec configuration box.
Bytes audioEntry(const char (&type)[5], uint16_t channels, uint32_t rate, const Bytes &configBox,
                 uint16_t sampleBits = 16, bool quickTimeV1 = false)
{
    Bytes payload(8, 0);
    put16(payload, quickTimeV1 ? 1 : 0); // version
    put16(payload, 0);                   // revision
    put32(payload, 0);                   // vendor
    put16(payload, channels);
    put16(payload, sampleBits);
    put32(payload, 0);
    put32(payload, rate << 16);
    if (quickTimeV1)
    {
        payload.resize(payload.size() + 16, 0); // samples per packet, bytes per packet, frame, sample
    }
    return box(type, cat({payload, configBox}));
}

// An esds box. With no AudioSpecificConfig the decoder config has no DecoderSpecificInfo, as MP3 has none.
Bytes aacEsds(const Bytes &asc, uint8_t objectType = 0x40)
{
    Bytes decoderConfig = {objectType, 0x15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (!asc.empty())
    {
        decoderConfig.push_back(0x05);
        decoderConfig.push_back(static_cast<uint8_t>(asc.size()));
        decoderConfig.insert(decoderConfig.end(), asc.begin(), asc.end());
    }
    Bytes es = {0, 1, 0, 0x04, static_cast<uint8_t>(decoderConfig.size())};
    es.insert(es.end(), decoderConfig.begin(), decoderConfig.end());
    Bytes body = {0x03, static_cast<uint8_t>(es.size())};
    body.insert(body.end(), es.begin(), es.end());
    return box("esds", fullBoxPayload(body));
}

// One track. By default it holds a single 100-byte sample. PCM tracks use one sample per frame.
Bytes trak(const char (&handler)[5], const Bytes &sampleEntry, uint32_t sampleSize = 100, uint32_t sampleCount = 1,
           uint32_t samplesPerChunk = 1, uint32_t timescale = 1000, uint32_t sampleDelta = 1000)
{
    Bytes hdlrBody(4, 0);
    hdlrBody.insert(hdlrBody.end(), handler, handler + 4);
    hdlrBody.resize(hdlrBody.size() + 12, 0);
    Bytes mdhdBody(8, 0);
    put32(mdhdBody, timescale);
    put32(mdhdBody, timescale);
    put32(mdhdBody, 0);
    Bytes stsdBody;
    put32(stsdBody, 1);
    stsdBody.insert(stsdBody.end(), sampleEntry.begin(), sampleEntry.end());
    Bytes stts, stsz, stsc, stco;
    put32(stts, 1); put32(stts, sampleCount); put32(stts, sampleDelta);
    put32(stsz, sampleSize); put32(stsz, sampleCount);
    put32(stsc, 1); put32(stsc, 1); put32(stsc, samplesPerChunk); put32(stsc, 1);
    put32(stco, 1); put32(stco, 4096);
    const Bytes stbl = box("stbl", cat({box("stsd", fullBoxPayload(stsdBody)), box("stts", fullBoxPayload(stts)),
                                        box("stsz", fullBoxPayload(stsz)), box("stsc", fullBoxPayload(stsc)),
                                        box("stco", fullBoxPayload(stco))}));
    return box("trak", box("mdia", cat({box("mdhd", mdhdBody), box("hdlr", fullBoxPayload(hdlrBody)),
                                        box("minf", stbl)})));
}

Bytes mp4Of(std::initializer_list<Bytes> traks)
{
    Bytes moov;
    for (const auto &t : traks)
    {
        moov.insert(moov.end(), t.begin(), t.end());
    }
    return box("moov", moov);
}

bool parseMp4(const Bytes &file, reference_player::mp4_demuxer::Tracks &tracks, std::string &error)
{
    std::istringstream input(std::string(file.begin(), file.end()));
    return reference_player::mp4_demuxer::parse(input, tracks, error);
}

namespace mp4 = reference_player::mp4_demuxer;

const Bytes kAvcC = {1, 0x64, 0, 0x1f};
const Bytes kHvcC = {1, 1, 0x60, 0, 0, 0, 0x90};
const Bytes kVpcRecord = {2, 10, 0x80, 2, 2, 2};
const Bytes kAv1C = {0x81, 0, 0x0c, 0};
const Bytes kAsc = {0x12, 0x10};

Bytes videoTrack(const char (&entry)[5], const char (&config)[5], const Bytes &record, bool fullBox = false)
{
    return trak("vide", visualEntry(entry, box(config, fullBox ? fullBoxPayload(record) : record)));
}

Bytes aacTrack()
{
    return trak("soun", audioEntry("mp4a", 2, 48000, aacEsds(kAsc)));
}

// Video codecs: each sample entry reports its codec and hands over its configuration record.
void testMp4VideoCodecs()
{
    struct Case
    {
        Bytes file;
        mp4::Codec codec;
        Bytes data;
    };
    const std::vector<Case> cases = {
        {mp4Of({videoTrack("avc1", "avcC", kAvcC)}), mp4::Codec::H264, kAvcC},
        {mp4Of({videoTrack("avc3", "avcC", kAvcC)}), mp4::Codec::H264, kAvcC},
        {mp4Of({videoTrack("hvc1", "hvcC", kHvcC)}), mp4::Codec::H265Hvc1, kHvcC},
        {mp4Of({videoTrack("hev1", "hvcC", kHvcC)}), mp4::Codec::H265Hev1, kHvcC},
        {mp4Of({videoTrack("vp09", "vpcC", kVpcRecord, true)}), mp4::Codec::Vp9, kVpcRecord},
        {mp4Of({videoTrack("av01", "av1C", kAv1C)}), mp4::Codec::Av1, kAv1C},
    };
    for (const auto &c : cases)
    {
        mp4::Tracks tracks;
        std::string error;
        CHECK(parseMp4(c.file, tracks, error));
        CHECK(tracks.video.codec == c.codec);
        CHECK(tracks.video.codecData == c.data);
        CHECK(tracks.video.width == 640 && tracks.video.height == 360);
        CHECK(tracks.video.samples.size() == 1);
        CHECK(!tracks.hasAudio);
    }

    // A video entry that has no configuration box is rejected, not played with empty codec data.
    mp4::Tracks tracks;
    std::string error;
    CHECK(!parseMp4(mp4Of({trak("vide", visualEntry("hvc1", Bytes()))}), tracks, error));
    CHECK(!error.empty());
    CHECK(!parseMp4(mp4Of({videoTrack("vp09", "vpcC", Bytes{0, 0, 0, 0}, false)}), tracks, error));
}

// Audio codecs. Opus gets an OpusHead built from dOps; E-AC-3 gets its channels from dec3.
void testMp4AudioCodecs()
{
    const Bytes video = videoTrack("avc1", "avcC", kAvcC);

    mp4::Tracks aac;
    std::string error;
    CHECK(parseMp4(mp4Of({video, aacTrack()}), aac, error));
    CHECK(aac.hasAudio && aac.audio.codec == mp4::Codec::Aac);
    CHECK(aac.audio.codecData == kAsc);
    CHECK(aac.audio.channels == 2 && aac.audio.sampleRate == 48000);

    // dOps: version 0, 6 channels, pre-skip 312, input rate 44100, gain -1, mapping family 1,
    // 4 streams, 2 coupled, mapping table 0 4 1 2 3 5.
    Bytes dops = {0, 6};
    put16(dops, 312);
    put32(dops, 44100);
    put16(dops, 0xFFFF);
    const Bytes mapping = {1, 4, 2, 0, 4, 1, 2, 3, 5};
    dops.insert(dops.end(), mapping.begin(), mapping.end());
    mp4::Tracks opus;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("Opus", 2, 48000, box("dOps", dops)))}), opus, error));
    CHECK(opus.hasAudio && opus.audio.codec == mp4::Codec::Opus);
    CHECK(opus.audio.channels == 6 && opus.audio.sampleRate == 48000);
    const Bytes expectedHead = {'O', 'p', 'u', 's', 'H', 'e', 'a', 'd', 1, 6, 0x38, 0x01, 0x44, 0xAC, 0, 0, 0xFF, 0xFF,
                                1, 4, 2, 0, 4, 1, 2, 3, 5};
    CHECK(opus.audio.codecData == expectedHead);

    // Mapping family 0 carries no table.
    Bytes stereo = {0, 2};
    put16(stereo, 0);
    put32(stereo, 48000);
    put16(stereo, 0);
    stereo.push_back(0);
    mp4::Tracks opusStereo;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("Opus", 2, 48000, box("dOps", stereo)))}), opusStereo,
                   error));
    CHECK(opusStereo.audio.codecData.size() == 19 && opusStereo.audio.channels == 2);

    // dec3: data rate 768, one independent substream, 48 kHz, acmod 7 (3/2) with LFE = 6 channels.
    const Bytes dec3 = {0x30, 0x00, 0x10, 0x0F, 0x00};
    mp4::Tracks eac3;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("ec-3", 2, 48000, box("dec3", dec3)))}), eac3, error));
    CHECK(eac3.hasAudio && eac3.audio.codec == mp4::Codec::Eac3);
    CHECK(eac3.audio.channels == 6 && eac3.audio.sampleRate == 48000);
    CHECK(eac3.audio.codecData == dec3);

    // Malformed configuration boxes drop the audio track, and the video still plays.
    mp4::Tracks broken;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("Opus", 2, 48000, box("dOps", Bytes{0, 2})))}), broken,
                   error));
    CHECK(!broken.hasAudio);
    mp4::Tracks noConfig;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("ec-3", 2, 48000, Bytes()))}), noConfig, error));
    CHECK(!noConfig.hasAudio);
}

// MP3 in MP4 is an mp4a entry with an MPEG audio object type, or a legacy ".mp3" entry. AAC must stay AAC.
void testMp4Mp3Audio()
{
    const Bytes video = videoTrack("avc1", "avcC", kAvcC);
    std::string error;

    for (const uint8_t objectType : {0x6B, 0x69})
    {
        mp4::Tracks tracks;
        CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("mp4a", 2, 44100, aacEsds(Bytes(), objectType)))}),
                       tracks, error));
        CHECK(tracks.hasAudio && tracks.audio.codec == mp4::Codec::Mp3);
        CHECK(tracks.audio.codecData.empty());
        CHECK(tracks.audio.channels == 2 && tracks.audio.sampleRate == 44100);
        CHECK(tracks.audio.samples.size() == 1);
    }

    mp4::Tracks legacy;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry(".mp3", 1, 32000, Bytes()))}), legacy, error));
    CHECK(legacy.hasAudio && legacy.audio.codec == mp4::Codec::Mp3 && legacy.audio.channels == 1);

    // An mp4a entry with an AudioSpecificConfig is still AAC.
    mp4::Tracks aac;
    CHECK(parseMp4(mp4Of({video, aacTrack()}), aac, error));
    CHECK(aac.audio.codec == mp4::Codec::Aac && aac.audio.codecData == kAsc);

    // An mp4a entry that is neither MP3 nor has a config is dropped, as before.
    mp4::Tracks neither;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("mp4a", 2, 44100, aacEsds(Bytes(), 0x40)))}), neither,
                   error));
    CHECK(!neither.hasAudio);
}

// FLAC: dfLa becomes the stream header list, and STREAMINFO overrides the sample entry's channels and rate.
void testMp4FlacAudio()
{
    const Bytes video = videoTrack("avc1", "avcC", kAvcC);
    std::string error;

    // STREAMINFO: block sizes, frame sizes, 44100 Hz, 2 channels, 16 bits, 0 total samples, zero MD5.
    Bytes info = {0x10, 0, 0x10, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0xC4, 0x42, 0xF0, 0, 0, 0, 0};
    info.resize(34, 0);
    const Bytes comment = {1, 2, 3, 4};
    const Bytes streamInfoBlockLast = cat({Bytes{0x80, 0, 0, 34}, info});
    const Bytes streamInfoBlock = cat({Bytes{0x00, 0, 0, 34}, info});
    const Bytes commentBlock = cat({Bytes{0x84, 0, 0, 4}, comment});

    mp4::Tracks single;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("fLaC", 6, 0, box("dfLa", fullBoxPayload(streamInfoBlockLast))))}),
                   single, error));
    CHECK(single.hasAudio && single.audio.codec == mp4::Codec::Flac);
    CHECK(single.audio.channels == 2 && single.audio.sampleRate == 44100);
    CHECK(single.audio.streamHeader.size() == 2);
    if (single.audio.streamHeader.size() == 2)
    {
        CHECK(single.audio.streamHeader[0] == (Bytes{'f', 'L', 'a', 'C'}));
        CHECK(single.audio.streamHeader[1] == streamInfoBlockLast);
    }
    CHECK(single.audio.codecData.empty());

    mp4::Tracks multi;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("fLaC", 2, 44100,
                                                         box("dfLa", fullBoxPayload(cat({streamInfoBlock, commentBlock})))))}),
                   multi, error));
    CHECK(multi.audio.streamHeader.size() == 3);
    if (multi.audio.streamHeader.size() == 3)
    {
        CHECK(multi.audio.streamHeader[1] == streamInfoBlock && multi.audio.streamHeader[2] == commentBlock);
    }

    // No dfLa, a first block that is not STREAMINFO, or a block that overruns the box: no audio, video plays.
    const Bytes wrongFirst = cat({Bytes{0x84, 0, 0, 34}, info});
    const Bytes overrun = cat({Bytes{0x80, 0, 0, 60}, info});
    for (const Bytes &config : {Bytes(), box("dfLa", fullBoxPayload(wrongFirst)), box("dfLa", fullBoxPayload(overrun))})
    {
        mp4::Tracks broken;
        CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("fLaC", 2, 44100, config))}), broken, error));
        CHECK(!broken.hasAudio);
    }
}

// PCM: the sample format comes from the entry type, and one-frame MP4 samples are grouped into larger ones.
void testMp4PcmAudio()
{
    const Bytes video = videoTrack("avc1", "avcC", kAvcC);
    std::string error;
    constexpr uint32_t kFrames = 3000;

    struct Case
    {
        Bytes entry;
        mp4::PcmFormat format;
        uint32_t frameBytes;
    };
    const Bytes pcmcLe16 = box("pcmC", Bytes{0, 0, 0, 0, 1, 16});
    const Bytes pcmcBe24 = box("pcmC", Bytes{0, 0, 0, 0, 0, 24});
    const std::vector<Case> cases = {
        {audioEntry("sowt", 2, 48000, Bytes(), 16), mp4::PcmFormat::S16LE, 4},
        {audioEntry("twos", 2, 48000, Bytes(), 16), mp4::PcmFormat::S16BE, 4},
        {audioEntry("twos", 2, 48000, Bytes(), 24), mp4::PcmFormat::S24BE, 6},
        {audioEntry("twos", 2, 48000, Bytes(), 32), mp4::PcmFormat::S32BE, 8},
        {audioEntry("twos", 1, 48000, Bytes(), 8), mp4::PcmFormat::S8, 1},
        {audioEntry("ipcm", 2, 48000, pcmcLe16, 16), mp4::PcmFormat::S16LE, 4},
        {audioEntry("ipcm", 2, 48000, pcmcBe24, 24), mp4::PcmFormat::S24BE, 6},
        {audioEntry("sowt", 2, 48000, Bytes(), 16, true), mp4::PcmFormat::S16LE, 4}, // QuickTime version 1
    };
    for (const auto &c : cases)
    {
        mp4::Tracks tracks;
        CHECK(parseMp4(mp4Of({video, trak("soun", c.entry, c.frameBytes, kFrames, kFrames, 48000, 1)}), tracks,
                       error));
        CHECK(tracks.hasAudio && tracks.audio.codec == mp4::Codec::Pcm && tracks.audio.pcmFormat == c.format);
        CHECK(tracks.audio.codecData.empty());

        // Samples are whole frames of at most 4096 bytes (at least one frame), cover every frame once, are
        // contiguous from the chunk offset, and time runs on with no gaps.
        uint64_t bytes = 0;
        int64_t expectedPts = 0;
        bool shapeOk = !tracks.audio.samples.empty();
        for (const auto &sample : tracks.audio.samples)
        {
            shapeOk = shapeOk && sample.size % c.frameBytes == 0 && sample.size > 0 &&
                      (sample.size <= 4096 || sample.size == c.frameBytes) && sample.offset == 4096 + bytes &&
                      sample.ptsNs == expectedPts;
            bytes += sample.size;
            expectedPts += sample.durationNs;
        }
        CHECK(shapeOk);
        CHECK(bytes == static_cast<uint64_t>(kFrames) * c.frameBytes);
        CHECK(expectedPts == 62500000); // 3000 frames at 48 kHz
        CHECK(tracks.audio.samples.size() < 40); // not one sample per frame
    }

    // A frame size in stsz that disagrees with the entry, an unsupported size, and a missing pcmC are dropped.
    mp4::Tracks wrongSize;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("sowt", 2, 48000, Bytes()), 100, 10, 10, 48000, 1)}),
                   wrongSize, error));
    CHECK(!wrongSize.hasAudio);
    mp4::Tracks odd;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("twos", 2, 48000, Bytes(), 12), 3, 10, 10, 48000, 1)}), odd,
                   error));
    CHECK(!odd.hasAudio);
    mp4::Tracks noPcmc;
    CHECK(parseMp4(mp4Of({video, trak("soun", audioEntry("ipcm", 2, 48000, Bytes()), 4, 10, 10, 48000, 1)}), noPcmc,
                   error));
    CHECK(!noPcmc.hasAudio);
}

// The codecs that worked before keep their tracks when a file also holds a newer codec.
void testMp4PrefersLegacyCodecs()
{
    const Bytes dec3 = {0x30, 0x00, 0x10, 0x0F, 0x00};
    const Bytes eac3 = trak("soun", audioEntry("ec-3", 6, 48000, box("dec3", dec3)));
    const Bytes hevc = videoTrack("hvc1", "hvcC", kHvcC);
    const Bytes avc = videoTrack("avc1", "avcC", kAvcC);

    for (const bool newerFirst : {true, false})
    {
        mp4::Tracks tracks;
        std::string error;
        const Bytes file = newerFirst ? mp4Of({hevc, avc, eac3, aacTrack()}) : mp4Of({avc, hevc, aacTrack(), eac3});
        CHECK(parseMp4(file, tracks, error));
        CHECK(tracks.video.codec == mp4::Codec::H264);
        CHECK(tracks.audio.codec == mp4::Codec::Aac);
        CHECK(tracks.audio.codecData == kAsc);
    }

    // Without H.264 or AAC the newer codecs are used.
    mp4::Tracks tracks;
    std::string error;
    CHECK(parseMp4(mp4Of({hevc, eac3}), tracks, error));
    CHECK(tracks.video.codec == mp4::Codec::H265Hvc1 && tracks.audio.codec == mp4::Codec::Eac3);
}

// Runs one parsed track model through SourceManager::attach() against the mock server.
std::vector<firebolt::rialto::MockAttachedSource> attachModel(const rialto_refplayer::demux::TrackModel &model)
{
    class ModelDemuxer final : public rialto_refplayer::demux::IDemuxer
    {
    public:
        explicit ModelDemuxer(const rialto_refplayer::demux::TrackModel &m) : m_model(m) {}
        bool parse(std::istream &, rialto_refplayer::demux::TrackModel &model, std::string &) override
        {
            model = m_model;
            return true;
        }

    private:
        rialto_refplayer::demux::TrackModel m_model;
    };

    {
        std::lock_guard<std::mutex> lock(firebolt::rialto::g_mockAttachedMutex);
        firebolt::rialto::g_mockAttachedSources.clear();
    }
    rr::SourceManager::Dependencies deps;
    deps.createDemuxer = [&model](const std::string &, const std::string &)
        -> std::unique_ptr<rialto_refplayer::demux::IDemuxer> { return std::make_unique<ModelDemuxer>(model); };
    deps.openStream = [](const std::string &) -> std::unique_ptr<std::istream> {
        return std::make_unique<std::istringstream>(std::string());
    };
    rr::SourceManager manager(std::make_shared<rr::Logger>([](const std::string &) {}), deps);
    std::string error;
    CHECK(manager.open("file:///fake/codec.mp4", "video/mp4", error));
    firebolt::rialto::MockMediaPipeline pipeline{std::weak_ptr<firebolt::rialto::IMediaPipelineClient>()};
    CHECK(manager.attach(pipeline));
    std::lock_guard<std::mutex> lock(firebolt::rialto::g_mockAttachedMutex);
    return firebolt::rialto::g_mockAttachedSources;
}

rialto_refplayer::demux::TrackModel modelOf(rialto_refplayer::demux::Codec videoCodec, const Bytes &videoData,
                                            bool hasAudio = false,
                                            rialto_refplayer::demux::Codec audioCodec =
                                                rialto_refplayer::demux::Codec::Unknown,
                                            const Bytes &audioData = Bytes())
{
    rialto_refplayer::demux::TrackModel model;
    model.video.codec = videoCodec;
    model.video.codecData = videoData;
    model.video.width = 640;
    model.video.height = 360;
    model.video.samples.push_back({0, 16, 0, 33000000});
    model.hasAudio = hasAudio;
    if (hasAudio)
    {
        model.audio.codec = audioCodec;
        model.audio.codecData = audioData;
        model.audio.channels = 2;
        model.audio.sampleRate = 48000;
        model.audio.samples.push_back({16, 16, 0, 21000000});
    }
    return model;
}

// What the server receives for each codec. A track with no codec set (the old behaviour of the
// track model) and an explicit H.264/AAC track must both give the original H.264 and AAC sources.
void testSourceDescriptionPerCodec()
{
    namespace rialto = firebolt::rialto;
    using rialto_refplayer::demux::Codec;

    for (const Codec videoCodec : {Codec::Unknown, Codec::H264})
    {
        for (const Codec audioCodec : {Codec::Unknown, Codec::Aac})
        {
            const auto sources = attachModel(modelOf(videoCodec, kAvcC, true, audioCodec, kAsc));
            CHECK(sources.size() == 2);
            if (sources.size() != 2)
            {
                continue;
            }
            CHECK(sources[0].type == rialto::MediaSourceType::VIDEO && sources[0].mimeType == "video/h264");
            CHECK(sources[0].format == rialto::StreamFormat::AVC);
            CHECK(sources[0].alignment == rialto::SegmentAlignment::AU && sources[0].capsCodecData == kAvcC);
            CHECK(sources[1].type == rialto::MediaSourceType::AUDIO && sources[1].mimeType == "audio/mp4");
            CHECK(sources[1].format == rialto::StreamFormat::RAW);
            CHECK(sources[1].alignment == rialto::SegmentAlignment::AU);
            CHECK(sources[1].capsCodecData == kAsc && sources[1].audioConfigData == kAsc);
            CHECK(sources[1].channels == 2 && sources[1].sampleRate == 48000);
        }
    }

    const auto hvc1 = attachModel(modelOf(Codec::H265Hvc1, kHvcC));
    CHECK(hvc1.size() == 1);
    if (hvc1.size() == 1)
    {
        CHECK(hvc1[0].mimeType == "video/h265" && hvc1[0].format == rialto::StreamFormat::HVC1);
        CHECK(hvc1[0].alignment == rialto::SegmentAlignment::AU && hvc1[0].capsCodecData == kHvcC);
    }
    const auto hev1 = attachModel(modelOf(Codec::H265Hev1, kHvcC));
    CHECK(hev1.size() == 1 && hev1[0].format == rialto::StreamFormat::HEV1 && hev1[0].capsCodecData == kHvcC);

    const auto vp9 = attachModel(modelOf(Codec::Vp9, kVpcRecord));
    CHECK(vp9.size() == 1);
    if (vp9.size() == 1)
    {
        CHECK(vp9[0].mimeType == "video/x-vp9" && vp9[0].format == rialto::StreamFormat::UNDEFINED);
        CHECK(vp9[0].alignment == rialto::SegmentAlignment::UNDEFINED && vp9[0].capsCodecData == kVpcRecord);
    }
    const auto av1 = attachModel(modelOf(Codec::Av1, kAv1C));
    CHECK(av1.size() == 1 && av1[0].mimeType == "video/x-av1" && av1[0].capsCodecData == kAv1C);

    // Opus: OpusHead travels in the audio config, which is where the server reads it.
    const Bytes head = {'O', 'p', 'u', 's', 'H', 'e', 'a', 'd', 1, 2};
    const auto opus = attachModel(modelOf(Codec::H264, kAvcC, true, Codec::Opus, head));
    CHECK(opus.size() == 2);
    if (opus.size() == 2)
    {
        CHECK(opus[1].mimeType == "audio/x-opus" && opus[1].format == rialto::StreamFormat::UNDEFINED);
        CHECK(opus[1].audioConfigData == head && opus[1].capsCodecData.empty());
        CHECK(opus[1].channels == 2 && opus[1].sampleRate == 48000);
    }

    const Bytes dec3 = {0x30, 0x00, 0x10, 0x0F, 0x00};
    const auto eac3 = attachModel(modelOf(Codec::H264, kAvcC, true, Codec::Eac3, dec3));
    CHECK(eac3.size() == 2);
    if (eac3.size() == 2)
    {
        CHECK(eac3[1].mimeType == "audio/x-eac3" && eac3[1].format == rialto::StreamFormat::UNDEFINED);
        CHECK(eac3[1].audioConfigData.empty() && eac3[1].capsCodecData.empty());
        CHECK(eac3[1].channels == 2 && eac3[1].sampleRate == 48000);
    }

    const auto mp3 = attachModel(modelOf(Codec::H264, kAvcC, true, Codec::Mp3));
    CHECK(mp3.size() == 2);
    if (mp3.size() == 2)
    {
        CHECK(mp3[1].mimeType == "audio/mp3" && mp3[1].format == rialto::StreamFormat::UNDEFINED);
        CHECK(mp3[1].audioConfigData.empty() && mp3[1].capsCodecData.empty() && mp3[1].streamHeader.empty());
        CHECK(!mp3[1].framed.has_value() && !mp3[1].pcmFormat.has_value());
        CHECK(mp3[1].channels == 2 && mp3[1].sampleRate == 48000);
    }

    auto flacModel = modelOf(Codec::H264, kAvcC, true, Codec::Flac);
    flacModel.audio.streamHeader = {{'f', 'L', 'a', 'C'}, {0x80, 0, 0, 1, 7}};
    const auto flac = attachModel(flacModel);
    CHECK(flac.size() == 2);
    if (flac.size() == 2)
    {
        CHECK(flac[1].mimeType == "audio/x-flac" && flac[1].streamHeader == flacModel.audio.streamHeader);
        CHECK(flac[1].framed.has_value() && *flac[1].framed);
        CHECK(flac[1].capsCodecData.empty() && !flac[1].pcmFormat.has_value());
    }

    auto pcmModel = modelOf(Codec::H264, kAvcC, true, Codec::Pcm);
    pcmModel.audio.pcmFormat = rialto_refplayer::demux::PcmFormat::S24BE;
    const auto pcm = attachModel(pcmModel);
    CHECK(pcm.size() == 2);
    if (pcm.size() == 2)
    {
        CHECK(pcm[1].mimeType == "audio/x-raw" && pcm[1].format == rialto::StreamFormat::UNDEFINED);
        CHECK(pcm[1].pcmFormat.has_value() && *pcm[1].pcmFormat == rialto::Format::S24BE);
        CHECK(pcm[1].layout.has_value() && *pcm[1].layout == rialto::Layout::INTERLEAVED);
        CHECK(!pcm[1].channelMask.has_value() && !pcm[1].framed.has_value()); // stereo needs no channel mask
        CHECK(pcm[1].channels == 2 && pcm[1].sampleRate == 48000);
    }
    pcmModel.audio.channels = 6;
    pcmModel.audio.pcmFormat = rialto_refplayer::demux::PcmFormat::S16LE;
    const auto pcm6 = attachModel(pcmModel);
    CHECK(pcm6.size() == 2 && pcm6[1].channels == 6 && pcm6[1].channelMask.has_value() && *pcm6[1].channelMask == 0);
    CHECK(pcm6.size() == 2 && pcm6[1].pcmFormat.has_value() && *pcm6[1].pcmFormat == rialto::Format::S16LE);
}

int main()
{
    testMp4VideoCodecs();
    testMp4AudioCodecs();
    testMp4Mp3Audio();
    testMp4FlacAudio();
    testMp4PcmAudio();
    testMp4PrefersLegacyCodecs();
    testSourceDescriptionPerCodec();
    testPlayPauseSeekStopReleases();
    testEndOfStreamReachesComplete();
    testReloadWithoutStop();
    testUnsupportedSourceFails();
    testIllegalCommandsRaiseEvents();
    testUnbackedCommandsReportNotSupported();
    testDestroyWhilePlaying();
    testRapidLoadStop();
    testStopThenLoadDoesNotWaitForTimeout();
    testRunOnPipelineUsesLivePipeline();

    std::printf("refplayer host test: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
