#pragma once

#include "EventBus.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>

namespace rialto_refplayer
{

// Position and duration caches. notify*() writes them on the Rialto IPC thread, and
// the JS-facing getters read them lock-free, with no IPC. Progress events are
// published no more often than the configured interval.
class PositionTracker
{
public:
    PositionTracker(std::shared_ptr<EventBus> bus, int64_t progressIntervalMs);

    void setProgressInterval(int64_t progressIntervalMs);
    void reset(); // new load: position and duration go back to zero

    void onPosition(int64_t positionNs);
    void onDuration(int64_t durationNs);

    int64_t positionMs() const;
    int64_t durationMs() const;

private:
    std::shared_ptr<EventBus> m_bus;
    std::atomic<int64_t> m_positionNs{0};
    std::atomic<int64_t> m_durationNs{0};
    std::atomic<int64_t> m_progressIntervalMs;

    std::mutex m_mutex; // guards m_lastPublish
    bool m_published{false};
    std::chrono::steady_clock::time_point m_lastPublish;
};

} // namespace rialto_refplayer
