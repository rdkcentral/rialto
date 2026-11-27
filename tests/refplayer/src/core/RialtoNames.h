#pragma once

#include "IControl.h"
#include "IMediaPipeline.h"

#include <string>

// Names for Rialto enums, shared by every core component for logging.
namespace rialto_refplayer
{

inline std::string toString(firebolt::rialto::PlaybackState state)
{
    using firebolt::rialto::PlaybackState;
    switch (state)
    {
    case PlaybackState::UNKNOWN: return "UNKNOWN";
    case PlaybackState::IDLE: return "IDLE";
    case PlaybackState::PLAYING: return "PLAYING";
    case PlaybackState::PAUSED: return "PAUSED";
    case PlaybackState::SEEKING: return "SEEKING";
    case PlaybackState::SEEK_DONE: return "SEEK_DONE";
    case PlaybackState::STOPPED: return "STOPPED";
    case PlaybackState::END_OF_STREAM: return "END_OF_STREAM";
    case PlaybackState::FAILURE: return "FAILURE";
    }
    return "UNKNOWN";
}

inline std::string toString(firebolt::rialto::NetworkState state)
{
    using firebolt::rialto::NetworkState;
    switch (state)
    {
    case NetworkState::UNKNOWN: return "UNKNOWN";
    case NetworkState::IDLE: return "IDLE";
    case NetworkState::BUFFERING: return "BUFFERING";
    case NetworkState::BUFFERING_PROGRESS: return "BUFFERING_PROGRESS";
    case NetworkState::BUFFERED: return "BUFFERED";
    case NetworkState::STALLED: return "STALLED";
    case NetworkState::FORMAT_ERROR: return "FORMAT_ERROR";
    case NetworkState::NETWORK_ERROR: return "NETWORK_ERROR";
    case NetworkState::DECODE_ERROR: return "DECODE_ERROR";
    }
    return "UNKNOWN";
}

inline std::string toString(firebolt::rialto::PlaybackError error)
{
    using firebolt::rialto::PlaybackError;
    switch (error)
    {
    case PlaybackError::UNKNOWN: return "UNKNOWN";
    case PlaybackError::DECRYPTION: return "DECRYPTION";
    case PlaybackError::OUTPUT_PROTECTION: return "OUTPUT_PROTECTION";
    }
    return "UNKNOWN";
}

inline std::string toString(firebolt::rialto::ApplicationState state)
{
    using firebolt::rialto::ApplicationState;
    switch (state)
    {
    case ApplicationState::UNKNOWN: return "UNKNOWN";
    case ApplicationState::RUNNING: return "RUNNING";
    case ApplicationState::INACTIVE: return "INACTIVE";
    }
    return "UNKNOWN";
}

} // namespace rialto_refplayer
