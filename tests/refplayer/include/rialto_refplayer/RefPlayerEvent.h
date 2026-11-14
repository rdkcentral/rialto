#pragma once

#include <MediaCommon.h>

#include "RefPlayerTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace rialto_refplayer
{

// Event names are AAMP's. The glue layer maps each type to its JS event name.
enum class EventType
{
    StateChanged,      // playbackStateChanged   -> state
    ProgressUpdate,    // playbackProgressUpdate -> positionMiliseconds
    Failed,            // playbackFailed         -> description
    BufferingChanged,  // bufferingChanged       -> buffering          (phase 2)
    SpeedsChanged,     // speedsChanged          -> speeds             (phase 2)
    HttpResponseHeader, // httpResponseHeader     -> payload            (phase 3)
    MetricReceived      // Rialto playback metric callback (CLI/internal)
};

// One value type for every event. Only the fields named in the comment above are
// meaningful for a given type; the rest keep their defaults.
struct Event
{
    EventType type{EventType::StateChanged};
    PlayerState state{PlayerState::Idle};
    int64_t positionMs{0};
    std::string description;
    bool buffering{false};
    std::vector<double> speeds;
    std::string headerPayload;
    firebolt::rialto::MetricEvent metric;
};

class IRefPlayerClient
{
public:
    virtual ~IRefPlayerClient() = default;

    // Called from any library thread. Must not block and must not call back into IRefPlayer.
    virtual void onEvent(const Event &event) = 0;
};

} // namespace rialto_refplayer
