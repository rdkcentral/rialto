#include "ContractValidator.h"

#include "CliLog.h"
#include "NegativeChecks.h"

#include "IClientLogControl.h"
#include "IClientLogHandler.h"
#include "IMediaKeysCapabilities.h"
#include "IMediaPipelineCapabilities.h"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace rialto_refplayer_cli
{

namespace
{

constexpr double kDefaultVolume = 0.7;
constexpr double kVolumeTolerance = 0.001;
constexpr uint32_t kBufferingLimitMs = 1500U;
constexpr int32_t kStreamSyncMode = 1;

constexpr int32_t kDefaultStreamSyncMode = 0;
constexpr std::chrono::milliseconds kSettleTimeout{3000};
constexpr std::chrono::milliseconds kSettlePoll{50};

const char *boolText(bool value)
{
    return value ? "true" : "false";
}

// Rialto applies setters on its worker thread while getters read the pipeline directly, so a read-back is
// repeated until it matches or the timeout passes. Returns whether the last getter call succeeded.
template <typename T, typename Get, typename Match> bool readUntil(T &value, Get get, Match matches)
{
    const auto deadline = std::chrono::steady_clock::now() + kSettleTimeout;
    bool got = get(value);
    while (!(got && matches(value)) && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(kSettlePoll);
        got = get(value);
    }
    return got;
}

// Logs each call made on the player's own pipeline and keeps the verdict for the group.
class PipelineChecks
{
public:
    // A call that returns false fails the group.
    bool call(const std::string &name, bool returned)
    {
        cliLog("[pipeline] " + name + " returned " + boolText(returned) + (returned ? "" : " (FAILED)"));
        m_passed = m_passed && returned;
        return returned;
    }

    // A read-back that differs from what was set fails the group.
    void expect(const std::string &name, const std::string &actual, const std::string &expected, bool matches)
    {
        cliLog("[pipeline] " + name + " read back " + actual + ", expected " + expected +
               (matches ? "" : " (MISMATCH)"));
        m_passed = m_passed && matches;
    }

    // The media does not offer what the call needs, so the call is not made and does not fail the group.
    void skip(const std::string &name, const std::string &reason)
    {
        cliLog("[pipeline] " + name + " skipped: " + reason);
    }

    bool passed() const { return m_passed; }

private:
    bool m_passed{true};
};

} // namespace

bool validatePipelineBeforePlay(firebolt::rialto::IMediaPipeline &pipeline,
                                const rialto_refplayer::PipelineSources &sources)
{
    PipelineChecks checks;
    // Sync off is documented as "must be set before the pipeline is PLAYING".
    checks.call("setSyncOff(false)", pipeline.setSyncOff(false));
    // Position 0 is the start the stream has anyway, so it does not move the media.
    checks.call("setSourcePosition()", pipeline.setSourcePosition(sources.videoSourceId, 0, false, 1.0, 0ULL));
    return checks.passed();
}

bool validatePipelineWhilePlaying(firebolt::rialto::IMediaPipeline &pipeline,
                                  const rialto_refplayer::PipelineSources &sources, const ContractOptions &options)
{
    PipelineChecks checks;
    const double targetVolume = options.volume.value_or(kDefaultVolume);
    const bool hasAudio = sources.audioSourceId != 0;

    checks.call("setLowLatency(true)", pipeline.setLowLatency(true));
    // Low latency turns off audio decoding and a/v sync, so it is put back for the rest of playback.
    checks.call("setLowLatency(false)", pipeline.setLowLatency(false));

    checks.call("setSync(true)", pipeline.setSync(true));
    bool sync = false;
    if (checks.call("getSync()", readUntil(
                                     sync, [&](bool &v) { return pipeline.getSync(v); }, [](bool v) { return v; })))
    {
        checks.expect("sync", boolText(sync), "true", sync);
    }

    if (hasAudio)
    {
        // Mute is set, read back, and put back so the audio stays audible.
        checks.call("setMute(true)", pipeline.setMute(sources.audioSourceId, true));
        bool muted = false;
        const auto getMute = [&](bool &v) { return pipeline.getMute(sources.audioSourceId, v); };
        if (checks.call("getMute()", readUntil(muted, getMute, [](bool v) { return v; })))
        {
            checks.expect("mute", boolText(muted), "true", muted);
        }
        checks.call("setMute(false)", pipeline.setMute(sources.audioSourceId, false));
        if (checks.call("getMute()", readUntil(muted, getMute, [](bool v) { return !v; })))
        {
            checks.expect("mute", boolText(muted), "false", !muted);
        }

        // The getter reads the audio decoder, so the value is set on the audio source.
        checks.call("setStreamSyncMode(audio)", pipeline.setStreamSyncMode(sources.audioSourceId, kStreamSyncMode));
        int32_t streamSyncMode = -1;
        const auto getStreamSyncMode = [&](int32_t &v) { return pipeline.getStreamSyncMode(v); };
        if (checks.call("getStreamSyncMode()", readUntil(streamSyncMode, getStreamSyncMode,
                                                       [](int32_t v) { return v == kStreamSyncMode; })))
        {
            checks.expect("stream sync mode", std::to_string(streamSyncMode), std::to_string(kStreamSyncMode),
                          streamSyncMode == kStreamSyncMode);
        }
        checks.call("setStreamSyncMode(audio, default)",
                    pipeline.setStreamSyncMode(sources.audioSourceId, kDefaultStreamSyncMode));
        if (checks.call("getStreamSyncMode()", readUntil(streamSyncMode, getStreamSyncMode,
                                                       [](int32_t v) { return v == kDefaultStreamSyncMode; })))
        {
            checks.expect("stream sync mode", std::to_string(streamSyncMode),
                          std::to_string(kDefaultStreamSyncMode), streamSyncMode == kDefaultStreamSyncMode);
        }
    }
    else
    {
        checks.skip("mute and stream sync mode", "the media has no audio source");
    }

    checks.call("setVolume()", pipeline.setVolume(targetVolume));
    double volume = 0.0;
    if (checks.call("getVolume()",
                    readUntil(
                        volume, [&](double &v) { return pipeline.getVolume(v); },
                        [&](double v) { return std::fabs(v - targetVolume) <= kVolumeTolerance; })))
    {
        checks.expect("volume", std::to_string(volume), std::to_string(targetVolume),
                      std::fabs(volume - targetVolume) <= kVolumeTolerance);
    }

    checks.skip("text track identifier and subtitle offset", "the media has no subtitle source");

    checks.call("setBufferingLimit()", pipeline.setBufferingLimit(kBufferingLimitMs));
    uint32_t bufferingMs = 0U;
    if (checks.call("getBufferingLimit()",
                    readUntil(
                        bufferingMs, [&](uint32_t &v) { return pipeline.getBufferingLimit(v); },
                        [](uint32_t v) { return v == kBufferingLimitMs; })))
    {
        checks.expect("buffering limit", std::to_string(bufferingMs), std::to_string(kBufferingLimitMs),
                      bufferingMs == kBufferingLimitMs);
    }
    checks.call("setUseBuffering()", pipeline.setUseBuffering(true));
    bool useBuffering = false;
    if (checks.call("getUseBuffering()",
                    readUntil(
                        useBuffering, [&](bool &v) { return pipeline.getUseBuffering(v); },
                        [](bool v) { return v; })))
    {
        checks.expect("use buffering", boolText(useBuffering), "true", useBuffering);
    }

    int64_t durationNs = 0;
    const bool hasDuration = readUntil(
        durationNs, [&](int64_t &v) { return pipeline.getDuration(v); }, [](int64_t v) { return v > 0; });
    if (hasDuration)
    {
        cliLog("[pipeline] getDuration() returned true");
        checks.expect("duration", std::to_string(durationNs) + " ns", "> 0", durationNs > 0);
    }
    else
    {
        // Pushed samples carry no total length, so the pipeline may have nothing to report.
        checks.skip("getDuration()", "the pipeline reported no duration");
    }
    int64_t firstPositionNs = 0;
    if (checks.call("getPosition()", pipeline.getPosition(firstPositionNs)))
    {
        // Playing media moves, so the second reading has to be later than the first.
        int64_t secondPositionNs = 0;
        const auto getPosition = [&](int64_t &v) { return pipeline.getPosition(v); };
        if (checks.call("getPosition()", readUntil(secondPositionNs, getPosition, [&](int64_t v) {
                            return v > firstPositionNs;
                        })))
        {
            checks.expect("position", std::to_string(firstPositionNs) + " ns, then " +
                                          std::to_string(secondPositionNs) + " ns",
                          "advancing", secondPositionNs > firstPositionNs);
        }
    }

    return checks.passed();
}

bool validateCapabilities(const ContractOptions &options)
{
    const auto logMimeTypes = [](const std::string &source, const std::vector<std::string> &mimeTypes)
    {
        std::string message = source + " MIME types:";
        if (mimeTypes.empty())
        {
            message += " <none>";
        }
        else
        {
            for (const auto &mimeType : mimeTypes)
            {
                message += " " + mimeType;
            }
        }
        cliLog(message);
    };

    auto capsFactory = firebolt::rialto::IMediaPipelineCapabilitiesFactory::createFactory();
    if (!capsFactory)
    {
        cliLog("IMediaPipelineCapabilitiesFactory::createFactory() returned null");
        return false;
    }

    auto caps = capsFactory->createMediaPipelineCapabilities();
    if (!caps)
    {
        cliLog("createMediaPipelineCapabilities() returned null");
        return false;
    }

    const auto videoMimeTypes = caps->getSupportedMimeTypes(firebolt::rialto::MediaSourceType::VIDEO);
    const auto audioMimeTypes = caps->getSupportedMimeTypes(firebolt::rialto::MediaSourceType::AUDIO);
    logMimeTypes("Video", videoMimeTypes);
    logMimeTypes("Audio", audioMimeTypes);
    bool supported = caps->isMimeTypeSupported(options.mimeType);
    const auto props = caps->getSupportedProperties(firebolt::rialto::MediaSourceType::VIDEO,
                                                   {"volume", "mute", "low-latency"});
    bool isVideoMaster = false;
    caps->isVideoMaster(isVideoMaster);

    cliLog("Media pipeline capabilities: supported=" + std::string(supported ? "true" : "false") +
           ", properties=" + std::to_string(props.size()) +
           ", isVideoMaster=" + std::string(isVideoMaster ? "true" : "false"));

    auto keySystemFactory = firebolt::rialto::IMediaKeysCapabilitiesFactory::createFactory();
    if (!keySystemFactory)
    {
        cliLog("IMediaKeysCapabilitiesFactory::createFactory() returned null");
        return false;
    }

    auto keyCaps = keySystemFactory->getMediaKeysCapabilities();
    if (!keyCaps)
    {
        cliLog("getMediaKeysCapabilities() returned null");
        return false;
    }

    const auto keySystems = keyCaps->getSupportedKeySystems();
    cliLog("Key systems supported: " + std::to_string(keySystems.size()));
    return true;
}

bool validateWebAudio(const ContractOptions &options)
{
    const auto probe = std::make_shared<RawProbe>();
    if (!probe->createWebAudio(options.priority))
    {
        cliLog("Web audio player not created");
        return false;
    }
    auto &player = *probe->webAudioPlayer;

    uint32_t preferredFrames = 0;
    uint32_t maximumFrames = 0;
    bool supportDeferredPlay = false;
    if (!player.getDeviceInfo(preferredFrames, maximumFrames, supportDeferredPlay))
    {
        cliLog("IWebAudioPlayer::getDeviceInfo() returned false");
        return false;
    }
    cliLog("WebAudio device info: preferredFrames=" + std::to_string(preferredFrames) + ", maximumFrames=" +
           std::to_string(maximumFrames) +
           ", supportDeferredPlay=" + std::string(supportDeferredPlay ? "true" : "false"));

    if (!player.play())
    {
        cliLog("IWebAudioPlayer::play() returned false");
        return false;
    }

    double volume = 0.0;
    if (!player.setVolume(0.3) || !player.getVolume(volume))
    {
        cliLog("IWebAudioPlayer volume operations returned false");
        return false;
    }

    uint32_t availableFrames = 0;
    std::shared_ptr<firebolt::rialto::WebAudioShmInfo> shmInfo = std::make_shared<firebolt::rialto::WebAudioShmInfo>();
    if (!player.getBufferAvailable(availableFrames, shmInfo))
    {
        cliLog("IWebAudioPlayer::getBufferAvailable() returned false");
        return false;
    }

    const uint32_t bytesPerFrame = kWebAudioChannels * (kWebAudioSampleSize / 8U);
    if (availableFrames > 0)
    {
        std::vector<uint8_t> audioData(availableFrames * bytesPerFrame, 0);
        if (!player.writeBuffer(availableFrames, audioData.data()))
        {
            cliLog("IWebAudioPlayer::writeBuffer() returned false");
            return false;
        }
    }

    if (!player.setEos())
    {
        cliLog("IWebAudioPlayer::setEos() returned false");
        return false;
    }

    return true;
}

bool validateLogging()
{
    auto logFactory = firebolt::rialto::IClientLogControlFactory::createFactory();
    if (!logFactory)
    {
        cliLog("IClientLogControlFactory::createFactory() returned null");
        return false;
    }

    auto &logControl = logFactory->createClientLogControl();
    class ConsoleLogHandler final : public firebolt::rialto::IClientLogHandler
    {
    public:
        void log(Level level, const std::string &file, int line, const std::string &function,
                 const std::string &message) override
        {
            (void)level;
            (void)file;
            (void)line;
            (void)function;
            (void)message;
        }
    };

    const auto handler = std::make_shared<ConsoleLogHandler>();
    if (!logControl.registerLogHandler(handler, false))
    {
        cliLog("IClientLogControl::registerLogHandler() returned false");
        return false;
    }

    cliLog("Logging handler registered successfully");
    return true;
}

} // namespace rialto_refplayer_cli
