#include "PipelineCallbacks.h"

#include "RialtoNames.h"

#include <chrono>

namespace rialto_refplayer
{
namespace
{

int64_t steadyNowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

} // namespace

PipelineCallbacks::PipelineCallbacks(std::shared_ptr<PlayerStateMachine> state,
                                     std::shared_ptr<PositionTracker> position, std::shared_ptr<MediaFeeder> feeder,
                                     std::shared_ptr<Logger> logger)
    : m_state(std::move(state)), m_position(std::move(position)), m_feeder(std::move(feeder)),
      m_logger(std::move(logger))
{
}

void PipelineCallbacks::deactivate()
{
    m_active = false;
}

void PipelineCallbacks::markLoadStart()
{
    m_firstFrameReported = false;
    m_loadStartNs = steadyNowNs();
}

void PipelineCallbacks::notifyDuration(int64_t duration)
{
    if (!m_active || !m_state->acceptingRialto())
    {
        return;
    }
    m_position->onDuration(duration);
    m_logger->debug("notifyDuration: " + std::to_string(duration));
}

void PipelineCallbacks::notifyPosition(int64_t position)
{
    if (!m_active || !m_state->acceptingRialto())
    {
        return;
    }
    m_position->onPosition(position);
}

void PipelineCallbacks::notifyNativeSize(uint32_t width, uint32_t height, double aspect)
{
    m_logger->debug("notifyNativeSize: " + std::to_string(width) + "x" + std::to_string(height) + " aspect " +
                    std::to_string(aspect));
}

void PipelineCallbacks::notifyNetworkState(firebolt::rialto::NetworkState state)
{
    if (!m_active)
    {
        return;
    }
    m_logger->debug("notifyNetworkState: " + toString(state));
    m_state->onNetworkState(state);
}

void PipelineCallbacks::notifyPlaybackState(firebolt::rialto::PlaybackState state)
{
    if (!m_active)
    {
        return;
    }
    m_logger->debug("notifyPlaybackState: " + toString(state));
    m_state->onPlaybackState(state);
}

void PipelineCallbacks::notifyVideoData(bool hasData)
{
    m_logger->debug(std::string("notifyVideoData: ") + (hasData ? "true" : "false"));
}

void PipelineCallbacks::notifyAudioData(bool hasData)
{
    m_logger->debug(std::string("notifyAudioData: ") + (hasData ? "true" : "false"));
}

void PipelineCallbacks::notifyNeedMediaData(int32_t sourceId, size_t frameCount, uint32_t needDataRequestId,
                                            const std::shared_ptr<firebolt::rialto::MediaPlayerShmInfo> &shmInfo)
{
    (void)shmInfo;
    if (!m_active)
    {
        return;
    }
    m_feeder->enqueue(sourceId, frameCount, needDataRequestId);
}

void PipelineCallbacks::notifyCancelNeedMediaData(int32_t sourceId)
{
    if (!m_active)
    {
        return;
    }
    m_feeder->cancel(sourceId);
}

void PipelineCallbacks::notifyQos(int32_t sourceId, const firebolt::rialto::QosInfo &qosInfo)
{
    m_logger->debug("notifyQos: sourceId=" + std::to_string(sourceId) + ", processed=" +
                    std::to_string(qosInfo.processed) + ", dropped=" + std::to_string(qosInfo.dropped));
}

void PipelineCallbacks::notifyBufferUnderflow(int32_t sourceId)
{
    m_logger->debug("notifyBufferUnderflow: sourceId=" + std::to_string(sourceId));
}

void PipelineCallbacks::notifyFirstFrameReceived(int32_t sourceId)
{
    m_logger->debug("notifyFirstFrameReceived: sourceId=" + std::to_string(sourceId));
    if (!m_active)
    {
        return;
    }
    const int64_t start = m_loadStartNs.load();
    if (start != 0 && !m_firstFrameReported.exchange(true))
    {
        m_logger->tuneTime((steadyNowNs() - start) / 1000000);
    }
}

void PipelineCallbacks::notifyPlaybackError(int32_t sourceId, firebolt::rialto::PlaybackError error)
{
    if (!m_active)
    {
        return;
    }
    m_state->onPlaybackError("playback error " + toString(error) + " on source " + std::to_string(sourceId));
}

void PipelineCallbacks::notifySourceFlushed(int32_t sourceId)
{
    m_logger->debug("notifySourceFlushed: sourceId=" + std::to_string(sourceId));
}

void PipelineCallbacks::notifyPlaybackInfo(const firebolt::rialto::PlaybackInfo &playbackInfo)
{
    if (!m_active || !m_state->acceptingRialto() || playbackInfo.currentPosition < 0)
    {
        return;
    }
    m_position->onPosition(playbackInfo.currentPosition);
}

} // namespace rialto_refplayer
