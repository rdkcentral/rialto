#pragma once

#include "EventBus.h"
#include "Logger.h"

#include "rialto_refplayer/RefPlayerTypes.h"

#include "IMediaPipeline.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace rialto_refplayer
{

// Owns the AAMP-coded player state. Rialto callbacks are the only thing that moves it
// to PLAYING, PAUSED, SEEKING, BUFFERING, STOPPED, COMPLETE or ERROR. The worker sets
// the states it owns (INITIALIZING, PREPARING, PREPARED, STOPPING, ...). Every change
// is published once, after the lock is released.
class PlayerStateMachine
{
public:
    PlayerStateMachine(std::shared_ptr<EventBus> bus, std::shared_ptr<Logger> logger);

    PlayerState state() const { return m_state.load(); }

    // False while INITIALIZING. Any Rialto callback then comes from the pipeline being
    // replaced, so the callers drop it.
    bool acceptingRialto() const { return m_state.load() != PlayerState::Initializing; }

    // Worker-side transitions. A no-op if the state does not change.
    void setState(PlayerState next, const std::string &reason);

    // Failure after acceptance: state ERROR, then playbackFailed.
    void fail(const std::string &description);

    // Rejected command: playbackFailed only, state unchanged.
    void reject(const std::string &description);

    // Rialto callbacks (IPC thread). Callers check acceptingRialto() first.
    void onPlaybackState(firebolt::rialto::PlaybackState state);
    void onNetworkState(firebolt::rialto::NetworkState state);
    void onPlaybackError(const std::string &description);

    // Bounded wait for a state. Only the worker calls this, never the Rialto thread.
    bool waitForState(PlayerState target, std::chrono::milliseconds timeout);

    // Clears per-load flags at the start of a new load.
    void resetForLoad();

private:
    struct Pending
    {
        std::vector<std::string> logs;
        std::vector<Event> events;
    };

    void transitionLocked(PlayerState next, const std::string &reason, Pending &pending);
    void failLocked(const std::string &description, Pending &pending);
    void setBufferingLocked(bool buffering, Pending &pending);
    PlayerState resolveSeekLocked() const;
    void flush(const Pending &pending);

    std::shared_ptr<EventBus> m_bus;
    std::shared_ptr<Logger> m_logger;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<PlayerState> m_state{PlayerState::Idle};
    PlayerState m_preSeekState{PlayerState::Playing};
    PlayerState m_stateBeforeBuffering{PlayerState::Playing};
    bool m_buffering{false};
};

} // namespace rialto_refplayer
