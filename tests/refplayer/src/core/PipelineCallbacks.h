#pragma once

#include "EventBus.h"
#include "Logger.h"
#include "MediaFeeder.h"
#include "PlayerStateMachine.h"
#include "PositionTracker.h"

#include "IMediaPipeline.h"

#include <atomic>
#include <cstdint>
#include <memory>

namespace rialto_refplayer
{

// The IMediaPipelineClient for one pipeline. Rialto calls these on its IPC threads, so
// each one routes its work to a component and returns. Nothing here waits on the worker.
// The object is created per pipeline and deactivated on release, so a late callback
// from a dead pipeline cannot change the state of the next one.
class PipelineCallbacks final : public firebolt::rialto::IMediaPipelineClient
{
public:
    PipelineCallbacks(std::shared_ptr<EventBus> bus, std::shared_ptr<PlayerStateMachine> state,
                      std::shared_ptr<PositionTracker> position, std::shared_ptr<MediaFeeder> feeder,
                      std::shared_ptr<Logger> logger);

    // Stops forwarding. Safe from any thread.
    void deactivate();

    // Marks the start of a load, for the tune-time line. Resets the first-frame latch.
    void markLoadStart();

    void notifyDuration(int64_t duration) override;
    void notifyPosition(int64_t position) override;
    void notifyNativeSize(uint32_t width, uint32_t height, double aspect) override;
    void notifyNetworkState(firebolt::rialto::NetworkState state) override;
    void notifyPlaybackState(firebolt::rialto::PlaybackState state) override;
    void notifyVideoData(bool hasData) override;
    void notifyAudioData(bool hasData) override;
    void notifyNeedMediaData(int32_t sourceId, size_t frameCount, uint32_t needDataRequestId,
                             const std::shared_ptr<firebolt::rialto::MediaPlayerShmInfo> &shmInfo) override;
    void notifyCancelNeedMediaData(int32_t sourceId) override;
    void notifyQos(int32_t sourceId, const firebolt::rialto::QosInfo &qosInfo) override;
    void notifyMetric(const firebolt::rialto::MetricEvent &metric) override;
    void notifyBufferUnderflow(int32_t sourceId) override;
    void notifyFirstFrameReceived(int32_t sourceId) override;
    void notifyPlaybackError(int32_t sourceId, firebolt::rialto::PlaybackError error) override;
    void notifySourceFlushed(int32_t sourceId) override;
    void notifyPlaybackInfo(const firebolt::rialto::PlaybackInfo &playbackInfo) override;

private:
    std::shared_ptr<EventBus> m_bus;
    std::shared_ptr<PlayerStateMachine> m_state;
    std::shared_ptr<PositionTracker> m_position;
    std::shared_ptr<MediaFeeder> m_feeder;
    std::shared_ptr<Logger> m_logger;

    std::atomic<bool> m_active{true};
    std::atomic<int64_t> m_loadStartNs{0}; // steady clock, 0 when no load is timed
    std::atomic<bool> m_firstFrameReported{false};
};

} // namespace rialto_refplayer
