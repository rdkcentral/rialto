#include "PositionTracker.h"

namespace rialto_refplayer
{
namespace
{
constexpr int64_t kNsPerMs = 1000000;
constexpr std::chrono::seconds kPositionLogInterval{1};
} // namespace

PositionTracker::PositionTracker(std::shared_ptr<EventBus> bus, std::shared_ptr<Logger> logger,
                                 int64_t progressIntervalMs)
    : m_bus(std::move(bus)), m_logger(std::move(logger)), m_progressIntervalMs(progressIntervalMs)
{
}

void PositionTracker::setProgressInterval(int64_t progressIntervalMs)
{
    m_progressIntervalMs = progressIntervalMs;
}

void PositionTracker::reset()
{
    m_positionNs = 0;
    m_durationNs = 0;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_published = false;
    m_logged = false;
}

void PositionTracker::onPosition(int64_t positionNs)
{
    m_positionNs = positionNs;

    const auto now = std::chrono::steady_clock::now();
    const auto interval = std::chrono::milliseconds(m_progressIntervalMs.load());
    bool logPosition = false;
    bool publish = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_logged || now - m_lastLog >= kPositionLogInterval)
        {
            m_logged = true;
            m_lastLog = now;
            logPosition = true;
        }
        if (!m_published || now - m_lastPublish >= interval)
        {
            m_published = true;
            m_lastPublish = now;
            publish = true;
        }
    }

    if (logPosition)
    {
        m_logger->info("position: " + std::to_string(positionNs / kNsPerMs) + " ms");
    }
    if (!publish)
    {
        return;
    }

    Event event;
    event.type = EventType::ProgressUpdate;
    event.positionMs = positionNs / kNsPerMs;
    m_bus->publish(event);
}

void PositionTracker::onDuration(int64_t durationNs)
{
    m_durationNs = durationNs;
    // Negative values are Rialto's kDurationUnknown (-1) and kDurationUnending (-2), not lengths.
    m_logger->info(durationNs < 0 ? "duration: unknown or unending (" + std::to_string(durationNs) + ")"
                                  : "duration: " + std::to_string(durationNs / kNsPerMs) + " ms");
}

void PositionTracker::onDurationFallback(int64_t durationNs)
{
    int64_t unreported = 0;
    if (durationNs <= 0 || !m_durationNs.compare_exchange_strong(unreported, durationNs))
    {
        return;
    }
    m_logger->warning("duration not supplied by the Rialto server; using " + std::to_string(durationNs / kNsPerMs) +
                      " ms taken from the player's own demuxer");
}

int64_t PositionTracker::positionMs() const
{
    return m_positionNs.load() / kNsPerMs;
}

int64_t PositionTracker::durationMs() const
{
    return m_durationNs.load() / kNsPerMs;
}

} // namespace rialto_refplayer
