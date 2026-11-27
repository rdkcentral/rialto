#include "PlayerStateMachine.h"

#include "RialtoNames.h"

namespace rialto_refplayer
{

using firebolt::rialto::NetworkState;
using firebolt::rialto::PlaybackState;

PlayerStateMachine::PlayerStateMachine(std::shared_ptr<EventBus> bus, std::shared_ptr<Logger> logger)
    : m_bus(std::move(bus)), m_logger(std::move(logger))
{
}

void PlayerStateMachine::setState(PlayerState next, const std::string &reason)
{
    Pending pending;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        transitionLocked(next, reason, pending);
    }
    flush(pending);
}

void PlayerStateMachine::fail(const std::string &description)
{
    Pending pending;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        failLocked(description, pending);
    }
    flush(pending);
}

void PlayerStateMachine::reject(const std::string &description)
{
    Pending pending;
    pending.logs.push_back("rejected: " + description);
    Event event;
    event.type = EventType::Failed;
    event.description = description;
    pending.events.push_back(event);
    flush(pending);
}

void PlayerStateMachine::onPlaybackState(PlaybackState rialtoState)
{
    Pending pending;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const PlayerState current = m_state.load();

        // Initializing: the callback belongs to the pipeline being replaced.
        // Stopping: a stop finishes on STOPPED alone, so a late PLAYING or PAUSED must not undo it.
        if (current == PlayerState::Initializing)
        {
            pending.logs.push_back("ignoring Rialto " + toString(rialtoState) + " while initializing");
        }
        else if (current == PlayerState::Stopping && rialtoState != PlaybackState::STOPPED &&
                 rialtoState != PlaybackState::FAILURE)
        {
            pending.logs.push_back("ignoring Rialto " + toString(rialtoState) + " while stopping");
        }
        else
        {
            switch (rialtoState)
            {
            case PlaybackState::PLAYING:
                transitionLocked(PlayerState::Playing, "Rialto PLAYING", pending);
                break;
            case PlaybackState::PAUSED:
                transitionLocked(PlayerState::Paused, "Rialto PAUSED", pending);
                break;
            case PlaybackState::SEEKING:
                if (current != PlayerState::Seeking)
                {
                    m_preSeekState = current;
                }
                transitionLocked(PlayerState::Seeking, "Rialto SEEKING", pending);
                break;
            case PlaybackState::SEEK_DONE:
                transitionLocked(resolveSeekLocked(), "Rialto SEEK_DONE", pending);
                break;
            case PlaybackState::STOPPED:
                transitionLocked(PlayerState::Stopped, "Rialto STOPPED", pending);
                break;
            case PlaybackState::END_OF_STREAM:
                transitionLocked(PlayerState::Complete, "Rialto END_OF_STREAM", pending);
                break;
            case PlaybackState::FAILURE:
                failLocked("Rialto reported FAILURE", pending);
                break;
            case PlaybackState::IDLE:
            case PlaybackState::UNKNOWN:
                pending.logs.push_back("Rialto playback state " + toString(rialtoState) + " ignored");
                break;
            }
        }
    }
    flush(pending);
}

void PlayerStateMachine::onPlaybackError(const std::string &description)
{
    if (!acceptingRialto())
    {
        m_logger->info("ignoring playback error while initializing: " + description);
        return;
    }
    fail(description);
}

void PlayerStateMachine::onNetworkState(NetworkState rialtoState)
{
    Pending pending;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_state.load() == PlayerState::Initializing)
        {
            pending.logs.push_back("ignoring Rialto network state " + toString(rialtoState) + " while initializing");
        }
        else
        switch (rialtoState)
        {
        case NetworkState::BUFFERING:
        case NetworkState::BUFFERING_PROGRESS:
        case NetworkState::STALLED:
            setBufferingLocked(true, pending);
            break;
        case NetworkState::BUFFERED:
        case NetworkState::IDLE:
            setBufferingLocked(false, pending);
            break;
        case NetworkState::FORMAT_ERROR:
        case NetworkState::NETWORK_ERROR:
        case NetworkState::DECODE_ERROR:
            failLocked("Rialto network state " + toString(rialtoState), pending);
            break;
        case NetworkState::UNKNOWN:
            pending.logs.push_back("Rialto network state UNKNOWN ignored");
            break;
        }
    }
    flush(pending);
}

bool PlayerStateMachine::waitForState(PlayerState target, std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_cv.wait_for(lock, timeout, [this, target] { return m_state.load() == target; });
}

void PlayerStateMachine::resetForLoad()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffering = false;
    m_preSeekState = PlayerState::Playing;
    m_stateBeforeBuffering = PlayerState::Playing;
}

void PlayerStateMachine::transitionLocked(PlayerState next, const std::string &reason, Pending &pending)
{
    const PlayerState previous = m_state.load();
    if (previous == next)
    {
        return;
    }
    m_state.store(next);
    pending.logs.push_back(std::string("state ") + toString(previous) + " -> " + toString(next) + " (" + reason + ")");

    Event event;
    event.type = EventType::StateChanged;
    event.state = next;
    pending.events.push_back(event);
    m_cv.notify_all();
}

void PlayerStateMachine::failLocked(const std::string &description, Pending &pending)
{
    transitionLocked(PlayerState::Error, description, pending);

    Event event;
    event.type = EventType::Failed;
    event.description = description;
    pending.events.push_back(event);
}

void PlayerStateMachine::setBufferingLocked(bool buffering, Pending &pending)
{
    if (buffering == m_buffering)
    {
        return;
    }
    m_buffering = buffering;

    Event event;
    event.type = EventType::BufferingChanged;
    event.buffering = buffering;
    pending.events.push_back(event);

    const PlayerState current = m_state.load();
    if (buffering && current == PlayerState::Playing)
    {
        m_stateBeforeBuffering = current;
        transitionLocked(PlayerState::Buffering, "buffering", pending);
    }
    else if (!buffering && current == PlayerState::Buffering)
    {
        transitionLocked(m_stateBeforeBuffering, "buffered", pending);
    }
}

PlayerState PlayerStateMachine::resolveSeekLocked() const
{
    switch (m_preSeekState)
    {
    case PlayerState::Paused:
        return PlayerState::Paused;
    case PlayerState::Prepared:
        return PlayerState::Prepared;
    default:
        return PlayerState::Playing;
    }
}

void PlayerStateMachine::flush(const Pending &pending)
{
    for (const auto &line : pending.logs)
    {
        m_logger->info(line);
    }
    for (const auto &event : pending.events)
    {
        m_bus->publish(event);
    }
}

} // namespace rialto_refplayer
