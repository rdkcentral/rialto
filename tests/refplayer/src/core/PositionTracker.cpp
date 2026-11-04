#include "PositionTracker.h"

namespace rialto_refplayer
{
namespace
{
constexpr int64_t kNsPerMs = 1000000;
} // namespace

PositionTracker::PositionTracker(std::shared_ptr<EventBus> bus, int64_t progressIntervalMs)
    : m_bus(std::move(bus)), m_progressIntervalMs(progressIntervalMs)
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
}

void PositionTracker::onPosition(int64_t positionNs)
{
    m_positionNs = positionNs;

    const auto now = std::chrono::steady_clock::now();
    const auto interval = std::chrono::milliseconds(m_progressIntervalMs.load());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_published && now - m_lastPublish < interval)
        {
            return;
        }
        m_published = true;
        m_lastPublish = now;
    }

    Event event;
    event.type = EventType::ProgressUpdate;
    event.positionMs = positionNs / kNsPerMs;
    m_bus->publish(event);
}

void PositionTracker::onDuration(int64_t durationNs)
{
    m_durationNs = durationNs;
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
