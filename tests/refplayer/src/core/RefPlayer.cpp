#include "RefPlayer.h"

#include "RialtoNames.h"

#include "IDemuxer.h"

#include <chrono>

namespace rialto_refplayer
{
namespace
{

constexpr const char *kLocalMp4Mime = "video/mp4";
constexpr std::chrono::milliseconds kStopTimeout{2000};
constexpr int64_t kNsPerMs = 1000000;

RefPlayer::Dependencies defaultDependencies()
{
    RefPlayer::Dependencies dependencies;
    dependencies.source.createDemuxer = [](const std::string &url, const std::string &mimeType) {
        return demux::createDemuxer(url, mimeType);
    };
    dependencies.source.openStream = [](const std::string &url) { return openLocalFile(url); };
    return dependencies;
}

// Only the states in which a command can act. Everything else is rejected by the worker.
bool canPlay(PlayerState state)
{
    switch (state)
    {
    case PlayerState::Prepared:
    case PlayerState::Paused:
    case PlayerState::Buffering:
    case PlayerState::Seeking:
    case PlayerState::Complete:
        return true;
    default:
        return false;
    }
}

bool canPause(PlayerState state)
{
    return state == PlayerState::Playing || state == PlayerState::Buffering;
}

bool canSeek(PlayerState state)
{
    switch (state)
    {
    case PlayerState::Prepared:
    case PlayerState::Playing:
    case PlayerState::Paused:
    case PlayerState::Buffering:
    case PlayerState::Seeking:
    case PlayerState::Complete:
        return true;
    default:
        return false;
    }
}

} // namespace

RefPlayer::RefPlayer() : RefPlayer(defaultDependencies())
{
}

RefPlayer::RefPlayer(Dependencies dependencies)
    : m_logger(std::make_shared<Logger>()), m_bus(std::make_shared<EventBus>())
{
    m_state = std::make_shared<PlayerStateMachine>(m_bus, m_logger);
    m_position = std::make_shared<PositionTracker>(m_bus, m_options.progressIntervalMs);
    m_feeder = std::make_shared<MediaFeeder>(m_logger);
    m_sources = std::make_shared<SourceManager>(m_logger, std::move(dependencies.source));
    m_session = std::make_unique<RialtoSession>(m_logger, [this](firebolt::rialto::ApplicationState state) {
        onAppState(state);
    });
}

RefPlayer::~RefPlayer()
{
    destroy();
}

Result RefPlayer::initialize(const Options &options)
{
    if (m_destroyed)
    {
        return Result::InvalidState;
    }
    if (m_initialized)
    {
        return rejectCall("initialize");
    }

    {
        std::lock_guard<std::mutex> lock(m_optionsMutex);
        m_options = options;
    }
    m_logger->setLevel(options.verboseLogging ? Logger::Level::Debug : Logger::Level::Info);
    m_logger->attachClientLog();
    m_position->setProgressInterval(options.progressIntervalMs);
    m_logger->info("Initializing Rialto reference player");

    const Result connected = m_session->connect();
    if (connected != Result::Ok)
    {
        return connected;
    }
    if (!ensurePipeline())
    {
        return Result::RialtoError;
    }
    m_initialized = true;
    return Result::Ok;
}

Result RefPlayer::updateConfig(const Options &options)
{
    if (!alive() || !m_initialized)
    {
        return Result::InvalidState;
    }
    {
        std::lock_guard<std::mutex> lock(m_optionsMutex);
        m_options = options;
    }
    m_position->setProgressInterval(options.progressIntervalMs);
    return Result::Ok;
}

void RefPlayer::destroy()
{
    if (m_destroyed.exchange(true))
    {
        return;
    }
    // Stop delivering events first; the glue is tearing down and may not be listening.
    m_bus->setClient({});
    m_worker.shutdown();

    if (m_sessionLoaded && m_session->hasPipeline())
    {
        m_session->pipeline()->stop();
    }
    teardown();
    m_session->disconnect();
}

Result RefPlayer::load(const std::string &url, bool autoplay)
{
    if (!ready())
    {
        return rejectCall("load");
    }
    // INITIALIZING is set here only when the worker is idle. While a stop is still in
    // progress, the worker sets it at the start of the load, so the stop's STOPPED callback
    // is not dropped by the Initializing guard.
    const bool workerIdle = m_worker.idle();
    m_state->resetForLoad();
    m_position->reset();
    if (workerIdle)
    {
        m_state->setState(PlayerState::Initializing, "load requested");
    }
    if (!m_worker.post([this, url, autoplay] { doLoad(url, autoplay); }))
    {
        return rejectCall("load");
    }
    return Result::Ok;
}

Result RefPlayer::play()
{
    if (!ready())
    {
        return rejectCall("play");
    }
    return m_worker.post([this] { doPlay(); }) ? Result::Ok : rejectCall("play");
}

Result RefPlayer::pause()
{
    if (!ready())
    {
        return rejectCall("pause");
    }
    return m_worker.post([this] { doPause(); }) ? Result::Ok : rejectCall("pause");
}

Result RefPlayer::stop()
{
    if (!alive() || !m_initialized)
    {
        return rejectCall("stop");
    }
    return m_worker.post([this] { doStop(); }) ? Result::Ok : rejectCall("stop");
}

Result RefPlayer::seek(int64_t positionMs)
{
    if (!ready())
    {
        return rejectCall("seek");
    }
    return m_worker.post([this, positionMs] { doSeek(positionMs); }) ? Result::Ok : rejectCall("seek");
}

// Phase 2 and later: the library does not back these yet.
Result RefPlayer::setPlaybackRate(double rate)
{
    (void)rate;
    return Result::NotSupported;
}

Result RefPlayer::setVolume(double volume)
{
    (void)volume;
    return Result::NotSupported;
}

Result RefPlayer::setVideoWindow(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    return Result::NotSupported;
}

Result RefPlayer::setVideoMute(bool mute)
{
    (void)mute;
    return Result::NotSupported;
}

Result RefPlayer::setTextTrack(int index)
{
    (void)index;
    return Result::NotSupported;
}

Result RefPlayer::setClosedCaptions(bool enabled)
{
    (void)enabled;
    return Result::NotSupported;
}

int64_t RefPlayer::positionMs() const
{
    return m_position->positionMs();
}

int64_t RefPlayer::durationMs() const
{
    return m_position->durationMs();
}

PlayerState RefPlayer::state() const
{
    return m_state->state();
}

std::optional<double> RefPlayer::volume() const
{
    return std::nullopt;
}

std::vector<double> RefPlayer::supportedSpeeds() const
{
    return {1.0};
}

std::optional<std::string> RefPlayer::textTracksJson() const
{
    return std::nullopt;
}

std::optional<int> RefPlayer::textTrack() const
{
    return std::nullopt;
}

void RefPlayer::setClient(std::weak_ptr<IRefPlayerClient> client)
{
    m_bus->setClient(std::move(client));
}

bool RefPlayer::ready() const
{
    return alive() && m_initialized && !m_appInactive;
}

bool RefPlayer::alive() const
{
    return !m_destroyed;
}

Result RefPlayer::rejectCall(const char *name) const
{
    m_logger->info(std::string(name) + "() rejected: not ready (state " + toString(m_state->state()) + ")");
    return Result::InvalidState;
}

void RefPlayer::onAppState(firebolt::rialto::ApplicationState state)
{
    const bool inactive = state == firebolt::rialto::ApplicationState::INACTIVE;
    if (m_appInactive.exchange(inactive) != inactive)
    {
        m_logger->warning("application state changed to " + toString(state) +
                          (inactive ? "; control calls are blocked" : ""));
    }
}

bool RefPlayer::ensurePipeline()
{
    Options options;
    {
        std::lock_guard<std::mutex> lock(m_optionsMutex);
        options = m_options;
    }
    const firebolt::rialto::VideoRequirements requirements{options.maxVideoWidth, options.maxVideoHeight};

    auto callbacks = std::make_shared<PipelineCallbacks>(m_bus, m_state, m_position, m_feeder, m_logger);
    if (m_session->createPipeline(callbacks, requirements) != Result::Ok)
    {
        return false;
    }
    m_callbacks = std::move(callbacks);
    return true;
}

void RefPlayer::doLoad(const std::string &url, bool autoplay)
{
    // Before teardown, so callbacks from the previous pipeline are dropped from here on.
    m_state->setState(PlayerState::Initializing, "load started");
    if (m_sessionLoaded)
    {
        teardown();
    }

    std::string error;
    if (!m_sources->open(url, kLocalMp4Mime, error))
    {
        failLoad("cannot load " + url + ": " + error);
        return;
    }
    if (!m_session->hasPipeline() && !ensurePipeline())
    {
        failLoad("could not create media pipeline");
        return;
    }

    firebolt::rialto::IMediaPipeline &pipeline = *m_session->pipeline();
    m_callbacks->markLoadStart();
    if (!pipeline.load(firebolt::rialto::MediaType::MSE, kLocalMp4Mime, url, false))
    {
        failLoad("IMediaPipeline::load() returned false");
        return;
    }
    m_state->setState(PlayerState::Initialized, "pipeline loaded");

    m_state->setState(PlayerState::Preparing, "attaching sources");
    if (!m_sources->attach(pipeline))
    {
        failLoad("attaching sources failed");
        return;
    }
    // The feeder starts before allSourcesAttached(), so the first needData cannot be missed.
    std::optional<FeederFaultControl> feederFaultControl;
    {
        std::lock_guard<std::mutex> lock(m_optionsMutex);
        feederFaultControl = m_options.feederFaultControl;
    }
    m_feeder->start(pipeline, m_sources->tracks(), *m_sources->input(), m_sources->videoSourceId(),
                    m_sources->audioSourceId(), feederFaultControl);
    m_sessionLoaded = true;

    if (!pipeline.allSourcesAttached())
    {
        failLoad("IMediaPipeline::allSourcesAttached() returned false");
        return;
    }
    m_state->setState(PlayerState::Prepared, "all sources attached");

    if (autoplay)
    {
        doPlay();
    }
}

void RefPlayer::doPlay()
{
    const PlayerState current = m_state->state();
    if (current == PlayerState::Playing)
    {
        return;
    }
    if (!canPlay(current) || !m_session->hasPipeline())
    {
        m_state->reject(std::string("play() rejected in state ") + toString(current));
        return;
    }
    bool async = true;
    if (!m_session->pipeline()->play(async))
    {
        m_state->fail("IMediaPipeline::play() returned false");
        return;
    }
    m_logger->info(std::string("play() invoked; async=") + (async ? "true" : "false"));
}

void RefPlayer::doPause()
{
    const PlayerState current = m_state->state();
    if (current == PlayerState::Paused)
    {
        return;
    }
    if (!canPause(current) || !m_session->hasPipeline())
    {
        m_state->reject(std::string("pause() rejected in state ") + toString(current));
        return;
    }
    if (!m_session->pipeline()->pause())
    {
        m_state->fail("IMediaPipeline::pause() returned false");
        return;
    }
    m_logger->info("pause() invoked");
}

void RefPlayer::doSeek(int64_t positionMs)
{
    const PlayerState current = m_state->state();
    if (!canSeek(current) || !m_session->hasPipeline())
    {
        m_state->reject(std::string("seek() rejected in state ") + toString(current));
        return;
    }
    if (!m_session->pipeline()->setPosition(positionMs * kNsPerMs))
    {
        m_state->fail("IMediaPipeline::setPosition() returned false");
        return;
    }
    m_logger->info("seek() requested to " + std::to_string(positionMs) + " ms");
}

void RefPlayer::doStop()
{
    if (!m_sessionLoaded)
    {
        return; // nothing loaded: stop is a no-op
    }

    m_state->setState(PlayerState::Stopping, "stop requested");
    if (!m_session->pipeline()->stop())
    {
        m_state->reject("IMediaPipeline::stop() returned false; releasing anyway");
    }
    else if (!m_state->waitForState(PlayerState::Stopped, kStopTimeout))
    {
        m_logger->warning("STOPPED not received within the timeout; releasing anyway");
        m_state->setState(PlayerState::Stopped, "stop timed out");
    }

    teardown();
    m_state->setState(PlayerState::Released, "pipeline released");
    m_state->setState(PlayerState::Idle, "released");
}

// Order matters. The feeder must be joined before any source or sample is released,
// and the pipeline must go before the callbacks that it holds.
void RefPlayer::teardown()
{
    m_feeder->stop();
    if (m_callbacks)
    {
        m_callbacks->deactivate();
    }
    m_sources->removeAll(m_session->pipeline());
    m_session->releasePipeline();
    m_callbacks.reset();
    m_sessionLoaded = false;
}

void RefPlayer::failLoad(const std::string &description)
{
    teardown();
    m_state->fail(description);
}

std::shared_ptr<IRefPlayer> createRefPlayer()
{
    return std::make_shared<RefPlayer>();
}

} // namespace rialto_refplayer
