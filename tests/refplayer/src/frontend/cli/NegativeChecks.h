#pragma once

#include "IControl.h"
#include "IMediaPipeline.h"
#include "IWebAudioPlayer.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace rialto_refplayer_cli
{

// Web audio configuration used by the probes: 48 kHz, stereo, signed 16-bit PCM.
constexpr uint32_t kWebAudioRate = 48000U;
constexpr uint32_t kWebAudioChannels = 2U;
constexpr uint32_t kWebAudioSampleSize = 16U;

// Player states as the probes log them. These are not the library's PlayerState.
enum class ProbeState
{
    CREATED,
    LOADED,
    PLAYING,
    PAUSED,
    STOPPED,
    FAILED
};

// A raw Rialto session owned by the CLI for one check. It is also the client that the
// Rialto objects call back into, so it must outlive them. Tear it down with
// destroyPipelineProbe(): the mock control holds its client strongly.
class RawProbe : public std::enable_shared_from_this<RawProbe>,
                 public firebolt::rialto::IControlClient,
                 public firebolt::rialto::IMediaPipelineClient,
                 public firebolt::rialto::IWebAudioPlayerClient
{
public:
    bool createControl();
    bool connectControl();
    bool createMediaPipeline(const firebolt::rialto::VideoRequirements &requirements =
                                 firebolt::rialto::VideoRequirements{1920, 1080});
    bool createWebAudio(uint32_t priority);

    void notifyApplicationState(firebolt::rialto::ApplicationState state) override;
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
    void notifyBufferUnderflow(int32_t sourceId) override;
    void notifyFirstFrameReceived(int32_t sourceId) override;
    void notifyPlaybackError(int32_t sourceId, firebolt::rialto::PlaybackError error) override;
    void notifySourceFlushed(int32_t sourceId) override;
    void notifyPlaybackInfo(const firebolt::rialto::PlaybackInfo &playbackInfo) override;
    void notifyState(firebolt::rialto::WebAudioPlayerState state) override;

    std::shared_ptr<firebolt::rialto::IControl> control;
    std::shared_ptr<firebolt::rialto::IMediaPipeline> mediaPipeline;
    std::unique_ptr<firebolt::rialto::IWebAudioPlayer> webAudioPlayer;
    std::shared_ptr<firebolt::rialto::WebAudioConfig> webAudioConfig;
    int32_t currentSourceId{0};
    int32_t audioSourceId{0};

private:
    void setState(ProbeState next, const std::string &reason);

    std::atomic<ProbeState> m_playerState{ProbeState::CREATED};
};

// Probe helpers. Each returns null when the Rialto setup fails; the failure is logged.
// The attached probe is loaded, has one video source of the given MIME type, optionally
// an AAC audio source, and has its source list finalised.
std::shared_ptr<RawProbe> createAttachedPipelineProbe(const std::string &mimeType = "video/mp4",
                                                       bool withAudio = false);
void destroyPipelineProbe(std::shared_ptr<RawProbe> &probe);

// Runs the negative suite and returns 0 on success. In mock mode it logs a skip and returns 0.
int runNegativeChecks();

} // namespace rialto_refplayer_cli
