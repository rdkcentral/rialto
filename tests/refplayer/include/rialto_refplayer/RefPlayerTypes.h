#pragma once

#include <cstdint>
#include <string>

namespace rialto_refplayer
{

// AAMP's numeric player states. The library emits these values directly, so the
// glue layer maps a number to a JSValue and owns no translation table.
enum class PlayerState : int
{
    Idle = 0,
    Initializing = 1,
    Initialized = 2,
    Preparing = 3,
    Prepared = 4,
    Buffering = 5,
    Paused = 6,
    Seeking = 7,
    Playing = 8,
    Stopping = 9,
    Stopped = 10,
    Complete = 11,
    Error = 12,
    Released = 13,
    Blocked = 14
};

enum class Result
{
    Ok,
    InvalidState,
    InvalidArgument,
    NotSupported,
    RialtoError,
    // initialize() found the application not in RUNNING state (CHANGE_SPEC §2.6).
    ApplicationNotRunning
};

struct Options
{
    // Progress throttle. The glue converts AAMP's progressReportingInterval (seconds).
    int64_t progressIntervalMs{500};

    // Pipeline video requirements. Source dimensions still come from the content.
    uint32_t maxVideoWidth{1920};
    uint32_t maxVideoHeight{1080};

    // Device tokens from org.rdk.AuthService. Stored for phase-4 licence acquisition.
    // Never logged.
    std::string authToken;
    std::string accountId;
    std::string deviceId;

    // Debug-level library logs (per-sample and per-callback detail).
    bool verboseLogging{false};
};

inline const char *toString(PlayerState state)
{
    switch (state)
    {
    case PlayerState::Idle: return "IDLE";
    case PlayerState::Initializing: return "INITIALIZING";
    case PlayerState::Initialized: return "INITIALIZED";
    case PlayerState::Preparing: return "PREPARING";
    case PlayerState::Prepared: return "PREPARED";
    case PlayerState::Buffering: return "BUFFERING";
    case PlayerState::Paused: return "PAUSED";
    case PlayerState::Seeking: return "SEEKING";
    case PlayerState::Playing: return "PLAYING";
    case PlayerState::Stopping: return "STOPPING";
    case PlayerState::Stopped: return "STOPPED";
    case PlayerState::Complete: return "COMPLETE";
    case PlayerState::Error: return "ERROR";
    case PlayerState::Released: return "RELEASED";
    case PlayerState::Blocked: return "BLOCKED";
    }
    return "UNKNOWN";
}

inline const char *toString(Result result)
{
    switch (result)
    {
    case Result::Ok: return "OK";
    case Result::InvalidState: return "INVALID_STATE";
    case Result::InvalidArgument: return "INVALID_ARGUMENT";
    case Result::NotSupported: return "NOT_SUPPORTED";
    case Result::RialtoError: return "RIALTO_ERROR";
    case Result::ApplicationNotRunning: return "APPLICATION_NOT_RUNNING";
    }
    return "UNKNOWN";
}

} // namespace rialto_refplayer
