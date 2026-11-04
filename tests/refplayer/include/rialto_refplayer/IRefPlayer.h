#pragma once

#include "RefPlayerEvent.h"
#include "RefPlayerTypes.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The library's only exported symbol family. The library is built with -fvisibility=hidden.
#if defined(__GNUC__)
#define RIALTO_REFPLAYER_API __attribute__((visibility("default")))
#else
#define RIALTO_REFPLAYER_API
#endif

namespace rialto_refplayer
{

// Threading contract:
//  - initialize() is the only call that may block. It connects to Rialto.
//  - Control calls validate what they can, enqueue the work, and return. Completion
//    and failure after acceptance are reported as events, never as exceptions.
//  - Getters are lock-free reads of cached values. They never touch Rialto.
//  - No method may be called from inside IRefPlayerClient::onEvent().
class IRefPlayer
{
public:
    virtual ~IRefPlayer() = default;

    // Lifecycle
    virtual Result initialize(const Options &options) = 0;
    virtual Result updateConfig(const Options &options) = 0;
    virtual void destroy() = 0; // idempotent; releases the pipeline, joins the worker and feeder

    // Control. Result::Ok means accepted, not completed.
    virtual Result load(const std::string &url, bool autoplay) = 0;
    virtual Result play() = 0;
    virtual Result pause() = 0;
    virtual Result stop() = 0; // releases the pipeline
    virtual Result seek(int64_t positionMs) = 0;
    virtual Result setPlaybackRate(double rate) = 0;          // phase 2
    virtual Result setVolume(double volume) = 0;              // phase 2
    virtual Result setVideoWindow(uint32_t x, uint32_t y, uint32_t w, uint32_t h) = 0; // phase 2
    virtual Result setVideoMute(bool mute) = 0;               // phase 2
    virtual Result setTextTrack(int index) = 0;               // phase 4
    virtual Result setClosedCaptions(bool enabled) = 0;       // phase 4

    // Getters: cached, lock-free, never IPC
    virtual int64_t positionMs() const = 0;
    virtual int64_t durationMs() const = 0;
    virtual PlayerState state() const = 0;
    virtual std::optional<double> volume() const = 0;               // phase 2
    virtual std::vector<double> supportedSpeeds() const = 0;        // phase 2; {1.0} before
    virtual std::optional<std::string> textTracksJson() const = 0;  // phase 4
    virtual std::optional<int> textTrack() const = 0;               // phase 4

    virtual void setClient(std::weak_ptr<IRefPlayerClient> client) = 0;
};

RIALTO_REFPLAYER_API std::shared_ptr<IRefPlayer> createRefPlayer();

} // namespace rialto_refplayer
