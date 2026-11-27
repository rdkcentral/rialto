#include "NegativeChecks.h"

#include "CliLog.h"

#include "IClientLogControl.h"
#include "IClientLogHandler.h"
#include "IMediaKeysCapabilities.h"
#include "IMediaPipelineCapabilities.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace rialto_refplayer_cli
{

namespace
{

std::string toString(firebolt::rialto::ApplicationState state)
{
    switch (state)
    {
    case firebolt::rialto::ApplicationState::UNKNOWN: return "UNKNOWN";
    case firebolt::rialto::ApplicationState::RUNNING: return "RUNNING";
    case firebolt::rialto::ApplicationState::INACTIVE: return "INACTIVE";
    }
    return "UNKNOWN";
}

std::string toString(firebolt::rialto::PlaybackState state)
{
    switch (state)
    {
    case firebolt::rialto::PlaybackState::UNKNOWN: return "UNKNOWN";
    case firebolt::rialto::PlaybackState::IDLE: return "IDLE";
    case firebolt::rialto::PlaybackState::PLAYING: return "PLAYING";
    case firebolt::rialto::PlaybackState::PAUSED: return "PAUSED";
    case firebolt::rialto::PlaybackState::SEEKING: return "SEEKING";
    case firebolt::rialto::PlaybackState::SEEK_DONE: return "SEEK_DONE";
    case firebolt::rialto::PlaybackState::STOPPED: return "STOPPED";
    case firebolt::rialto::PlaybackState::END_OF_STREAM: return "END_OF_STREAM";
    case firebolt::rialto::PlaybackState::FAILURE: return "FAILURE";
    }
    return "UNKNOWN";
}

std::string toString(firebolt::rialto::NetworkState state)
{
    switch (state)
    {
    case firebolt::rialto::NetworkState::UNKNOWN: return "UNKNOWN";
    case firebolt::rialto::NetworkState::IDLE: return "IDLE";
    case firebolt::rialto::NetworkState::BUFFERING: return "BUFFERING";
    case firebolt::rialto::NetworkState::BUFFERING_PROGRESS: return "BUFFERING_PROGRESS";
    case firebolt::rialto::NetworkState::BUFFERED: return "BUFFERED";
    case firebolt::rialto::NetworkState::STALLED: return "STALLED";
    case firebolt::rialto::NetworkState::FORMAT_ERROR: return "FORMAT_ERROR";
    case firebolt::rialto::NetworkState::NETWORK_ERROR: return "NETWORK_ERROR";
    case firebolt::rialto::NetworkState::DECODE_ERROR: return "DECODE_ERROR";
    }
    return "UNKNOWN";
}

std::string toString(firebolt::rialto::WebAudioPlayerState state)
{
    switch (state)
    {
    case firebolt::rialto::WebAudioPlayerState::UNKNOWN: return "UNKNOWN";
    case firebolt::rialto::WebAudioPlayerState::IDLE: return "IDLE";
    case firebolt::rialto::WebAudioPlayerState::PLAYING: return "PLAYING";
    case firebolt::rialto::WebAudioPlayerState::PAUSED: return "PAUSED";
    case firebolt::rialto::WebAudioPlayerState::END_OF_STREAM: return "END_OF_STREAM";
    case firebolt::rialto::WebAudioPlayerState::FAILURE: return "FAILURE";
    }
    return "UNKNOWN";
}

std::string toString(ProbeState state)
{
    switch (state)
    {
    case ProbeState::CREATED: return "CREATED";
    case ProbeState::LOADED: return "LOADED";
    case ProbeState::PLAYING: return "PLAYING";
    case ProbeState::PAUSED: return "PAUSED";
    case ProbeState::STOPPED: return "STOPPED";
    case ProbeState::FAILED: return "FAILED";
    }
    return "UNKNOWN";
}

} // namespace

bool RawProbe::createControl()
{
    auto factory = firebolt::rialto::IControlFactory::createFactory();
    if (!factory)
    {
        cliLog("IControlFactory::createFactory() returned null");
        return false;
    }

    control = factory->createControl();
    if (!control)
    {
        cliLog("IControlFactory::createControl() failed");
        return false;
    }
    return true;
}

bool RawProbe::connectControl()
{
    if (!createControl())
    {
        return false;
    }
    firebolt::rialto::ApplicationState appState = firebolt::rialto::ApplicationState::UNKNOWN;
    const auto controlClient = std::static_pointer_cast<firebolt::rialto::IControlClient>(shared_from_this());
    if (!control->registerClient(controlClient, appState))
    {
        cliLog("registerClient() failed");
        return false;
    }

    cliLog("Registered control client; application state = " + toString(appState));
    return true;
}

bool RawProbe::createMediaPipeline(const firebolt::rialto::VideoRequirements &requirements)
{
    auto factory = firebolt::rialto::IMediaPipelineFactory::createFactory();
    if (!factory)
    {
        cliLog("IMediaPipelineFactory::createFactory() returned null");
        return false;
    }

    const auto self = std::static_pointer_cast<firebolt::rialto::IMediaPipelineClient>(shared_from_this());
    mediaPipeline = factory->createMediaPipeline(self, requirements);
    if (!mediaPipeline)
    {
        cliLog("IMediaPipelineFactory::createMediaPipeline() failed");
        return false;
    }

    cliLog("Media pipeline created");
    return true;
}

bool RawProbe::createWebAudio(uint32_t priority)
{
    auto factory = firebolt::rialto::IWebAudioPlayerFactory::createFactory();
    if (!factory)
    {
        cliLog("IWebAudioPlayerFactory::createFactory() returned null");
        return false;
    }

    firebolt::rialto::WebAudioConfig config{};
    config.pcm.rate = kWebAudioRate;
    config.pcm.channels = kWebAudioChannels;
    config.pcm.sampleSize = kWebAudioSampleSize;
    config.pcm.isBigEndian = false;
    config.pcm.isSigned = true;
    config.pcm.isFloat = false;

    // The player holds the config weakly, so the probe keeps it alive.
    webAudioConfig = std::make_shared<firebolt::rialto::WebAudioConfig>(config);
    const auto self = std::static_pointer_cast<firebolt::rialto::IWebAudioPlayerClient>(shared_from_this());
    const auto selfWeak = std::weak_ptr<firebolt::rialto::IWebAudioPlayerClient>(self);
    const auto configWeak = std::weak_ptr<const firebolt::rialto::WebAudioConfig>(webAudioConfig);
    webAudioPlayer = factory->createWebAudioPlayer(selfWeak, "audio/x-raw", priority, configWeak);
    if (!webAudioPlayer)
    {
        cliLog("IWebAudioPlayerFactory::createWebAudioPlayer() failed");
        return false;
    }

    cliLog("Web audio player created");
    return true;
}

void RawProbe::notifyApplicationState(firebolt::rialto::ApplicationState state)
{
    cliLog("notifyApplicationState: " + toString(state));
}

void RawProbe::notifyDuration(int64_t duration)
{
    cliLog("notifyDuration: " + std::to_string(duration));
}

void RawProbe::notifyPosition(int64_t position)
{
    (void)position;
}

void RawProbe::notifyNativeSize(uint32_t width, uint32_t height, double aspect)
{
    cliLog("notifyNativeSize: width=" + std::to_string(width) + ", height=" + std::to_string(height) +
           ", aspect=" + std::to_string(aspect));
}

void RawProbe::notifyNetworkState(firebolt::rialto::NetworkState state)
{
    cliLog("notifyNetworkState: " + toString(state));
}

void RawProbe::notifyPlaybackState(firebolt::rialto::PlaybackState state)
{
    cliLog("notifyPlaybackState: " + toString(state));
    switch (state)
    {
    case firebolt::rialto::PlaybackState::STOPPED:
    case firebolt::rialto::PlaybackState::END_OF_STREAM:
        setState(ProbeState::STOPPED, "Rialto reported " + toString(state));
        cliLog("Playback complete");
        break;
    case firebolt::rialto::PlaybackState::FAILURE:
        setState(ProbeState::FAILED, "Rialto reported FAILURE");
        break;
    default:
        break;
    }
}

void RawProbe::notifyVideoData(bool hasData)
{
    cliLog("notifyVideoData: " + std::string(hasData ? "true" : "false"));
}

void RawProbe::notifyAudioData(bool hasData)
{
    cliLog("notifyAudioData: " + std::string(hasData ? "true" : "false"));
}

void RawProbe::notifyNeedMediaData(int32_t sourceId, size_t frameCount, uint32_t needDataRequestId,
                                   const std::shared_ptr<firebolt::rialto::MediaPlayerShmInfo> &shmInfo)
{
    (void)sourceId;
    (void)frameCount;
    (void)needDataRequestId;
    (void)shmInfo;
}

void RawProbe::notifyCancelNeedMediaData(int32_t sourceId)
{
    cliLog("notifyCancelNeedMediaData: sourceId=" + std::to_string(sourceId));
}

void RawProbe::notifyQos(int32_t sourceId, const firebolt::rialto::QosInfo &qosInfo)
{
    cliLog("notifyQos: sourceId=" + std::to_string(sourceId) + ", processed=" + std::to_string(qosInfo.processed) +
           ", dropped=" + std::to_string(qosInfo.dropped));
}

void RawProbe::notifyBufferUnderflow(int32_t sourceId)
{
    cliLog("notifyBufferUnderflow: sourceId=" + std::to_string(sourceId));
}

void RawProbe::notifyFirstFrameReceived(int32_t sourceId)
{
    cliLog("notifyFirstFrameReceived: sourceId=" + std::to_string(sourceId));
}

void RawProbe::notifyPlaybackError(int32_t sourceId, firebolt::rialto::PlaybackError error)
{
    cliLog("notifyPlaybackError: sourceId=" + std::to_string(sourceId) +
           ", error=" + std::to_string(static_cast<int>(error)));
}

void RawProbe::notifySourceFlushed(int32_t sourceId)
{
    cliLog("notifySourceFlushed: sourceId=" + std::to_string(sourceId));
}

void RawProbe::notifyPlaybackInfo(const firebolt::rialto::PlaybackInfo &playbackInfo)
{
    (void)playbackInfo;
}

void RawProbe::notifyState(firebolt::rialto::WebAudioPlayerState state)
{
    cliLog("notifyState: " + toString(state));
}

void RawProbe::setState(ProbeState next, const std::string &reason)
{
    const ProbeState previous = m_playerState.exchange(next);
    cliLog("state " + toString(previous) + " -> " + toString(next) + " (" + reason + ")");
}

bool reportInitializationCheck(const std::string &name, const std::string &expected, const std::string &observed,
                               bool passed)
{
    cliLog(std::string("[NEG] ") + (passed ? "PASS " : "FAIL ") + name + ": expected=" + expected +
           " observed=" + observed);
    return passed;
}

std::shared_ptr<RawProbe> createPipelineProbe()
{
    auto probe = std::make_shared<RawProbe>();
    if (!probe->connectControl() || !probe->createMediaPipeline())
    {
        probe->mediaPipeline.reset();
        probe->control.reset();
        return nullptr;
    }
    return probe;
}

std::shared_ptr<RawProbe> createLoadedPipelineProbe(const std::string &mimeType = "video/mp4")
{
    auto probe = createPipelineProbe();
    if (!probe || !probe->mediaPipeline->load(firebolt::rialto::MediaType::MSE, mimeType, "blob:negative-test", false))
    {
        destroyPipelineProbe(probe);
        return nullptr;
    }
    return probe;
}

void destroyPipelineProbe(std::shared_ptr<RawProbe> &probe)
{
    if (probe)
    {
        probe->mediaPipeline.reset();
        probe->control.reset();
        probe.reset();
    }
}

std::shared_ptr<RawProbe> createAttachedPipelineProbe(const std::string &mimeType, bool withAudio)
{
    auto probe = createLoadedPipelineProbe(mimeType);
    if (!probe)
    {
        return nullptr;
    }

    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>(
            mimeType, false, 1920, 1080, firebolt::rialto::SegmentAlignment::UNDEFINED,
            firebolt::rialto::StreamFormat::UNDEFINED, nullptr);
    if (!probe->mediaPipeline->attachSource(source))
    {
        destroyPipelineProbe(probe);
        return nullptr;
    }
    probe->currentSourceId = source->getId();
    if (withAudio)
    {
        firebolt::rialto::AudioConfig config{};
        config.numberOfChannels = 2;
        config.sampleRate = 48000;
        config.codecSpecificConfig = {0x11, 0x90};
        std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> audioSource =
            std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>(
                "audio/mp4", false, config, firebolt::rialto::SegmentAlignment::AU);
        if (!probe->mediaPipeline->attachSource(audioSource))
        {
            destroyPipelineProbe(probe);
            return nullptr;
        }
        probe->audioSourceId = audioSource->getId();
    }
    if (!probe->mediaPipeline->allSourcesAttached())
    {
        destroyPipelineProbe(probe);
        return nullptr;
    }
    return probe;
}

/************************   Negative checks  *************************/
#ifndef REFERENCE_PLAYER_USE_MOCKS

std::size_t runCreateControlNegativeCases()
{
    std::size_t failures = 0;
    auto probe = std::make_shared<RawProbe>();
    const bool firstCreated = probe->createControl();
    const bool secondCreated = firstCreated && probe->createControl();
    failures += !reportInitializationCheck("createControl repeated creation", "second call returns",
                                           "first=" + std::string(firstCreated ? "created" : "failed") +
                                               ", second=" + (secondCreated ? "created" : "failed"),
                                           firstCreated);
    probe->control.reset();
    return failures;
}

std::size_t runRegisterClientNegativeCases()
{
    std::size_t failures = 0;
    auto probe = std::make_shared<RawProbe>();
    const bool controlCreated = probe->createControl();
    bool emptyAccepted = false;
    bool expiredAccepted = false;
    bool validAfterInvalidAccepted = false;
    if (controlCreated)
    {
        firebolt::rialto::ApplicationState appState = firebolt::rialto::ApplicationState::UNKNOWN;
        emptyAccepted = probe->control->registerClient({}, appState);

        std::weak_ptr<firebolt::rialto::IControlClient> expiredClient;
        {
            auto expiredOwner = std::make_shared<RawProbe>();
            expiredClient = std::static_pointer_cast<firebolt::rialto::IControlClient>(expiredOwner);
        }
        expiredAccepted = probe->control->registerClient(expiredClient, appState);

        const auto validClient = std::static_pointer_cast<firebolt::rialto::IControlClient>(probe);
        validAfterInvalidAccepted = probe->control->registerClient(validClient, appState);
    }
    failures += !reportInitializationCheck("registerClient empty client", "reject",
                                           controlCreated ? (emptyAccepted ? "accepted" : "rejected")
                                                          : "control creation failed",
                                           controlCreated && !emptyAccepted);
    failures += !reportInitializationCheck("registerClient expired client", "reject",
                                           controlCreated ? (expiredAccepted ? "accepted" : "rejected")
                                                          : "control creation failed",
                                           controlCreated && !expiredAccepted);
    failures += !reportInitializationCheck("registerClient recovery", "accept valid client after invalid clients",
                                           controlCreated ? (validAfterInvalidAccepted ? "accepted" : "rejected")
                                                          : "control creation failed",
                                           controlCreated && validAfterInvalidAccepted);
    probe->control.reset();

    auto repeatProbe = std::make_shared<RawProbe>();
    const bool repeatControlCreated = repeatProbe->createControl();
    bool firstRegistration = false;
    bool duplicateRegistration = false;
    bool secondClientRegistration = false;
    std::shared_ptr<RawProbe> secondClient;
    if (repeatControlCreated)
    {
        firebolt::rialto::ApplicationState appState = firebolt::rialto::ApplicationState::UNKNOWN;
        const auto client = std::static_pointer_cast<firebolt::rialto::IControlClient>(repeatProbe);
        firstRegistration = repeatProbe->control->registerClient(client, appState);
        duplicateRegistration = repeatProbe->control->registerClient(client, appState);
        secondClient = std::make_shared<RawProbe>();
        const auto anotherClient = std::static_pointer_cast<firebolt::rialto::IControlClient>(secondClient);
        secondClientRegistration = repeatProbe->control->registerClient(anotherClient, appState);
    }
    failures += !reportInitializationCheck(
        "registerClient duplicate and second client", "calls return; observe registration results",
        repeatControlCreated
            ? "first=" + std::string(firstRegistration ? "accepted" : "rejected") +
                  ", duplicate=" + (duplicateRegistration ? "accepted" : "rejected") +
                  ", second=" + (secondClientRegistration ? "accepted" : "rejected")
            : "control creation failed",
        repeatControlCreated && firstRegistration);
    repeatProbe->control.reset();
    repeatProbe.reset();
    secondClient.reset();
    return failures;
}

std::size_t runCreateMediaPipelineNegativeCases()
{
    std::size_t failures = 0;
    auto factory = firebolt::rialto::IMediaPipelineFactory::createFactory();
    std::unique_ptr<firebolt::rialto::IMediaPipeline> pipelineForExpiredClient;
    if (factory)
    {
        const std::weak_ptr<firebolt::rialto::IMediaPipelineClient> expiredClient;
        pipelineForExpiredClient = factory->createMediaPipeline(
            expiredClient, firebolt::rialto::VideoRequirements{1920, 1080});
    }
    failures += !reportInitializationCheck("createMediaPipeline expired client", "reject",
                                           factory ? (pipelineForExpiredClient ? "accepted" : "rejected")
                                                   : "factory unavailable",
                                           factory && !pipelineForExpiredClient);
    pipelineForExpiredClient.reset();

    auto invalidRequirementsProbe = std::make_shared<RawProbe>();
    const bool requirementsControlConnected = invalidRequirementsProbe->connectControl();
    const bool zeroRequirementsAccepted = requirementsControlConnected &&
        invalidRequirementsProbe->createMediaPipeline(firebolt::rialto::VideoRequirements{0, 0});
    failures += !reportInitializationCheck("createMediaPipeline zero dimensions", "return; observe result",
                                           requirementsControlConnected
                                               ? (zeroRequirementsAccepted ? "accepted" : "rejected")
                                               : "control setup failed",
                                           requirementsControlConnected);
    invalidRequirementsProbe->mediaPipeline.reset();
    invalidRequirementsProbe->control.reset();

    auto repeatedProbe = createPipelineProbe();
    const bool repeatedCreated = repeatedProbe && repeatedProbe->createMediaPipeline();
    failures += !reportInitializationCheck("createMediaPipeline repeated creation", "second call returns",
                                           repeatedProbe ? (repeatedCreated ? "created" : "rejected")
                                                         : "pipeline setup failed",
                                           static_cast<bool>(repeatedProbe));
    destroyPipelineProbe(repeatedProbe);
    return failures;
}

std::size_t runLoadNegativeCases()
{
    std::size_t failures = 0;
    const auto runRejectedLoad = [&failures](const std::string &name, firebolt::rialto::MediaType type,
                                                   const std::string &mimeType, const std::string &url)
    {
        auto probe = createPipelineProbe();
        const bool loaded = probe && probe->mediaPipeline->load(type, mimeType, url, false);
        failures += !reportInitializationCheck(name, "reject invalid media description",
                                               probe ? (loaded ? "accepted" : "rejected") : "pipeline setup failed",
                                               probe && !loaded);
        destroyPipelineProbe(probe);
    };
    runRejectedLoad("load unknown media type", firebolt::rialto::MediaType::UNKNOWN, "video/mp4",
                    "blob:negative-test");
    runRejectedLoad("load empty MIME type", firebolt::rialto::MediaType::MSE, "", "blob:negative-test");
    runRejectedLoad("load empty URL", firebolt::rialto::MediaType::MSE, "video/mp4", "");
    runRejectedLoad("load unsupported MIME type", firebolt::rialto::MediaType::MSE, "video/unknown",
                    "blob:negative-test");

    auto repeatedProbe = createPipelineProbe();
    bool firstLoad = false;
    bool secondLoad = false;
    if (repeatedProbe)
    {
        firstLoad = repeatedProbe->mediaPipeline->load(firebolt::rialto::MediaType::MSE, "video/mp4",
                                                           "blob:negative-test", false);
        if (firstLoad)
        {
            secondLoad = repeatedProbe->mediaPipeline->load(firebolt::rialto::MediaType::MSE, "video/mp4",
                                                               "blob:negative-test", false);
        }
    }
    failures += !reportInitializationCheck(
        "load repeated call", "first load succeeds; second call returns",
        repeatedProbe ? "first=" + std::string(firstLoad ? "accepted" : "rejected") +
                            ", second=" + (secondLoad ? "accepted" : "rejected")
                      : "pipeline setup failed",
        repeatedProbe && firstLoad);
    destroyPipelineProbe(repeatedProbe);
    return failures;
}

std::size_t runAttachSourceNegativeCases()
{
    using MediaSource = firebolt::rialto::IMediaPipeline::MediaSource;
    using MediaSourceVideo = firebolt::rialto::IMediaPipeline::MediaSourceVideo;
    std::size_t failures = 0;
    const auto makeVideo = [](const std::string &mimeType, int32_t width, int32_t height)
        -> std::unique_ptr<MediaSource>
    {
        return std::make_unique<MediaSourceVideo>(mimeType, false, width, height);
    };
    const auto makeAudio = [](uint32_t channels, uint32_t sampleRate) -> std::unique_ptr<MediaSource>
    {
        firebolt::rialto::AudioConfig config{};
        config.numberOfChannels = channels;
        config.sampleRate = sampleRate;
        config.codecSpecificConfig = {0x11, 0x90};
        return std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>(
            "audio/mp4", false, config, firebolt::rialto::SegmentAlignment::AU);
    };
    const auto runRejectedAttach = [&failures](const std::string &name, bool loadFirst,
                                                      std::unique_ptr<MediaSource> source)
    {
        auto probe = createPipelineProbe();
        const bool loaded = probe && (!loadFirst || probe->mediaPipeline->load(
                                                         firebolt::rialto::MediaType::MSE, "video/mp4",
                                                         "blob:negative-test", false));
        const bool attached = loaded && probe->mediaPipeline->attachSource(source);
        failures += !reportInitializationCheck(name, "reject invalid source or lifecycle order",
                                               loaded ? (attached ? "accepted" : "rejected")
                                                      : "pipeline/load setup failed",
                                               loaded && !attached);
        destroyPipelineProbe(probe);
    };

    runRejectedAttach("attachSource before load", false, makeVideo("video/mp4", 1920, 1080));
    runRejectedAttach("attachSource null source", true, nullptr);
    runRejectedAttach("attachSource empty MIME type", true, makeVideo("", 1920, 1080));
    runRejectedAttach("attachSource zero video dimensions", true, makeVideo("video/mp4", 0, 0));
    runRejectedAttach("attachSource negative video dimensions", true, makeVideo("video/mp4", -1, -1));
    runRejectedAttach("attachSource zero audio channels", true, makeAudio(0, 48000));
    runRejectedAttach("attachSource zero audio sample rate", true, makeAudio(2, 0));

    auto duplicateProbe = createPipelineProbe();
    bool loaded = duplicateProbe && duplicateProbe->mediaPipeline->load(
                                       firebolt::rialto::MediaType::MSE, "video/mp4", "blob:negative-test", false);
    bool firstAttached = false;
    bool duplicateAttached = false;
    if (loaded)
    {
        auto firstSource = makeVideo("video/mp4", 1920, 1080);
        auto duplicateSource = makeVideo("video/mp4", 1280, 720);
        firstAttached = duplicateProbe->mediaPipeline->attachSource(firstSource);
        duplicateAttached = firstAttached && duplicateProbe->mediaPipeline->attachSource(duplicateSource);
    }
    failures += !reportInitializationCheck(
        "attachSource duplicate video", "reject second video source",
        loaded ? "first=" + std::string(firstAttached ? "accepted" : "rejected") +
                     ", duplicate=" + (duplicateAttached ? "accepted" : "rejected")
               : "pipeline/load setup failed",
        loaded && firstAttached && !duplicateAttached);
    destroyPipelineProbe(duplicateProbe);

    auto afterAttachedProbe = createPipelineProbe();
    bool afterLoad = afterAttachedProbe && afterAttachedProbe->mediaPipeline->load(
                                              firebolt::rialto::MediaType::MSE, "video/mp4",
                                              "blob:negative-test", false);
    bool sourceAttached = false;
    bool sourcesFinalized = false;
    bool lateSourceAttached = false;
    if (afterLoad)
    {
        auto firstSource = makeVideo("video/mp4", 1920, 1080);
        sourceAttached = afterAttachedProbe->mediaPipeline->attachSource(firstSource);
        sourcesFinalized = sourceAttached && afterAttachedProbe->mediaPipeline->allSourcesAttached();
        auto lateSource = makeAudio(2, 48000);
        lateSourceAttached = sourcesFinalized && afterAttachedProbe->mediaPipeline->attachSource(lateSource);
    }
    failures += !reportInitializationCheck(
        "attachSource after allSourcesAttached", "reject source after source list is finalized",
        afterLoad ? "first=" + std::string(sourceAttached ? "attached" : "rejected") +
                        ", finalized=" + (sourcesFinalized ? "true" : "false") +
                        ", late=" + (lateSourceAttached ? "accepted" : "rejected")
                  : "pipeline/load setup failed",
        afterLoad && sourceAttached && sourcesFinalized && !lateSourceAttached);
    destroyPipelineProbe(afterAttachedProbe);
    return failures;
}

std::size_t runSourceIdChecks()
{
    using MediaSource = firebolt::rialto::IMediaPipeline::MediaSource;
    using MediaSourceVideo = firebolt::rialto::IMediaPipeline::MediaSourceVideo;
    using MediaSourceAudio = firebolt::rialto::IMediaPipeline::MediaSourceAudio;
    std::size_t failures = 0;
    auto probe = createPipelineProbe();
    const bool loaded = probe && probe->mediaPipeline->load(
                                     firebolt::rialto::MediaType::MSE, "video/mp4", "blob:negative-test", false);
    std::unique_ptr<MediaSource> source = std::make_unique<MediaSourceVideo>("video/mp4", false, 1920, 1080);
    const int32_t initialId = source->getId();
    const bool attached = loaded && probe->mediaPipeline->attachSource(source);
    const int32_t attachedId = source->getId();
    failures += !reportInitializationCheck("MediaSource getId before/after attach", "starts at 0; attach assigns ID",
                                           "initial=" + std::to_string(initialId) +
                                               ", attach=" + (attached ? "success" : "failure") +
                                               ", after=" + std::to_string(attachedId),
                                           loaded && initialId == 0 && attached && attachedId != initialId);
    firebolt::rialto::AudioConfig audioConfig{};
    audioConfig.numberOfChannels = 2;
    audioConfig.sampleRate = 48000;
    audioConfig.codecSpecificConfig = {0x11, 0x90};
    std::unique_ptr<MediaSource> audioSource = std::make_unique<MediaSourceAudio>(
        "audio/mp4", false, audioConfig, firebolt::rialto::SegmentAlignment::AU);
    const bool audioAttached = attached && probe->mediaPipeline->attachSource(audioSource);
    const int32_t audioId = audioSource->getId();
    failures += !reportInitializationCheck("MediaSource video/audio IDs", "successful sources have distinct IDs",
                                           "video=" + std::to_string(attachedId) +
                                               ", audio=" + std::to_string(audioId) +
                                               ", audio attached=" + (audioAttached ? "true" : "false"),
                                           attached && audioAttached && attachedId != audioId);
    destroyPipelineProbe(probe);

    auto rejectedProbe = createPipelineProbe();
    const bool rejectedLoad = rejectedProbe && rejectedProbe->mediaPipeline->load(
                                                  firebolt::rialto::MediaType::MSE, "video/mp4",
                                                  "blob:negative-test", false);
    std::unique_ptr<MediaSource> rejectedSource = std::make_unique<MediaSourceVideo>("video/mp4", false, 0, 0);
    const int32_t idBeforeRejectedAttach = rejectedSource->getId();
    const bool rejectedAttach = rejectedLoad && rejectedProbe->mediaPipeline->attachSource(rejectedSource);
    const int32_t idAfterRejectedAttach = rejectedSource->getId();
    failures += !reportInitializationCheck("MediaSource getId after rejected attach", "remains unchanged",
                                           "before=" + std::to_string(idBeforeRejectedAttach) +
                                               ", attach=" + (rejectedAttach ? "accepted" : "rejected") +
                                               ", after=" + std::to_string(idAfterRejectedAttach),
                                           rejectedLoad && !rejectedAttach &&
                                               idBeforeRejectedAttach == idAfterRejectedAttach);
    destroyPipelineProbe(rejectedProbe);
    return failures;
}

std::size_t runAllSourcesAttachedNegativeCases()
{
    using MediaSource = firebolt::rialto::IMediaPipeline::MediaSource;
    using MediaSourceVideo = firebolt::rialto::IMediaPipeline::MediaSourceVideo;
    std::size_t failures = 0;

    auto beforeLoadProbe = createPipelineProbe();
    const bool beforeLoad = beforeLoadProbe && beforeLoadProbe->mediaPipeline->allSourcesAttached();
    failures += !reportInitializationCheck("allSourcesAttached before load", "reject", beforeLoadProbe
                                                                                       ? (beforeLoad ? "accepted" : "rejected")
                                                                                       : "pipeline setup failed",
                                           beforeLoadProbe && !beforeLoad);
    destroyPipelineProbe(beforeLoadProbe);

    auto noSourcesProbe = createPipelineProbe();
    const bool noSourcesLoaded = noSourcesProbe && noSourcesProbe->mediaPipeline->load(
                                                       firebolt::rialto::MediaType::MSE, "video/mp4",
                                                       "blob:negative-test", false);
    const bool noSourcesFinalized = noSourcesLoaded && noSourcesProbe->mediaPipeline->allSourcesAttached();
    failures += !reportInitializationCheck("allSourcesAttached with no sources", "reject",
                                           noSourcesLoaded ? (noSourcesFinalized ? "accepted" : "rejected")
                                                           : "pipeline/load setup failed",
                                           noSourcesLoaded && !noSourcesFinalized);
    destroyPipelineProbe(noSourcesProbe);

    auto afterRejectedAttachProbe = createPipelineProbe();
    const bool failedAttachLoaded = afterRejectedAttachProbe && afterRejectedAttachProbe->mediaPipeline->load(
                                                                 firebolt::rialto::MediaType::MSE, "video/mp4",
                                                                 "blob:negative-test", false);
    std::unique_ptr<MediaSource> invalidSource = std::make_unique<MediaSourceVideo>("video/mp4", false, 0, 0);
    const bool invalidSourceAttached = failedAttachLoaded &&
        afterRejectedAttachProbe->mediaPipeline->attachSource(invalidSource);
    const bool finalizedAfterRejectedAttach = failedAttachLoaded && !invalidSourceAttached &&
        afterRejectedAttachProbe->mediaPipeline->allSourcesAttached();
    failures += !reportInitializationCheck("allSourcesAttached after rejected attach", "reject incomplete source set",
                                           failedAttachLoaded
                                               ? (finalizedAfterRejectedAttach ? "accepted" : "rejected")
                                               : "pipeline/load setup failed",
                                           failedAttachLoaded && !invalidSourceAttached &&
                                               !finalizedAfterRejectedAttach);
    destroyPipelineProbe(afterRejectedAttachProbe);

    auto repeatedProbe = createPipelineProbe();
    const bool repeatedLoaded = repeatedProbe && repeatedProbe->mediaPipeline->load(
                                                   firebolt::rialto::MediaType::MSE, "video/mp4",
                                                   "blob:negative-test", false);
    bool sourceAttached = false;
    bool firstFinalization = false;
    bool secondFinalization = false;
    if (repeatedLoaded)
    {
        std::unique_ptr<MediaSource> source = std::make_unique<MediaSourceVideo>("video/mp4", false, 1920, 1080);
        sourceAttached = repeatedProbe->mediaPipeline->attachSource(source);
        firstFinalization = sourceAttached && repeatedProbe->mediaPipeline->allSourcesAttached();
        secondFinalization = firstFinalization && repeatedProbe->mediaPipeline->allSourcesAttached();
    }
    failures += !reportInitializationCheck(
        "allSourcesAttached repeated call", "first succeeds; second is rejected",
        repeatedLoaded ? "attached=" + std::string(sourceAttached ? "true" : "false") +
                             ", first=" + (firstFinalization ? "true" : "false") +
                             ", second=" + (secondFinalization ? "true" : "false")
                       : "pipeline/load setup failed",
        repeatedLoaded && sourceAttached && firstFinalization && !secondFinalization);
    destroyPipelineProbe(repeatedProbe);
    return failures;
}

std::size_t runPlaybackControlNegativeCases()
{
    std::size_t failures = 0;
    const auto observeUnloadedCall = [&failures](const std::string &name, const auto &call)
    {
        auto probe = createPipelineProbe();
        if (!probe)
        {
            failures += !reportInitializationCheck(name, "call returns", "pipeline setup failed", false);
            return;
        }
        const bool result = call(*probe);
        failures += !reportInitializationCheck(name, "call returns; observe boolean",
                                               result ? "true" : "false", true);
        destroyPipelineProbe(probe);
    };

    observeUnloadedCall("play before load", [](RawProbe &probe)
                        {
                            bool async = false;
                            return probe.mediaPipeline->play(async);
                        });
    observeUnloadedCall("pause before play", [](RawProbe &probe) { return probe.mediaPipeline->pause(); });
    observeUnloadedCall("stop before play", [](RawProbe &probe) { return probe.mediaPipeline->stop(); });
    observeUnloadedCall("setPlaybackRate zero", [](RawProbe &probe)
                        { return probe.mediaPipeline->setPlaybackRate(0.0); });
    observeUnloadedCall("setPlaybackRate negative", [](RawProbe &probe)
                        { return probe.mediaPipeline->setPlaybackRate(-2.0); });
    observeUnloadedCall("setPosition negative", [](RawProbe &probe)
                        { return probe.mediaPipeline->setPosition(-1); });
    observeUnloadedCall("setPosition out of range", [](RawProbe &probe)
                        { return probe.mediaPipeline->setPosition(std::numeric_limits<int64_t>::max()); });
    observeUnloadedCall("setVolume below range", [](RawProbe &probe)
                        { return probe.mediaPipeline->setVolume(-0.1); });
    observeUnloadedCall("setVolume above range", [](RawProbe &probe)
                        { return probe.mediaPipeline->setVolume(1.5); });
    observeUnloadedCall("getPosition before load", [](RawProbe &probe)
                        {
                            int64_t position = -1;
                            return probe.mediaPipeline->getPosition(position);
                        });
    return failures;
}

std::size_t runPipelinePropertyNegativeCases()
{
    std::size_t failures = 0;
    const auto observe = [&failures](const std::string &name, const auto &call)
    {
    auto probe = createAttachedPipelineProbe();
    if (!probe)
    {
        failures += !reportInitializationCheck(name, "call returns", "pipeline setup failed", false);
        return;
    }
    const bool result = call(*probe);
    failures += !reportInitializationCheck(name, "call returns; observe boolean",
                           result ? "true" : "false", true);
    destroyPipelineProbe(probe);
    };

    observe("setLowLatency before play", [](RawProbe &probe)
        { return probe.mediaPipeline->setLowLatency(true); });
    observe("setSync before play", [](RawProbe &probe) { return probe.mediaPipeline->setSync(true); });
    observe("getSync before play", [](RawProbe &probe)
        {
        bool sync = false;
        return probe.mediaPipeline->getSync(sync);
        });
    observe("setSyncOff before play", [](RawProbe &probe) { return probe.mediaPipeline->setSyncOff(true); });
    observe("setMute invalid source", [](RawProbe &probe)
        { return probe.mediaPipeline->setMute(-1, true); });
    observe("getMute invalid source", [](RawProbe &probe)
        {
        bool mute = false;
        return probe.mediaPipeline->getMute(-1, mute);
        });
    observe("setVolume below range", [](RawProbe &probe) { return probe.mediaPipeline->setVolume(-0.1); });
    observe("setVolume above range", [](RawProbe &probe) { return probe.mediaPipeline->setVolume(1.5); });
    observe("getVolume before play", [](RawProbe &probe)
        {
        double volume = 0.0;
        return probe.mediaPipeline->getVolume(volume);
        });
    observe("setTextTrackIdentifier empty", [](RawProbe &probe)
        { return probe.mediaPipeline->setTextTrackIdentifier(""); });
    observe("setTextTrackIdentifier long", [](RawProbe &probe)
        { return probe.mediaPipeline->setTextTrackIdentifier(std::string(4096, 'x')); });
    observe("getTextTrackIdentifier before play", [](RawProbe &probe)
        {
        std::string textTrack;
        return probe.mediaPipeline->getTextTrackIdentifier(textTrack);
        });
    observe("setBufferingLimit zero", [](RawProbe &probe) { return probe.mediaPipeline->setBufferingLimit(0); });
    observe("getBufferingLimit before play", [](RawProbe &probe)
        {
        uint32_t limit = 0;
        return probe.mediaPipeline->getBufferingLimit(limit);
        });
    observe("setUseBuffering before play", [](RawProbe &probe)
        { return probe.mediaPipeline->setUseBuffering(true); });
    observe("getUseBuffering before play", [](RawProbe &probe)
        {
        bool useBuffering = false;
        return probe.mediaPipeline->getUseBuffering(useBuffering);
        });
    observe("setStreamSyncMode invalid source/mode", [](RawProbe &probe)
        { return probe.mediaPipeline->setStreamSyncMode(-1, -1); });
    observe("getStreamSyncMode before play", [](RawProbe &probe)
        {
        int32_t streamSyncMode = 0;
        return probe.mediaPipeline->getStreamSyncMode(streamSyncMode);
        });
    observe("setSourcePosition invalid source/rate", [](RawProbe &probe)
        { return probe.mediaPipeline->setSourcePosition(-1, -1, true, 0.0, 0); });
    observe("setSubtitleOffset invalid source/position", [](RawProbe &probe)
        { return probe.mediaPipeline->setSubtitleOffset(-1, -1); });
    observe("getDuration before play", [](RawProbe &probe)
        {
        int64_t duration = 0;
        return probe.mediaPipeline->getDuration(duration);
        });
    observe("getPosition before play", [](RawProbe &probe)
        {
        int64_t position = 0;
        return probe.mediaPipeline->getPosition(position);
        });
    return failures;
}

std::size_t runSegmentSubmissionNegativeCases()
{
    std::size_t failures = 0;
    using MediaSegment = firebolt::rialto::IMediaPipeline::MediaSegment;
    using MediaSegmentVideo = firebolt::rialto::IMediaPipeline::MediaSegmentVideo;
    const uint32_t staleRequestId = std::numeric_limits<uint32_t>::max();

    auto emptySegmentProbe = createAttachedPipelineProbe();
    std::unique_ptr<MediaSegment> emptySegment;
    const auto emptySegmentResult = emptySegmentProbe
        ? emptySegmentProbe->mediaPipeline->addSegment(staleRequestId, emptySegment)
        : firebolt::rialto::AddSegmentStatus::ERROR;
    failures += !reportInitializationCheck("addSegment empty segment/stale request", "reject segment",
                                           "status=" + std::to_string(static_cast<int>(emptySegmentResult)),
                                           emptySegmentProbe && emptySegmentResult != firebolt::rialto::AddSegmentStatus::OK);
    destroyPipelineProbe(emptySegmentProbe);

    auto invalidSegmentProbe = createAttachedPipelineProbe();
    std::unique_ptr<MediaSegment> invalidSegment =
        std::make_unique<MediaSegmentVideo>(-1, -1, -1, 0, 0);
    const auto invalidSegmentResult = invalidSegmentProbe
        ? invalidSegmentProbe->mediaPipeline->addSegment(staleRequestId, invalidSegment)
        : firebolt::rialto::AddSegmentStatus::ERROR;
    failures += !reportInitializationCheck("addSegment invalid source/stale request", "reject segment",
                                           "status=" + std::to_string(static_cast<int>(invalidSegmentResult)),
                                           invalidSegmentProbe && invalidSegmentResult != firebolt::rialto::AddSegmentStatus::OK);
    destroyPipelineProbe(invalidSegmentProbe);

    const auto observeStaleHaveData = [&failures, staleRequestId](const std::string &name,
                                                                       firebolt::rialto::MediaSourceStatus status)
    {
        auto probe = createAttachedPipelineProbe();
        const bool result = probe && probe->mediaPipeline->haveData(status, staleRequestId);
        failures += !reportInitializationCheck(name, "reject stale request", probe
                                                                               ? (result ? "accepted" : "rejected")
                                                                               : "pipeline setup failed",
                                               probe && !result);
        destroyPipelineProbe(probe);
    };
    observeStaleHaveData("haveData stale request with ERROR", firebolt::rialto::MediaSourceStatus::ERROR);
    observeStaleHaveData("haveData stale request with EOS", firebolt::rialto::MediaSourceStatus::EOS);
    return failures;
}

std::size_t runCapabilityNegativeCases()
{
    std::size_t failures = 0;
    auto pipelineFactory = firebolt::rialto::IMediaPipelineCapabilitiesFactory::createFactory();
    if (!pipelineFactory)
    {
        cliLog("[NEG] SKIP capability cases: media pipeline capabilities factory unavailable");
        return failures;
    }
    auto capabilities = pipelineFactory->createMediaPipelineCapabilities();
    if (!capabilities)
    {
        failures += !reportInitializationCheck("createMediaPipelineCapabilities", "create object",
                                               "returned null", false);
        return failures;
    }

    const auto observeMimeList = [&failures, &capabilities](firebolt::rialto::MediaSourceType sourceType,
                                                                  const std::string &name)
    {
        if (!capabilities)
        {
            failures += !reportInitializationCheck(name, "return safely", "capabilities unavailable", false);
            return;
        }
        const auto mimeTypes = capabilities->getSupportedMimeTypes(sourceType);
        reportInitializationCheck(name, "return a list", std::to_string(mimeTypes.size()) + " MIME type(s)", true);
    };
    observeMimeList(firebolt::rialto::MediaSourceType::UNKNOWN, "getSupportedMimeTypes unknown source");
    observeMimeList(firebolt::rialto::MediaSourceType::SUBTITLE, "getSupportedMimeTypes subtitle source");

    const bool emptyMimeSupported = capabilities->isMimeTypeSupported("");
    failures += !reportInitializationCheck("isMimeTypeSupported empty MIME", "unsupported",
                                           emptyMimeSupported ? "supported" : "unsupported", !emptyMimeSupported);
    const bool unknownMimeSupported = capabilities->isMimeTypeSupported("video/unknown");
    failures += !reportInitializationCheck("isMimeTypeSupported unknown MIME", "unsupported",
                                           unknownMimeSupported ? "supported" : "unsupported", !unknownMimeSupported);
    const std::string longMime(4096, 'x');
    const bool longMimeSupported = capabilities->isMimeTypeSupported(longMime);
    failures += !reportInitializationCheck("isMimeTypeSupported long MIME", "unsupported",
                                           longMimeSupported ? "supported" : "unsupported", !longMimeSupported);

    const std::vector<std::string> requestedProperties{"", "not-a-property", "volume"};
    const auto returnedProperties = capabilities->getSupportedProperties(
        firebolt::rialto::MediaSourceType::UNKNOWN, requestedProperties);
    const bool propertiesAreSubset = std::all_of(returnedProperties.begin(), returnedProperties.end(),
                                                  [&requestedProperties](const std::string &property)
                                                  {
                                                      return std::find(requestedProperties.begin(),
                                                                       requestedProperties.end(), property) !=
                                                             requestedProperties.end();
                                                  });
    failures += !reportInitializationCheck("getSupportedProperties unknown type/names", "return requested subset",
                                           std::to_string(returnedProperties.size()) + " property(ies)",
                                           propertiesAreSubset);
    bool isVideoMaster = false;
    const bool videoMasterQuery = capabilities->isVideoMaster(isVideoMaster);
    failures += !reportInitializationCheck("isVideoMaster", "return without error",
                                           videoMasterQuery ? (isVideoMaster ? "true" : "false") : "failed",
                                           videoMasterQuery);

    auto keysFactory = firebolt::rialto::IMediaKeysCapabilitiesFactory::createFactory();
    auto keyCapabilities = keysFactory ? keysFactory->getMediaKeysCapabilities() : nullptr;
    if (!keyCapabilities)
    {
        cliLog("[NEG] SKIP key-system capabilities: factory or capabilities unavailable");
    }
    else
    {
        const auto systems = keyCapabilities->getSupportedKeySystems();
        reportInitializationCheck("getSupportedKeySystems", "return a list",
                                  std::to_string(systems.size()) + " key system(s)", true);
    }
    return failures;
}

std::size_t runWebAudioNegativeCases()
{
    std::size_t failures = 0;
    auto factory = firebolt::rialto::IWebAudioPlayerFactory::createFactory();
    if (!factory)
    {
        cliLog("[NEG] SKIP WebAudio cases: factory unavailable");
        return failures;
    }

    auto config = std::make_shared<firebolt::rialto::WebAudioConfig>();
    config->pcm.rate = 48000;
    config->pcm.channels = 2;
    config->pcm.sampleSize = 16;
    config->pcm.isBigEndian = false;
    config->pcm.isSigned = true;
    config->pcm.isFloat = false;
    const std::weak_ptr<const firebolt::rialto::WebAudioConfig> validConfig = config;

    auto probe = std::make_shared<RawProbe>();
    const auto client = std::static_pointer_cast<firebolt::rialto::IWebAudioPlayerClient>(probe);
    const std::weak_ptr<firebolt::rialto::IWebAudioPlayerClient> validClient = client;
    const std::weak_ptr<firebolt::rialto::IWebAudioPlayerClient> emptyClient;

    auto emptyClientPlayer = factory->createWebAudioPlayer(emptyClient, "audio/x-raw", 1, validConfig);
    failures += !reportInitializationCheck("createWebAudioPlayer empty client", "reject",
                                           emptyClientPlayer ? "created" : "rejected", !emptyClientPlayer);
    emptyClientPlayer.reset();

    std::weak_ptr<firebolt::rialto::IWebAudioPlayerClient> expiredClient;
    {
        auto expiredOwner = std::make_shared<RawProbe>();
        expiredClient = std::static_pointer_cast<firebolt::rialto::IWebAudioPlayerClient>(expiredOwner);
    }
    auto expiredClientPlayer = factory->createWebAudioPlayer(expiredClient, "audio/x-raw", 1, validConfig);
    failures += !reportInitializationCheck("createWebAudioPlayer expired client", "reject",
                                           expiredClientPlayer ? "created" : "rejected", !expiredClientPlayer);
    expiredClientPlayer.reset();

    auto unsupportedMimePlayer = factory->createWebAudioPlayer(validClient, "audio/unknown", 1, validConfig);
    failures += !reportInitializationCheck("createWebAudioPlayer unsupported MIME", "reject",
                                           unsupportedMimePlayer ? "created" : "rejected", !unsupportedMimePlayer);
    unsupportedMimePlayer.reset();

    std::weak_ptr<const firebolt::rialto::WebAudioConfig> emptyConfig;
    auto noConfigPlayer = factory->createWebAudioPlayer(validClient, "audio/x-raw", 1, emptyConfig);
    reportInitializationCheck("createWebAudioPlayer no config", "observe nullable config behavior",
                              noConfigPlayer ? "created" : "rejected", true);
    noConfigPlayer.reset();

    probe->webAudioPlayer = factory->createWebAudioPlayer(validClient, "audio/x-raw", 1, validConfig);
    if (!probe->webAudioPlayer)
    {
        failures += !reportInitializationCheck("createWebAudioPlayer valid setup", "create player", "returned null",
                                               false);
        return failures;
    }

    const bool negativeVolumeAccepted = probe->webAudioPlayer->setVolume(-0.1);
    failures += !reportInitializationCheck("WebAudio setVolume below range", "reject",
                                           negativeVolumeAccepted ? "accepted" : "rejected",
                                           !negativeVolumeAccepted);
    const bool excessiveVolumeAccepted = probe->webAudioPlayer->setVolume(1.5);
    failures += !reportInitializationCheck("WebAudio setVolume above range", "reject",
                                           excessiveVolumeAccepted ? "accepted" : "rejected",
                                           !excessiveVolumeAccepted);

    double volume = 0.0;
    const bool gotVolume = probe->webAudioPlayer->getVolume(volume);
    reportInitializationCheck("WebAudio getVolume after invalid values", "return safely",
                              gotVolume ? std::to_string(volume) : "failed", true);

    uint32_t preferredFrames = 0;
    uint32_t maximumFrames = 0;
    bool deferredPlay = false;
    const bool gotDeviceInfo = probe->webAudioPlayer->getDeviceInfo(preferredFrames, maximumFrames, deferredPlay);
    reportInitializationCheck("WebAudio getDeviceInfo", "return safely",
                              gotDeviceInfo ? std::to_string(preferredFrames) + "/" +
                                                  std::to_string(maximumFrames)
                                            : "failed",
                              true);

    uint32_t availableFrames = 0;
    uint32_t delayFrames = 0;
    auto shmInfo = std::make_shared<firebolt::rialto::WebAudioShmInfo>();
    const bool gotBufferInfo = probe->webAudioPlayer->getBufferAvailable(availableFrames, shmInfo);
    reportInitializationCheck("WebAudio getBufferAvailable before play", "return safely",
                              gotBufferInfo ? std::to_string(availableFrames) + " frame(s)" : "failed", true);
    const bool gotDelay = probe->webAudioPlayer->getBufferDelay(delayFrames);
    reportInitializationCheck("WebAudio getBufferDelay before play", "return safely",
                              gotDelay ? std::to_string(delayFrames) + " frame(s)" : "failed", true);

    const bool pausedBeforePlay = probe->webAudioPlayer->pause();
    reportInitializationCheck("WebAudio pause before play", "return safely",
                              pausedBeforePlay ? "true" : "false", true);
    const bool played = probe->webAudioPlayer->play();
    reportInitializationCheck("WebAudio play", "return safely", played ? "true" : "false", true);
    const bool eos = probe->webAudioPlayer->setEos();
    reportInitializationCheck("WebAudio setEos", "return safely", eos ? "true" : "false", true);
    const bool repeatedEos = probe->webAudioPlayer->setEos();
    reportInitializationCheck("WebAudio repeated setEos", "return safely", repeatedEos ? "true" : "false", true);

    uint8_t emptyData = 0;
    const bool zeroFrameWrite = probe->webAudioPlayer->writeBuffer(0, &emptyData);
    reportInitializationCheck("WebAudio zero-frame write", "return safely",
                              zeroFrameWrite ? "true" : "false", true);
    std::array<uint8_t, 4> oneFrame{};
    const bool writeAfterEos = probe->webAudioPlayer->writeBuffer(1, oneFrame.data());
    reportInitializationCheck("WebAudio write after EOS", "return safely",
                              writeAfterEos ? "true" : "false", true);
    return failures;
}

std::size_t runLoggingNegativeCases()
{
    auto factory = firebolt::rialto::IClientLogControlFactory::createFactory();
    if (!factory)
    {
        cliLog("[NEG] SKIP logging cases: log-control factory unavailable");
        return 0;
    }

    auto &control = factory->createClientLogControl();
    const bool nullHandlerAccepted = control.registerLogHandler({}, false);
    std::size_t failures = !reportInitializationCheck("registerLogHandler null handler", "reject",
                                                      nullHandlerAccepted ? "accepted" : "rejected",
                                                      !nullHandlerAccepted);

    class ProbeLogHandler final : public firebolt::rialto::IClientLogHandler
    {
    public:
        void log(Level, const std::string &, int, const std::string &, const std::string &) override {}
    };
    const auto handler = std::make_shared<ProbeLogHandler>();
    const bool firstRegistration = control.registerLogHandler(handler, false);
    const bool duplicateRegistration = control.registerLogHandler(handler, false);
    failures += !reportInitializationCheck("registerLogHandler repeated handler", "calls return; observe results",
                                           "first=" + std::string(firstRegistration ? "true" : "false") +
                                               ", second=" + (duplicateRegistration ? "true" : "false"),
                                           true);
    return failures;
}
#endif // REFERENCE_PLAYER_USE_MOCKS

/******** Negative checks for media pipeline and web audio creation end here *******/

int runNegativeChecks()
{
#ifdef REFERENCE_PLAYER_USE_MOCKS
    cliLog("Initialization failure checks require the real Rialto client; skipped in mock mode");
    return 0;
#else
    const std::size_t failures = runCreateControlNegativeCases() + runRegisterClientNegativeCases() +
                                 runCreateMediaPipelineNegativeCases() + runLoadNegativeCases() +
                                 runAttachSourceNegativeCases() + runSourceIdChecks() +
                                 runAllSourcesAttachedNegativeCases() + runPlaybackControlNegativeCases() +
                                 runPipelinePropertyNegativeCases() + runSegmentSubmissionNegativeCases() +
                                 runCapabilityNegativeCases() + runWebAudioNegativeCases() +
                                 runLoggingNegativeCases();
    cliLog("[NEG] total failures=" + std::to_string(failures));
    return failures == 0 ? 0 : 1;
#endif
}

} // namespace rialto_refplayer_cli
