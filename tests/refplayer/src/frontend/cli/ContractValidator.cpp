#include "ContractValidator.h"

#include "CliLog.h"
#include "NegativeChecks.h"

#include "IClientLogControl.h"
#include "IClientLogHandler.h"
#include "IMediaKeysCapabilities.h"
#include "IMediaPipelineCapabilities.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace rialto_refplayer_cli
{

namespace
{

constexpr double kDefaultVolume = 0.7;
constexpr double kVolumeTolerance = 0.001;
constexpr uint32_t kBufferingLimitMs = 1500U;
constexpr int32_t kStreamSyncMode = 1;

const char *boolText(bool value)
{
    return value ? "true" : "false";
}

} // namespace

bool validateAdvancedMediaPipelineFeatures(const ContractOptions &options)
{
    auto probe = createAttachedPipelineProbe(options.mimeType, true);
    if (!probe)
    {
        cliLog("Advanced pipeline probe could not be created");
        return false;
    }

    const auto &pipeline = probe->mediaPipeline;
    // Mute is an audio control, so it targets the probe's audio source; the stream calls
    // target the video source.
    const int32_t audioSourceId = probe->audioSourceId;
    const int32_t sourceId = probe->currentSourceId;
    const double targetVolume = options.volume.value_or(kDefaultVolume);
    bool passed = true;

    // Logs a call's return value, and fails the run when it is false.
    const auto check = [&passed](const std::string &call, bool returned)
    {
        cliLog(call + " returned " + boolText(returned));
        passed = passed && returned;
        return returned;
    };
    // Logs a read-back against the value that was set, and fails the run on a mismatch.
    const auto expectValue = [&passed](const std::string &name, const std::string &actual,
                                       const std::string &expected, bool matches)
    {
        cliLog(name + " read back " + actual + ", expected " + expected + (matches ? "" : " (MISMATCH)"));
        passed = passed && matches;
    };

    check("setLowLatency(true)", pipeline->setLowLatency(true));
    check("setSync(true)", pipeline->setSync(true));
    bool sync = false;
    if (check("getSync()", pipeline->getSync(sync)))
    {
        expectValue("sync", boolText(sync), "true", sync);
    }
    check("setSyncOff(false)", pipeline->setSyncOff(false));
    check("setMute()", pipeline->setMute(audioSourceId, true));
    bool muted = false;
    if (check("getMute()", pipeline->getMute(audioSourceId, muted)))
    {
        expectValue("mute", boolText(muted), "true", muted);
    }
    check("setVolume()", pipeline->setVolume(targetVolume));
    double volume = 0.0;
    if (check("getVolume()", pipeline->getVolume(volume)))
    {
        expectValue("volume", std::to_string(volume), std::to_string(targetVolume),
                    std::fabs(volume - targetVolume) <= kVolumeTolerance);
    }
    check("setTextTrackIdentifier()", pipeline->setTextTrackIdentifier("eng"));
    std::string textTrackId;
    if (check("getTextTrackIdentifier()", pipeline->getTextTrackIdentifier(textTrackId)))
    {
        expectValue("text track", textTrackId, "eng", textTrackId == "eng");
    }
    check("setBufferingLimit()", pipeline->setBufferingLimit(kBufferingLimitMs));
    uint32_t bufferingMs = 0U;
    if (check("getBufferingLimit()", pipeline->getBufferingLimit(bufferingMs)))
    {
        expectValue("buffering limit", std::to_string(bufferingMs), std::to_string(kBufferingLimitMs),
                    bufferingMs == kBufferingLimitMs);
    }
    check("setUseBuffering()", pipeline->setUseBuffering(true));
    bool useBuffering = false;
    if (check("getUseBuffering()", pipeline->getUseBuffering(useBuffering)))
    {
        expectValue("use buffering", boolText(useBuffering), "true", useBuffering);
    }
    check("setStreamSyncMode()", pipeline->setStreamSyncMode(sourceId, kStreamSyncMode));
    int32_t streamSyncMode = 0;
    if (check("getStreamSyncMode()", pipeline->getStreamSyncMode(streamSyncMode)))
    {
        expectValue("stream sync mode", std::to_string(streamSyncMode), std::to_string(kStreamSyncMode),
                    streamSyncMode == kStreamSyncMode);
    }
    check("setSourcePosition()", pipeline->setSourcePosition(sourceId, 0, false, 1.0, 0ULL));
    check("setSubtitleOffset()", pipeline->setSubtitleOffset(sourceId, 0));
    int64_t duration = 0;
    check("getDuration()", pipeline->getDuration(duration));
    int64_t position = 0;
    check("getPosition()", pipeline->getPosition(position));

    cliLog("Advanced pipeline checks complete. final state: sync=" + std::string(boolText(sync)) +
           ", muted=" + boolText(muted) + ", volume=" + std::to_string(volume) +
           ", bufferingMs=" + std::to_string(bufferingMs) + ", streamSyncMode=" + std::to_string(streamSyncMode));
    destroyPipelineProbe(probe);
    return passed;
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
