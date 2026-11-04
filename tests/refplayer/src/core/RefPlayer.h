#pragma once

#include "CommandWorker.h"
#include "EventBus.h"
#include "Logger.h"
#include "MediaFeeder.h"
#include "PipelineCallbacks.h"
#include "PlayerStateMachine.h"
#include "PositionTracker.h"
#include "RialtoSession.h"
#include "SourceManager.h"

#include "rialto_refplayer/IRefPlayer.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

namespace rialto_refplayer
{

// The library's only IRefPlayer. It validates each control call, enqueues the work on the
// command worker, and returns. The worker executes commands in order against the actual
// state. It is the only thread that drives Rialto control calls, and the only thread that
// tears the session down.
//
// Internal header: the CLI and the host tests construct RefPlayer with injected
// dependencies. Applications use createRefPlayer().
class RefPlayer final : public IRefPlayer
{
public:
    struct Dependencies
    {
        SourceManager::Dependencies source;
    };

    RefPlayer();
    explicit RefPlayer(Dependencies dependencies);
    ~RefPlayer() override;

    RefPlayer(const RefPlayer &) = delete;
    RefPlayer &operator=(const RefPlayer &) = delete;

    Result initialize(const Options &options) override;
    Result updateConfig(const Options &options) override;
    void destroy() override;

    Result load(const std::string &url, bool autoplay) override;
    Result play() override;
    Result pause() override;
    Result stop() override;
    Result seek(int64_t positionMs) override;
    Result setPlaybackRate(double rate) override;
    Result setVolume(double volume) override;
    Result setVideoWindow(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override;
    Result setVideoMute(bool mute) override;
    Result setTextTrack(int index) override;
    Result setClosedCaptions(bool enabled) override;

    int64_t positionMs() const override;
    int64_t durationMs() const override;
    PlayerState state() const override;
    std::optional<double> volume() const override;
    std::vector<double> supportedSpeeds() const override;
    std::optional<std::string> textTracksJson() const override;
    std::optional<int> textTrack() const override;

    void setClient(std::weak_ptr<IRefPlayerClient> client) override;

private:
    bool ready() const;
    bool alive() const;
    Result rejectCall(const char *name) const;
    void onAppState(firebolt::rialto::ApplicationState state);
    bool ensurePipeline();

    // Worker side. Each runs on the command worker thread only.
    void doLoad(const std::string &url, bool autoplay);
    void doPlay();
    void doPause();
    void doSeek(int64_t positionMs);
    void doStop();
    void teardown();
    void failLoad(const std::string &description);

    std::shared_ptr<Logger> m_logger;
    std::shared_ptr<EventBus> m_bus;
    std::shared_ptr<PlayerStateMachine> m_state;
    std::shared_ptr<PositionTracker> m_position;
    std::shared_ptr<MediaFeeder> m_feeder;
    std::shared_ptr<SourceManager> m_sources;
    std::shared_ptr<PipelineCallbacks> m_callbacks;
    std::unique_ptr<RialtoSession> m_session;

    mutable std::mutex m_optionsMutex;
    Options m_options;

    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_destroyed{false};
    std::atomic<bool> m_appInactive{false};
    bool m_sessionLoaded{false}; // worker side only

    // Last member, so it is destroyed first and joins before anything it may use.
    CommandWorker m_worker;
};

} // namespace rialto_refplayer
