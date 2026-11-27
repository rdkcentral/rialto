#pragma once

#include "EventBus.h"
#include "Logger.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>

namespace rialto_refplayer
{

// Position and duration caches. notify*() writes them on the Rialto IPC thread, and
// the JS-facing getters read them lock-free, with no IPC. Progress events are
// published no more often than the configured interval. Each duration notification is
// logged, and the position is logged at most once a second.
class PositionTracker
{
public:
    PositionTracker(std::shared_ptr<EventBus> bus, std::shared_ptr<Logger> logger, int64_t progressIntervalMs);

    void setProgressInterval(int64_t progressIntervalMs);
    void reset(); // new load: position and duration go back to zero

    void onPosition(int64_t positionNs);
    void onDuration(int64_t durationNs);
    // Backup for a server that sends no duration. Used only while none has been reported, and
    // logged as a warning. A later onDuration() replaces it.
    void onDurationFallback(int64_t durationNs);

    int64_t positionMs() const;
    int64_t durationMs() const;

private:
    std::shared_ptr<EventBus> m_bus;
    std::shared_ptr<Logger> m_logger;
    std::atomic<int64_t> m_positionNs{0};
    std::atomic<int64_t> m_durationNs{0};
    std::atomic<int64_t> m_progressIntervalMs;

    std::mutex m_mutex; // guards the members below
    bool m_published{false};
    bool m_logged{false};
    std::chrono::steady_clock::time_point m_lastPublish;
    std::chrono::steady_clock::time_point m_lastLog;
};

} // namespace rialto_refplayer
