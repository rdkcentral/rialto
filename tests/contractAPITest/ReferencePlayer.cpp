#include "ReferencePlayer.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <limits>
#include <thread>

namespace reference_player
{

ReferencePlayer::ReferencePlayer(Options options) : m_options(std::move(options))
{
}

ReferencePlayer::~ReferencePlayer()
{
    stopFeeder();
}

std::shared_ptr<ReferencePlayer> ReferencePlayer::create(Options options)
{
    auto player = std::make_shared<ReferencePlayer>(std::move(options));
    if (!player->initialize())
    {
        return nullptr;
    }
    return player;
}

bool ReferencePlayer::initialize()
{
    log("Initializing Rialto reference player");
    if (!connectControl())
    {
        return false;
    }
    if (!createMediaPipeline())
    {
        return false;
    }
    if (m_options.useWebAudio && !createWebAudio())
    {
        return false;
    }
    return true;
}

int ReferencePlayer::run()
{
    const bool valid = validateContract();
    stopFeeder();
    if (!valid)
    {
        log("Contract validation failed");
        return 1;
    }
    log("Contract validation passed");
    return runNegativeChecks();
}

bool ReferencePlayer::createControl()
{
#ifdef REFERENCE_PLAYER_USE_MOCKS
    m_control = std::make_shared<firebolt::rialto::MockControl>();
#else
    auto factory = firebolt::rialto::IControlFactory::createFactory();
    if (!factory)
    {
        log("IControlFactory::createFactory() returned null");
        return false;
    }

    m_control = factory->createControl();
    if (!m_control)
    {
        log("IControlFactory::createControl() failed");
        return false;
    }
#endif
    return true;
}

bool ReferencePlayer::connectControl()
{
    if (!createControl())
    {
        return false;
    }
    firebolt::rialto::ApplicationState appState = firebolt::rialto::ApplicationState::UNKNOWN;
    const auto self = shared_from_this();
    const auto controlClient = std::static_pointer_cast<firebolt::rialto::IControlClient>(self);
    if (!m_control->registerClient(controlClient, appState))
    {
        log("registerClient() failed");
        return false;
    }

    log("Registered control client; application state = " + toString(appState));
    return true;
}



bool ReferencePlayer::createMediaPipeline()
{
    return createMediaPipeline(firebolt::rialto::VideoRequirements{1920, 1080});
}

bool ReferencePlayer::createMediaPipeline(const firebolt::rialto::VideoRequirements &requirements)
{
#ifdef REFERENCE_PLAYER_USE_MOCKS
    m_mediaPipeline = std::make_unique<firebolt::rialto::MockMediaPipeline>();
#else
    auto factory = firebolt::rialto::IMediaPipelineFactory::createFactory();
    if (!factory)
    {
        log("IMediaPipelineFactory::createFactory() returned null");
        return false;
    }

    const auto self = std::static_pointer_cast<firebolt::rialto::IMediaPipelineClient>(shared_from_this());
    m_mediaPipeline = factory->createMediaPipeline(self, requirements);
    if (!m_mediaPipeline)
    {
        log("IMediaPipelineFactory::createMediaPipeline() failed");
        return false;
    }
#endif

    log("Media pipeline created");
    return true;
}

bool ReferencePlayer::createWebAudio()
{
#ifdef REFERENCE_PLAYER_USE_MOCKS
    m_webAudioPlayer = std::make_unique<firebolt::rialto::MockWebAudioPlayer>();
#else
    auto factory = firebolt::rialto::IWebAudioPlayerFactory::createFactory();
    if (!factory)
    {
        log("IWebAudioPlayerFactory::createFactory() returned null");
        return false;
    }

    firebolt::rialto::WebAudioConfig config{};
    config.pcm.rate = m_options.audioRate;
    config.pcm.channels = m_options.audioChannels;
    config.pcm.sampleSize = m_options.audioSampleSize;
    config.pcm.isBigEndian = false;
    config.pcm.isSigned = true;
    config.pcm.isFloat = false;

    auto configShared = std::make_shared<firebolt::rialto::WebAudioConfig>();
    *configShared = config;
    const auto self = std::static_pointer_cast<firebolt::rialto::IWebAudioPlayerClient>(shared_from_this());
    const auto selfWeak = std::weak_ptr<firebolt::rialto::IWebAudioPlayerClient>(self);
    const auto configWeak = std::weak_ptr<const firebolt::rialto::WebAudioConfig>(configShared);
    m_webAudioPlayer = factory->createWebAudioPlayer(selfWeak, "audio/x-raw", m_options.priority, configWeak);
    if (!m_webAudioPlayer)
    {
        log("IWebAudioPlayerFactory::createWebAudioPlayer() failed");
        return false;
    }
#endif

    log("Web audio player created");
    return true;
}

bool ReferencePlayer::validateContract()
{
    if (!m_mediaPipeline)
    {
        log("No media pipeline available; contract validation cannot continue");
        return false;
    }

    if (!loadWithSource())
    {
        log("loadWithSource() failed");
        return false;
    }

    if (!playMedia())
    {
        log("playMedia() failed");
        return false;
    }

    if (!waitForPlayback())
    {
        log("waitForPlayback() failed");
        return false;
    }

    if (!seekIfRequested())
    {
        log("seekIfRequested() failed");
        return false;
    }

    if (!applyVolumeIfRequested())
    {
        log("applyVolumeIfRequested() failed");
        return false;
    }

    if (!validateAdvancedMediaPipelineFeatures())
    {
        log("validateAdvancedMediaPipelineFeatures() failed");
        return false;
    }

    if (m_options.validateCapabilities && !validateCapabilities())
    {
        log("validateCapabilities() failed");
        return false;
    }

    if (m_options.useWebAudio && !validateWebAudio())
    {
        log("validateWebAudio() failed");
        return false;
    }

    if (!validateLogging())
    {
        log("validateLogging() failed");
        return false;
    }

    if (!waitForPlaybackComplete())
    {
        log("waitForPlaybackComplete() failed");
        return false;
    }

    return true;
}

bool ReferencePlayer::loadWithSource()
{
    log("Loading media: type=MSE mime=" + m_options.mimeType + " url=" + m_options.url);
    std::string mimeType = m_options.mimeType;

#if defined(REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER) && !defined(REFERENCE_PLAYER_USE_MOCKS)
    if (m_options.enablePlayback && mp4_demuxer::isLocalMp4(m_options.url))
    {
        if (!loadMp4())
        {
            return false;
        }
        m_feedEnabled = true;
        m_feedStop = false;
        m_feedThread = std::thread(&ReferencePlayer::feedLoop, this);
    }
#endif

    if (!m_mediaPipeline->load(firebolt::rialto::MediaType::MSE, mimeType, m_options.url, m_options.isLive))
    {
        log("IMediaPipeline::load() returned false");
        return false;
    }

    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source;
#if defined(REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER) && !defined(REFERENCE_PLAYER_USE_MOCKS)
    if (m_feedEnabled)
    {
        std::shared_ptr<firebolt::rialto::CodecData> codecData;
        if (!m_codecData.empty())
        {
            codecData = std::make_shared<firebolt::rialto::CodecData>();
            codecData->data = m_codecData;
            codecData->type = firebolt::rialto::CodecDataType::BUFFER;
        }
        source = std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>(
            mimeType, false, m_videoWidth, m_videoHeight, firebolt::rialto::SegmentAlignment::AU,
            firebolt::rialto::StreamFormat::AVC, codecData);
    }
    else
#endif
    {
        source = std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>(mimeType, false, 1920, 1080);
    }
    if (!m_mediaPipeline->attachSource(source))
    {
        log("IMediaPipeline::attachSource() returned false");
        return false;
    }
    m_currentSourceId = source->getId();
#ifdef REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER
    if (m_feedEnabled && m_audioTrackEnabled)
    {
        firebolt::rialto::AudioConfig audioConfig{};
        audioConfig.numberOfChannels = static_cast<uint32_t>(m_audioChannels);
        audioConfig.sampleRate = static_cast<uint32_t>(m_audioSampleRate);
        audioConfig.codecSpecificConfig = m_audioCodecData;

        std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> audioSource =
            std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>(
                "audio/mp4", false, audioConfig, firebolt::rialto::SegmentAlignment::AU);
        if (!m_mediaPipeline->attachSource(audioSource))
        {
            log("IMediaPipeline::attachSource(audio) returned false");
            return false;
        }
        m_audioSourceId = audioSource->getId();
        log("Audio source attached with id " + std::to_string(m_audioSourceId.load()));
    }
#endif
    if (!m_mediaPipeline->allSourcesAttached())
    {
        log("IMediaPipeline::allSourcesAttached() returned false");
        return false;
    }

    log("Source attached with id " + std::to_string(m_currentSourceId));
    return true;
}

bool ReferencePlayer::playMedia()
{
    if (!m_options.enablePlayback)
    {
        log("Playback disabled by program options");
        return true;
    }

    bool async = true;
    if (!m_mediaPipeline->play(async))
    {
        log("IMediaPipeline::play() returned false");
        return false;
    }
    log("play() invoked; async=" + std::string(async ? "true" : "false"));
    return true;
}

bool ReferencePlayer::waitForPlayback()
{
    if (!m_options.enablePlayback)
    {
        return true;
    }

    constexpr std::chrono::seconds kStartTimeout{10};
    std::unique_lock<std::mutex> lock(m_stateMutex);

    if (!m_stateCv.wait_for(lock, kStartTimeout,
                            [this] { return m_playbackStarted || m_playbackFailed || m_playbackComplete; }))
    {
        log("Timed out waiting for playback to start");
        return false;
    }
    if (m_playbackStarted)
    {
        log("Playback started");
        return true;
    }
    if (m_playbackFailed)
    {
        log("Playback failed before it started");
        return false;
    }
    log("Playback ended before reaching PLAYING");
    return false;
}

bool ReferencePlayer::waitForPlaybackComplete()
{
    if (!m_options.enablePlayback)
    {
        return true;
    }

    // Allow the media's own length plus a margin; with no known length, wait without a limit.
    int64_t mediaEndNs = 0;
#ifdef REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER
    for (const auto *samples : {&m_samples, &m_audioSamples})
    {
        for (const mp4_demuxer::Sample &sample : *samples)
        {
            mediaEndNs = std::max(mediaEndNs, sample.ptsNs + sample.durationNs);
        }
    }
#endif
    const auto completed = [this] { return m_playbackComplete.load(); };

    std::unique_lock<std::mutex> lock(m_stateMutex);
    log("Waiting for playback to complete");
    if (mediaEndNs > 0)
    {
        const auto timeout = std::chrono::nanoseconds(mediaEndNs) + std::chrono::seconds(30);
        if (!m_stateCv.wait_for(lock, timeout, completed))
        {
            log("Timed out waiting for playback to complete");
            return false;
        }
    }
    else
    {
        m_stateCv.wait(lock, completed);
    }

    if (m_playbackFailed)
    {
        log("Playback failed before completion");
        return false;
    }
    log("Playback completed");
    return true;
}

bool ReferencePlayer::seekIfRequested()
{
    if (!m_options.seekPosition.has_value())
    {
        return true;
    }

    if (!m_mediaPipeline->setPosition(*m_options.seekPosition))
    {
        log("IMediaPipeline::setPosition() returned false");
        return false;
    }
    log("seek() requested to position " + std::to_string(*m_options.seekPosition));
    return true;
}

bool ReferencePlayer::applyVolumeIfRequested()
{
    if (!m_options.desiredVolume.has_value())
    {
        return true;
    }

    if (!m_mediaPipeline->setVolume(*m_options.desiredVolume))
    {
        log("IMediaPipeline::setVolume() returned false");
        return false;
    }
    log("setVolume() invoked with level " + std::to_string(*m_options.desiredVolume));
    return true;
}

#ifdef REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER
bool ReferencePlayer::loadMp4()
{
    std::string path = m_options.url;
    path.erase(0, std::string("file://").size());

    m_file.open(path, std::ios::binary);
    if (!m_file)
    {
        log("Cannot open " + path);
        return false;
    }
    mp4_demuxer::Tracks tracks;
    std::string error;
    if (!mp4_demuxer::parse(m_file, tracks, error))
    {
        log(error + " in " + path);
        return false;
    }

    m_samples = std::move(tracks.video.samples);
    m_codecData = std::move(tracks.video.codecData);
    m_videoWidth = tracks.video.width;
    m_videoHeight = tracks.video.height;
    m_nextSample = 0;
    log("Loaded " + path + ": H.264 " + std::to_string(m_videoWidth) + "x" + std::to_string(m_videoHeight) + ", " +
        std::to_string(m_samples.size()) + " samples, avcC " + std::to_string(m_codecData.size()) + " bytes");
    if (tracks.hasAudio)
    {
        m_audioSamples = std::move(tracks.audio.samples);
        m_audioCodecData = std::move(tracks.audio.codecData);
        m_audioSampleRate = tracks.audio.sampleRate;
        m_audioChannels = tracks.audio.channels;
        m_nextAudioSample = 0;
        m_audioTrackEnabled = true;
        log("Loaded AAC audio: " + std::to_string(m_audioSampleRate) + " Hz, " +
            std::to_string(m_audioChannels) + " channels, " + std::to_string(m_audioSamples.size()) + " samples");
    }
    else
    {
        m_audioSamples.clear();
        m_audioCodecData.clear();
        m_audioSampleRate = 0;
        m_audioChannels = 0;
        m_audioTrackEnabled = false;
        log("No supported AAC (mp4a) audio track found; continuing video-only");
    }
    return true;
}

void ReferencePlayer::feedLoop()
{
    std::unique_lock<std::mutex> lock(m_feedMutex);
    while (true)
    {
        m_feedCv.wait(lock, [this] { return m_feedStop || !m_needDataQueue.empty(); });
        if (m_feedStop)
        {
            return;
        }
        const NeedDataRequest request = m_needDataQueue.front();
        m_needDataQueue.pop_front();
        lock.unlock();
        feedRequest(request);
        lock.lock();
    }
}

void ReferencePlayer::feedRequest(const NeedDataRequest &request)
{
#ifndef REFERENCE_PLAYER_USE_MOCKS
    using firebolt::rialto::AddSegmentStatus;
    using firebolt::rialto::MediaSourceStatus;

    const bool isAudio = m_audioTrackEnabled && request.sourceId == m_audioSourceId.load();
    auto &samples = isAudio ? m_audioSamples : m_samples;
    size_t &nextSample = isAudio ? m_nextAudioSample : m_nextSample;
    std::vector<std::vector<uint8_t>> buffers;
    MediaSourceStatus status = MediaSourceStatus::OK;
    size_t sent = 0;

    while (sent < request.frameCount && nextSample < samples.size())
    {
        const mp4_demuxer::Sample &sample = samples[nextSample];
        std::vector<uint8_t> frame(sample.size);
        m_file.clear();
        m_file.seekg(static_cast<std::streamoff>(sample.offset));
        if (!m_file.read(reinterpret_cast<char *>(frame.data()), static_cast<std::streamsize>(sample.size)))
        {
            log("Failed to read " + std::string(isAudio ? "audio" : "video") + " sample " +
                std::to_string(nextSample));
            status = MediaSourceStatus::ERROR;
            break;
        }
        buffers.push_back(std::move(frame));

        std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSegment> base;
        if (isAudio)
        {
            auto segment = std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentAudio>(
                request.sourceId, sample.ptsNs, sample.durationNs, m_audioSampleRate, m_audioChannels);
            segment->setData(sample.size, buffers.back().data());
            segment->setSegmentAlignment(firebolt::rialto::SegmentAlignment::AU);
            base = std::move(segment);
        }
        else
        {
            auto segment = std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentVideo>(
                request.sourceId, sample.ptsNs, sample.durationNs, m_videoWidth, m_videoHeight);
            segment->setData(sample.size, buffers.back().data());
            segment->setSegmentAlignment(firebolt::rialto::SegmentAlignment::AU);
            base = std::move(segment);
        }
        const AddSegmentStatus result = m_mediaPipeline->addSegment(request.requestId, base);
        if (result == AddSegmentStatus::NO_SPACE)
        {
            buffers.pop_back();
            if (sent == 0)
            {
                log(std::string(isAudio ? "Audio" : "Video") + " sample " + std::to_string(nextSample) +
                    " does not fit in the shared buffer");
                status = MediaSourceStatus::ERROR;
            }
            break;
        }
        if (result != AddSegmentStatus::OK)
        {
            log("addSegment() failed for " + std::string(isAudio ? "audio" : "video") + " sample " +
                std::to_string(nextSample));
            status = MediaSourceStatus::ERROR;
            break;
        }
        ++nextSample;
        ++sent;
    }

    if (status == MediaSourceStatus::OK && nextSample >= samples.size())
    {
        status = MediaSourceStatus::EOS;
    }
 #if 0
    log(std::string(isAudio ? "Audio" : "Video") + " request " + std::to_string(request.requestId) + ": sent " +
        std::to_string(sent) + " sample(s), " + std::to_string(nextSample) + "/" + std::to_string(samples.size()) +
        " total, status=" +
        (status == MediaSourceStatus::EOS ? "EOS" : (status == MediaSourceStatus::OK ? "OK" : "ERROR")));
 #endif
    if (!m_mediaPipeline->haveData(status, request.requestId))
    {
        log("haveData() returned false for request " + std::to_string(request.requestId));
    }
#else
    (void)request;
#endif
}
#endif

void ReferencePlayer::stopFeeder()
{
#ifdef REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER
    {
        std::lock_guard<std::mutex> lock(m_feedMutex);
        m_feedStop = true;
    }
    m_feedCv.notify_all();
    if (m_feedThread.joinable())
    {
        m_feedThread.join();
    }
#endif
}

bool ReferencePlayer::validateAdvancedMediaPipelineFeatures()
{
    bool sync = false;
    bool muted = false;
    bool useBuffering = false;
    uint32_t bufferingMs = 0U;
    int32_t streamSyncMode = 0;
    double volume = 0.0;
    std::string textTrackId;

    if (!m_mediaPipeline->setLowLatency(true))
        log("setLowLatency(true) returned false");
    else
        log("setLowLatency(true) returned true");
    if (!m_mediaPipeline->setSync(true))
        log("setSync(true) returned false");
    else
        log("setSync(true) returned true");
    if (!m_mediaPipeline->getSync(sync))
        log("getSync() returned false");
    else
        log("getSync() returned true");
    if (!m_mediaPipeline->setSyncOff(false))
        log("setSyncOff(false) returned false");
    else
        log("setSyncOff(false) returned true");
    if (!m_mediaPipeline->setMute(m_audioSourceId, true))
        log("setMute() returned false");
    else
        log("setMute() returned true");
    if (!m_mediaPipeline->getMute(m_audioSourceId, muted))
        log("getMute() returned false");
    else
        log("getMute() returned true");
    if (!m_mediaPipeline->setVolume(0.7))
        log("setVolume() returned false");
    else
        log("setVolume() returned true");
    if (!m_mediaPipeline->getVolume(volume))
        log("getVolume() returned false");
    else
        log("getVolume() returned true");
    if (!m_mediaPipeline->setTextTrackIdentifier("eng"))
        log("setTextTrackIdentifier() returned false");
    else
        log("setTextTrackIdentifier() returned true");
    if (!m_mediaPipeline->getTextTrackIdentifier(textTrackId))
        log("getTextTrackIdentifier() returned false");
    else
        log("getTextTrackIdentifier() returned true");
    if (!m_mediaPipeline->setBufferingLimit(1500U))
        log("setBufferingLimit() returned false");
    else
        log("setBufferingLimit() returned true");
    if (!m_mediaPipeline->getBufferingLimit(bufferingMs))
        log("getBufferingLimit() returned false");
    else
        log("getBufferingLimit() returned true");
    if (!m_mediaPipeline->setUseBuffering(true))
        log("setUseBuffering() returned false");
    else
        log("setUseBuffering() returned true");
    if (!m_mediaPipeline->getUseBuffering(useBuffering))
        log("getUseBuffering() returned false");
    else
        log("getUseBuffering() returned true");
    if (!m_mediaPipeline->setStreamSyncMode(m_currentSourceId, 1))
        log("setStreamSyncMode() returned false");
    else
        log("setStreamSyncMode() returned true");
    if (!m_mediaPipeline->getStreamSyncMode(streamSyncMode))
        log("getStreamSyncMode() returned false");
    else
        log("getStreamSyncMode() returned true");
    if (!m_mediaPipeline->setSourcePosition(m_currentSourceId, 0, false, 1.0, 0ULL))
        log("setSourcePosition() returned false");
    else
        log("setSourcePosition() returned true");
    if (!m_mediaPipeline->setSubtitleOffset(m_currentSourceId, 0))
        log("setSubtitleOffset() returned false");
    else
        log("setSubtitleOffset() returned true");
    int64_t duration = 0;
    if (!m_mediaPipeline->getDuration(duration))
        log("getDuration() returned false");
    else
        log("getDuration() returned true");
    m_lastDuration = duration;
    int64_t position = 0;
    if (!m_mediaPipeline->getPosition(position))
        log("getPosition() returned false");
    else
        log("getPosition() returned true");
    m_lastPosition = position;

    log("Advanced pipeline checks complete. final state: sync=" + std::string(sync ? "true" : "false") +
        ", muted=" + std::string(muted ? "true" : "false") + ", volume=" + std::to_string(volume) +
        ", bufferingMs=" + std::to_string(bufferingMs) + ", streamSyncMode=" + std::to_string(streamSyncMode));
    return true;
}

bool ReferencePlayer::validateCapabilities()
{
    const auto logMimeTypes = [this](const std::string &source, const std::vector<std::string> &mimeTypes)
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
        log(message);
    };

#ifdef REFERENCE_PLAYER_USE_MOCKS
    auto caps = std::make_unique<firebolt::rialto::MockMediaPipelineCapabilities>();
    auto keyCaps = std::make_shared<firebolt::rialto::MockMediaKeysCapabilities>();
    const auto videoMimeTypes = caps->getSupportedMimeTypes(firebolt::rialto::MediaSourceType::VIDEO);
    const auto audioMimeTypes = caps->getSupportedMimeTypes(firebolt::rialto::MediaSourceType::AUDIO);
    logMimeTypes("Video", videoMimeTypes);
    logMimeTypes("Audio", audioMimeTypes);
    const bool supported = caps->isMimeTypeSupported(m_options.mimeType);
    const auto props = caps->getSupportedProperties(firebolt::rialto::MediaSourceType::VIDEO,
                                                   {"volume", "mute", "low-latency"});
    bool isVideoMaster = false;
    caps->isVideoMaster(isVideoMaster);
    log("Media pipeline capabilities: supported=" +
        std::string(supported ? "true" : "false") + ", properties=" + std::to_string(props.size()) +
        ", isVideoMaster=" + std::string(isVideoMaster ? "true" : "false"));
    const auto keySystems = keyCaps->getSupportedKeySystems();
    log("Key systems supported: " + std::to_string(keySystems.size()));
    return true;
#else
    auto capsFactory = firebolt::rialto::IMediaPipelineCapabilitiesFactory::createFactory();
    if (!capsFactory)
    {
        log("IMediaPipelineCapabilitiesFactory::createFactory() returned null");
        return false;
    }

    auto caps = capsFactory->createMediaPipelineCapabilities();
    if (!caps)
    {
        log("createMediaPipelineCapabilities() returned null");
        return false;
    }

    const auto videoMimeTypes = caps->getSupportedMimeTypes(firebolt::rialto::MediaSourceType::VIDEO);
    const auto audioMimeTypes = caps->getSupportedMimeTypes(firebolt::rialto::MediaSourceType::AUDIO);
    logMimeTypes("Video", videoMimeTypes);
    logMimeTypes("Audio", audioMimeTypes);
    bool supported = caps->isMimeTypeSupported(m_options.mimeType);
    const auto props = caps->getSupportedProperties(firebolt::rialto::MediaSourceType::VIDEO,
                                                   {"volume", "mute", "low-latency"});
    bool isVideoMaster = false;
    caps->isVideoMaster(isVideoMaster);

    log("Media pipeline capabilities: supported=" +
        std::string(supported ? "true" : "false") + ", properties=" + std::to_string(props.size()) +
        ", isVideoMaster=" + std::string(isVideoMaster ? "true" : "false"));

    auto keySystemFactory = firebolt::rialto::IMediaKeysCapabilitiesFactory::createFactory();
    if (!keySystemFactory)
    {
        log("IMediaKeysCapabilitiesFactory::createFactory() returned null");
        return false;
    }

    auto keyCaps = keySystemFactory->getMediaKeysCapabilities();
    if (!keyCaps)
    {
        log("getMediaKeysCapabilities() returned null");
        return false;
    }

    const auto keySystems = keyCaps->getSupportedKeySystems();
    log("Key systems supported: " + std::to_string(keySystems.size()));
    return true;
#endif
}

bool ReferencePlayer::validateWebAudio()
{
    if (!m_webAudioPlayer)
    {
        log("Web audio player not created");
        return false;
    }

    uint32_t preferredFrames = 0;
    uint32_t maximumFrames = 0;
    bool supportDeferredPlay = false;
    if (!m_webAudioPlayer->getDeviceInfo(preferredFrames, maximumFrames, supportDeferredPlay))
    {
        log("IWebAudioPlayer::getDeviceInfo() returned false");
        return false;
    }
    log("WebAudio device info: preferredFrames=" + std::to_string(preferredFrames) + ", maximumFrames=" +
        std::to_string(maximumFrames) + ", supportDeferredPlay=" + std::string(supportDeferredPlay ? "true" : "false"));

    if (!m_webAudioPlayer->play())
    {
        log("IWebAudioPlayer::play() returned false");
        return false;
    }

    double volume = 0.0;
    if (!m_webAudioPlayer->setVolume(0.3) || !m_webAudioPlayer->getVolume(volume))
    {
        log("IWebAudioPlayer volume operations returned false");
        return false;
    }

    uint32_t availableFrames = 0;
    std::shared_ptr<firebolt::rialto::WebAudioShmInfo> shmInfo = std::make_shared<firebolt::rialto::WebAudioShmInfo>();
    if (!m_webAudioPlayer->getBufferAvailable(availableFrames, shmInfo))
    {
        log("IWebAudioPlayer::getBufferAvailable() returned false");
        return false;
    }

    const uint32_t bytesPerFrame = m_options.audioChannels * (m_options.audioSampleSize / 8U);
    if (availableFrames > 0)
    {
        std::vector<uint8_t> audioData(availableFrames * bytesPerFrame, 0);
        if (!m_webAudioPlayer->writeBuffer(availableFrames, audioData.data()))
        {
            log("IWebAudioPlayer::writeBuffer() returned false");
            return false;
        }
    }

    if (!m_webAudioPlayer->setEos())
    {
        log("IWebAudioPlayer::setEos() returned false");
        return false;
    }

    return true;
}

bool ReferencePlayer::validateLogging()
{
#ifdef REFERENCE_PLAYER_USE_MOCKS
    auto handler = std::make_shared<firebolt::rialto::IClientLogHandler>();
    (void)handler;
    log("Logging handler registered successfully");
    return true;
#else
    auto logFactory = firebolt::rialto::IClientLogControlFactory::createFactory();
    if (!logFactory)
    {
        log("IClientLogControlFactory::createFactory() returned null");
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
        log("IClientLogControl::registerLogHandler() returned false");
        return false;
    }

    log("Logging handler registered successfully");
    return true;
#endif
}

void ReferencePlayer::notifyApplicationState(firebolt::rialto::ApplicationState state)
{
    m_applicationRunning = (state == firebolt::rialto::ApplicationState::RUNNING);
    log("notifyApplicationState: " + toString(state));
    m_stateCv.notify_all();
}

void ReferencePlayer::notifyDuration(int64_t duration)
{
    m_lastDuration = duration;
    log("notifyDuration: " + std::to_string(duration));
}

void ReferencePlayer::notifyPosition(int64_t position)
{
    m_lastPosition = position;
#if 0
    log("notifyPosition: " + std::to_string(position));
#endif
}

void ReferencePlayer::notifyNativeSize(uint32_t width, uint32_t height, double aspect)
{
    log("notifyNativeSize: width=" + std::to_string(width) + ", height=" + std::to_string(height) +
        ", aspect=" + std::to_string(aspect));
}

void ReferencePlayer::notifyNetworkState(firebolt::rialto::NetworkState state)
{
    log("notifyNetworkState: " + toString(state));
}

void ReferencePlayer::notifyPlaybackState(firebolt::rialto::PlaybackState state)
{
    log("notifyPlaybackState: " + toString(state));
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        switch (state)
        {
        case firebolt::rialto::PlaybackState::PLAYING:
            m_playbackStarted = true;
            break;
        case firebolt::rialto::PlaybackState::STOPPED:
        case firebolt::rialto::PlaybackState::END_OF_STREAM:
            m_playbackComplete = true;
            log("Playback complete");
            break;
        case firebolt::rialto::PlaybackState::FAILURE:
            m_playbackFailed = true;
            m_playbackComplete = true;
            break;
        default:
            break;
        }
    }
    m_stateCv.notify_all();
}

void ReferencePlayer::notifyVideoData(bool hasData)
{
    m_lastVideoData = hasData;
    log("notifyVideoData: " + std::string(hasData ? "true" : "false"));
}

void ReferencePlayer::notifyAudioData(bool hasData)
{
    m_lastAudioData = hasData;
    log("notifyAudioData: " + std::string(hasData ? "true" : "false"));
}

void ReferencePlayer::notifyNeedMediaData(int32_t sourceId, size_t frameCount, uint32_t needDataRequestId,
                                         const std::shared_ptr<firebolt::rialto::MediaPlayerShmInfo> &shmInfo)
{
#if 0
    log("notifyNeedMediaData: sourceId=" + std::to_string(sourceId) + ", frameCount=" + std::to_string(frameCount) +
        ", requestId=" + std::to_string(needDataRequestId));
#endif
    (void)shmInfo;
#if defined(REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER) && !defined(REFERENCE_PLAYER_USE_MOCKS)
    if (m_feedEnabled)
    {
        {
            std::lock_guard<std::mutex> lock(m_feedMutex);
            m_needDataQueue.push_back({sourceId, frameCount, needDataRequestId});
        }
        m_feedCv.notify_one();
    }
#else
    (void)sourceId;
    (void)frameCount;
    (void)needDataRequestId;
#endif
}

void ReferencePlayer::notifyCancelNeedMediaData(int32_t sourceId)
{
    log("notifyCancelNeedMediaData: sourceId=" + std::to_string(sourceId));
#if defined(REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER) && !defined(REFERENCE_PLAYER_USE_MOCKS)
    if (m_feedEnabled)
    {
        std::lock_guard<std::mutex> lock(m_feedMutex);
        m_needDataQueue.erase(std::remove_if(m_needDataQueue.begin(), m_needDataQueue.end(),
                                             [sourceId](const NeedDataRequest &r) { return r.sourceId == sourceId; }),
                              m_needDataQueue.end());
    }
#endif
}

void ReferencePlayer::notifyQos(int32_t sourceId, const firebolt::rialto::QosInfo &qosInfo)
{
    log("notifyQos: sourceId=" + std::to_string(sourceId) + ", processed=" + std::to_string(qosInfo.processed) +
        ", dropped=" + std::to_string(qosInfo.dropped));
}

void ReferencePlayer::notifyBufferUnderflow(int32_t sourceId)
{
    log("notifyBufferUnderflow: sourceId=" + std::to_string(sourceId));
}

void ReferencePlayer::notifyFirstFrameReceived(int32_t sourceId)
{
    log("notifyFirstFrameReceived: sourceId=" + std::to_string(sourceId));
}

void ReferencePlayer::notifyPlaybackError(int32_t sourceId, firebolt::rialto::PlaybackError error)
{
    log("notifyPlaybackError: sourceId=" + std::to_string(sourceId) +
        ", error=" + std::to_string(static_cast<int>(error)));
}

void ReferencePlayer::notifySourceFlushed(int32_t sourceId)
{
    log("notifySourceFlushed: sourceId=" + std::to_string(sourceId));
}

void ReferencePlayer::notifyPlaybackInfo(const firebolt::rialto::PlaybackInfo &playbackInfo)
{
#if 0 // Disable logging of playback info
    log("notifyPlaybackInfo: position=" + std::to_string(playbackInfo.currentPosition) +
        ", volume=" + std::to_string(playbackInfo.volume));
#endif
}

void ReferencePlayer::notifyState(firebolt::rialto::WebAudioPlayerState state)
{
    log("notifyState: " + toString(state));
}

std::string ReferencePlayer::toString(firebolt::rialto::ApplicationState state)
{
    switch (state)
    {
    case firebolt::rialto::ApplicationState::UNKNOWN: return "UNKNOWN";
    case firebolt::rialto::ApplicationState::RUNNING: return "RUNNING";
    case firebolt::rialto::ApplicationState::INACTIVE: return "INACTIVE";
    }
    return "UNKNOWN";
}

std::string ReferencePlayer::toString(firebolt::rialto::PlaybackState state)
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

std::string ReferencePlayer::toString(firebolt::rialto::NetworkState state)
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

std::string ReferencePlayer::toString(firebolt::rialto::WebAudioPlayerState state)
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

void ReferencePlayer::log(const std::string &message) const
{
    std::cout << "[rialto-ref-player] " << message << std::endl;
}

/************************   Negative checks  *************************/
int ReferencePlayer::runNegativeChecks()
{
#ifdef REFERENCE_PLAYER_USE_MOCKS
    log("Initialization failure checks require the real Rialto client; skipped in mock mode");
    return 0;
#else
    const std::size_t failures = runCreateControlNegativeCases() + runRegisterClientNegativeCases() +
                                 runCreateMediaPipelineNegativeCases() + runLoadNegativeCases() +
                                 runAttachSourceNegativeCases() + runSourceIdChecks() +
                                 runAllSourcesAttachedNegativeCases() + runPlaybackControlNegativeCases() +
                                 runPipelinePropertyNegativeCases() + runSegmentSubmissionNegativeCases() +
                                 runCapabilityNegativeCases() + runWebAudioNegativeCases() +
                                 runLoggingNegativeCases();
    log("[NEG] total failures=" + std::to_string(failures));
    return failures == 0 ? 0 : 1;
#endif
}

bool ReferencePlayer::reportInitializationCheck(const std::string &name, const std::string &expected,
                                                const std::string &observed, bool passed) const
{
    log(std::string("[NEG] ") + (passed ? "PASS " : "FAIL ") + name + ": expected=" + expected +
        " observed=" + observed);
    return passed;
}

std::shared_ptr<ReferencePlayer> ReferencePlayer::createPipelineProbe()
{
    auto probe = std::make_shared<ReferencePlayer>(Options{});
    if (!probe->connectControl() || !probe->createMediaPipeline())
    {
        probe->m_mediaPipeline.reset();
        probe->m_control.reset();
        return nullptr;
    }
    return probe;
}

std::shared_ptr<ReferencePlayer> ReferencePlayer::createLoadedPipelineProbe()
{
    auto probe = createPipelineProbe();
    if (!probe || !probe->m_mediaPipeline->load(firebolt::rialto::MediaType::MSE, "video/mp4",
                                                 "blob:negative-test", false))
    {
        destroyPipelineProbe(probe);
        return nullptr;
    }
    return probe;
}

std::shared_ptr<ReferencePlayer> ReferencePlayer::createAttachedPipelineProbe()
{
    auto probe = createLoadedPipelineProbe();
    if (!probe)
    {
        return nullptr;
    }

    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/mp4", false, 1920, 1080);
    if (!probe->m_mediaPipeline->attachSource(source))
    {
        destroyPipelineProbe(probe);
        return nullptr;
    }
    probe->m_currentSourceId = source->getId();
    if (!probe->m_mediaPipeline->allSourcesAttached())
    {
        destroyPipelineProbe(probe);
        return nullptr;
    }
    return probe;
}

void ReferencePlayer::destroyPipelineProbe(std::shared_ptr<ReferencePlayer> &probe)
{
    if (probe)
    {
        probe->m_mediaPipeline.reset();
        probe->m_control.reset();
        probe.reset();
    }
}

std::size_t ReferencePlayer::runCreateControlNegativeCases()
{
    std::size_t failures = 0;
    auto probe = std::make_shared<ReferencePlayer>(Options{});
    const bool firstCreated = probe->createControl();
    const bool secondCreated = firstCreated && probe->createControl();
    failures += !reportInitializationCheck("createControl repeated creation", "second call returns",
                                           "first=" + std::string(firstCreated ? "created" : "failed") +
                                               ", second=" + (secondCreated ? "created" : "failed"),
                                           firstCreated);
    probe->m_control.reset();
    return failures;
}

std::size_t ReferencePlayer::runRegisterClientNegativeCases()
{
    std::size_t failures = 0;
    auto probe = std::make_shared<ReferencePlayer>(Options{});
    const bool controlCreated = probe->createControl();
    bool emptyAccepted = false;
    bool expiredAccepted = false;
    bool validAfterInvalidAccepted = false;
    if (controlCreated)
    {
        firebolt::rialto::ApplicationState appState = firebolt::rialto::ApplicationState::UNKNOWN;
        emptyAccepted = probe->m_control->registerClient({}, appState);

        std::weak_ptr<firebolt::rialto::IControlClient> expiredClient;
        {
            auto expiredOwner = std::make_shared<ReferencePlayer>(Options{});
            expiredClient = std::static_pointer_cast<firebolt::rialto::IControlClient>(expiredOwner);
        }
        expiredAccepted = probe->m_control->registerClient(expiredClient, appState);

        const auto validClient = std::static_pointer_cast<firebolt::rialto::IControlClient>(probe);
        validAfterInvalidAccepted = probe->m_control->registerClient(validClient, appState);
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
    probe->m_control.reset();

    auto repeatProbe = std::make_shared<ReferencePlayer>(Options{});
    const bool repeatControlCreated = repeatProbe->createControl();
    bool firstRegistration = false;
    bool duplicateRegistration = false;
    bool secondClientRegistration = false;
    std::shared_ptr<ReferencePlayer> secondClient;
    if (repeatControlCreated)
    {
        firebolt::rialto::ApplicationState appState = firebolt::rialto::ApplicationState::UNKNOWN;
        const auto client = std::static_pointer_cast<firebolt::rialto::IControlClient>(repeatProbe);
        firstRegistration = repeatProbe->m_control->registerClient(client, appState);
        duplicateRegistration = repeatProbe->m_control->registerClient(client, appState);
        secondClient = std::make_shared<ReferencePlayer>(Options{});
        const auto anotherClient = std::static_pointer_cast<firebolt::rialto::IControlClient>(secondClient);
        secondClientRegistration = repeatProbe->m_control->registerClient(anotherClient, appState);
    }
    failures += !reportInitializationCheck(
        "registerClient duplicate and second client", "calls return; observe registration results",
        repeatControlCreated
            ? "first=" + std::string(firstRegistration ? "accepted" : "rejected") +
                  ", duplicate=" + (duplicateRegistration ? "accepted" : "rejected") +
                  ", second=" + (secondClientRegistration ? "accepted" : "rejected")
            : "control creation failed",
        repeatControlCreated && firstRegistration);
    repeatProbe->m_control.reset();
    repeatProbe.reset();
    secondClient.reset();
    return failures;
}

std::size_t ReferencePlayer::runCreateMediaPipelineNegativeCases()
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

    auto invalidRequirementsProbe = std::make_shared<ReferencePlayer>(Options{});
    const bool requirementsControlConnected = invalidRequirementsProbe->connectControl();
    const bool zeroRequirementsAccepted = requirementsControlConnected &&
        invalidRequirementsProbe->createMediaPipeline(firebolt::rialto::VideoRequirements{0, 0});
    failures += !reportInitializationCheck("createMediaPipeline zero dimensions", "return; observe result",
                                           requirementsControlConnected
                                               ? (zeroRequirementsAccepted ? "accepted" : "rejected")
                                               : "control setup failed",
                                           requirementsControlConnected);
    invalidRequirementsProbe->m_mediaPipeline.reset();
    invalidRequirementsProbe->m_control.reset();

    auto repeatedProbe = createPipelineProbe();
    const bool repeatedCreated = repeatedProbe && repeatedProbe->createMediaPipeline();
    failures += !reportInitializationCheck("createMediaPipeline repeated creation", "second call returns",
                                           repeatedProbe ? (repeatedCreated ? "created" : "rejected")
                                                         : "pipeline setup failed",
                                           static_cast<bool>(repeatedProbe));
    destroyPipelineProbe(repeatedProbe);
    return failures;
}

std::size_t ReferencePlayer::runLoadNegativeCases()
{
    std::size_t failures = 0;
    const auto runRejectedLoad = [this, &failures](const std::string &name, firebolt::rialto::MediaType type,
                                                   const std::string &mimeType, const std::string &url)
    {
        auto probe = createPipelineProbe();
        const bool loaded = probe && probe->m_mediaPipeline->load(type, mimeType, url, false);
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
        firstLoad = repeatedProbe->m_mediaPipeline->load(firebolt::rialto::MediaType::MSE, "video/mp4",
                                                           "blob:negative-test", false);
        if (firstLoad)
        {
            secondLoad = repeatedProbe->m_mediaPipeline->load(firebolt::rialto::MediaType::MSE, "video/mp4",
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

std::size_t ReferencePlayer::runAttachSourceNegativeCases()
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
    const auto runRejectedAttach = [this, &failures](const std::string &name, bool loadFirst,
                                                      std::unique_ptr<MediaSource> source)
    {
        auto probe = createPipelineProbe();
        const bool loaded = probe && (!loadFirst || probe->m_mediaPipeline->load(
                                                         firebolt::rialto::MediaType::MSE, "video/mp4",
                                                         "blob:negative-test", false));
        const bool attached = loaded && probe->m_mediaPipeline->attachSource(source);
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
    bool loaded = duplicateProbe && duplicateProbe->m_mediaPipeline->load(
                                       firebolt::rialto::MediaType::MSE, "video/mp4", "blob:negative-test", false);
    bool firstAttached = false;
    bool duplicateAttached = false;
    if (loaded)
    {
        auto firstSource = makeVideo("video/mp4", 1920, 1080);
        auto duplicateSource = makeVideo("video/mp4", 1280, 720);
        firstAttached = duplicateProbe->m_mediaPipeline->attachSource(firstSource);
        duplicateAttached = firstAttached && duplicateProbe->m_mediaPipeline->attachSource(duplicateSource);
    }
    failures += !reportInitializationCheck(
        "attachSource duplicate video", "reject second video source",
        loaded ? "first=" + std::string(firstAttached ? "accepted" : "rejected") +
                     ", duplicate=" + (duplicateAttached ? "accepted" : "rejected")
               : "pipeline/load setup failed",
        loaded && firstAttached && !duplicateAttached);
    destroyPipelineProbe(duplicateProbe);

    auto afterAttachedProbe = createPipelineProbe();
    bool afterLoad = afterAttachedProbe && afterAttachedProbe->m_mediaPipeline->load(
                                              firebolt::rialto::MediaType::MSE, "video/mp4",
                                              "blob:negative-test", false);
    bool sourceAttached = false;
    bool sourcesFinalized = false;
    bool lateSourceAttached = false;
    if (afterLoad)
    {
        auto firstSource = makeVideo("video/mp4", 1920, 1080);
        sourceAttached = afterAttachedProbe->m_mediaPipeline->attachSource(firstSource);
        sourcesFinalized = sourceAttached && afterAttachedProbe->m_mediaPipeline->allSourcesAttached();
        auto lateSource = makeAudio(2, 48000);
        lateSourceAttached = sourcesFinalized && afterAttachedProbe->m_mediaPipeline->attachSource(lateSource);
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

std::size_t ReferencePlayer::runSourceIdChecks()
{
    using MediaSource = firebolt::rialto::IMediaPipeline::MediaSource;
    using MediaSourceVideo = firebolt::rialto::IMediaPipeline::MediaSourceVideo;
    using MediaSourceAudio = firebolt::rialto::IMediaPipeline::MediaSourceAudio;
    std::size_t failures = 0;
    auto probe = createPipelineProbe();
    const bool loaded = probe && probe->m_mediaPipeline->load(
                                     firebolt::rialto::MediaType::MSE, "video/mp4", "blob:negative-test", false);
    std::unique_ptr<MediaSource> source = std::make_unique<MediaSourceVideo>("video/mp4", false, 1920, 1080);
    const int32_t initialId = source->getId();
    const bool attached = loaded && probe->m_mediaPipeline->attachSource(source);
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
    const bool audioAttached = attached && probe->m_mediaPipeline->attachSource(audioSource);
    const int32_t audioId = audioSource->getId();
    failures += !reportInitializationCheck("MediaSource video/audio IDs", "successful sources have distinct IDs",
                                           "video=" + std::to_string(attachedId) +
                                               ", audio=" + std::to_string(audioId) +
                                               ", audio attached=" + (audioAttached ? "true" : "false"),
                                           attached && audioAttached && attachedId != audioId);
    destroyPipelineProbe(probe);

    auto rejectedProbe = createPipelineProbe();
    const bool rejectedLoad = rejectedProbe && rejectedProbe->m_mediaPipeline->load(
                                                  firebolt::rialto::MediaType::MSE, "video/mp4",
                                                  "blob:negative-test", false);
    std::unique_ptr<MediaSource> rejectedSource = std::make_unique<MediaSourceVideo>("video/mp4", false, 0, 0);
    const int32_t idBeforeRejectedAttach = rejectedSource->getId();
    const bool rejectedAttach = rejectedLoad && rejectedProbe->m_mediaPipeline->attachSource(rejectedSource);
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

std::size_t ReferencePlayer::runAllSourcesAttachedNegativeCases()
{
    using MediaSource = firebolt::rialto::IMediaPipeline::MediaSource;
    using MediaSourceVideo = firebolt::rialto::IMediaPipeline::MediaSourceVideo;
    std::size_t failures = 0;

    auto beforeLoadProbe = createPipelineProbe();
    const bool beforeLoad = beforeLoadProbe && beforeLoadProbe->m_mediaPipeline->allSourcesAttached();
    failures += !reportInitializationCheck("allSourcesAttached before load", "reject", beforeLoadProbe
                                                                                       ? (beforeLoad ? "accepted" : "rejected")
                                                                                       : "pipeline setup failed",
                                           beforeLoadProbe && !beforeLoad);
    destroyPipelineProbe(beforeLoadProbe);

    auto noSourcesProbe = createPipelineProbe();
    const bool noSourcesLoaded = noSourcesProbe && noSourcesProbe->m_mediaPipeline->load(
                                                       firebolt::rialto::MediaType::MSE, "video/mp4",
                                                       "blob:negative-test", false);
    const bool noSourcesFinalized = noSourcesLoaded && noSourcesProbe->m_mediaPipeline->allSourcesAttached();
    failures += !reportInitializationCheck("allSourcesAttached with no sources", "reject",
                                           noSourcesLoaded ? (noSourcesFinalized ? "accepted" : "rejected")
                                                           : "pipeline/load setup failed",
                                           noSourcesLoaded && !noSourcesFinalized);
    destroyPipelineProbe(noSourcesProbe);

    auto afterRejectedAttachProbe = createPipelineProbe();
    const bool failedAttachLoaded = afterRejectedAttachProbe && afterRejectedAttachProbe->m_mediaPipeline->load(
                                                                 firebolt::rialto::MediaType::MSE, "video/mp4",
                                                                 "blob:negative-test", false);
    std::unique_ptr<MediaSource> invalidSource = std::make_unique<MediaSourceVideo>("video/mp4", false, 0, 0);
    const bool invalidSourceAttached = failedAttachLoaded &&
        afterRejectedAttachProbe->m_mediaPipeline->attachSource(invalidSource);
    const bool finalizedAfterRejectedAttach = failedAttachLoaded && !invalidSourceAttached &&
        afterRejectedAttachProbe->m_mediaPipeline->allSourcesAttached();
    failures += !reportInitializationCheck("allSourcesAttached after rejected attach", "reject incomplete source set",
                                           failedAttachLoaded
                                               ? (finalizedAfterRejectedAttach ? "accepted" : "rejected")
                                               : "pipeline/load setup failed",
                                           failedAttachLoaded && !invalidSourceAttached &&
                                               !finalizedAfterRejectedAttach);
    destroyPipelineProbe(afterRejectedAttachProbe);

    auto repeatedProbe = createPipelineProbe();
    const bool repeatedLoaded = repeatedProbe && repeatedProbe->m_mediaPipeline->load(
                                                   firebolt::rialto::MediaType::MSE, "video/mp4",
                                                   "blob:negative-test", false);
    bool sourceAttached = false;
    bool firstFinalization = false;
    bool secondFinalization = false;
    if (repeatedLoaded)
    {
        std::unique_ptr<MediaSource> source = std::make_unique<MediaSourceVideo>("video/mp4", false, 1920, 1080);
        sourceAttached = repeatedProbe->m_mediaPipeline->attachSource(source);
        firstFinalization = sourceAttached && repeatedProbe->m_mediaPipeline->allSourcesAttached();
        secondFinalization = firstFinalization && repeatedProbe->m_mediaPipeline->allSourcesAttached();
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

std::size_t ReferencePlayer::runPlaybackControlNegativeCases()
{
    std::size_t failures = 0;
    const auto observeUnloadedCall = [this, &failures](const std::string &name, const auto &call)
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

    observeUnloadedCall("play before load", [](ReferencePlayer &probe)
                        {
                            bool async = false;
                            return probe.m_mediaPipeline->play(async);
                        });
    observeUnloadedCall("pause before play", [](ReferencePlayer &probe) { return probe.m_mediaPipeline->pause(); });
    observeUnloadedCall("stop before play", [](ReferencePlayer &probe) { return probe.m_mediaPipeline->stop(); });
    observeUnloadedCall("setPlaybackRate zero", [](ReferencePlayer &probe)
                        { return probe.m_mediaPipeline->setPlaybackRate(0.0); });
    observeUnloadedCall("setPlaybackRate negative", [](ReferencePlayer &probe)
                        { return probe.m_mediaPipeline->setPlaybackRate(-2.0); });
    observeUnloadedCall("setPosition negative", [](ReferencePlayer &probe)
                        { return probe.m_mediaPipeline->setPosition(-1); });
    observeUnloadedCall("setPosition out of range", [](ReferencePlayer &probe)
                        { return probe.m_mediaPipeline->setPosition(std::numeric_limits<int64_t>::max()); });
    observeUnloadedCall("setVolume below range", [](ReferencePlayer &probe)
                        { return probe.m_mediaPipeline->setVolume(-0.1); });
    observeUnloadedCall("setVolume above range", [](ReferencePlayer &probe)
                        { return probe.m_mediaPipeline->setVolume(1.5); });
    observeUnloadedCall("getPosition before load", [](ReferencePlayer &probe)
                        {
                            int64_t position = -1;
                            return probe.m_mediaPipeline->getPosition(position);
                        });
    return failures;
}

std::size_t ReferencePlayer::runPipelinePropertyNegativeCases()
{
    std::size_t failures = 0;
    const auto observe = [this, &failures](const std::string &name, const auto &call)
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

    observe("setLowLatency before play", [](ReferencePlayer &probe)
        { return probe.m_mediaPipeline->setLowLatency(true); });
    observe("setSync before play", [](ReferencePlayer &probe) { return probe.m_mediaPipeline->setSync(true); });
    observe("getSync before play", [](ReferencePlayer &probe)
        {
        bool sync = false;
        return probe.m_mediaPipeline->getSync(sync);
        });
    observe("setSyncOff before play", [](ReferencePlayer &probe) { return probe.m_mediaPipeline->setSyncOff(true); });
    observe("setMute invalid source", [](ReferencePlayer &probe)
        { return probe.m_mediaPipeline->setMute(-1, true); });
    observe("getMute invalid source", [](ReferencePlayer &probe)
        {
        bool mute = false;
        return probe.m_mediaPipeline->getMute(-1, mute);
        });
    observe("setVolume below range", [](ReferencePlayer &probe) { return probe.m_mediaPipeline->setVolume(-0.1); });
    observe("setVolume above range", [](ReferencePlayer &probe) { return probe.m_mediaPipeline->setVolume(1.5); });
    observe("getVolume before play", [](ReferencePlayer &probe)
        {
        double volume = 0.0;
        return probe.m_mediaPipeline->getVolume(volume);
        });
    observe("setTextTrackIdentifier empty", [](ReferencePlayer &probe)
        { return probe.m_mediaPipeline->setTextTrackIdentifier(""); });
    observe("setTextTrackIdentifier long", [](ReferencePlayer &probe)
        { return probe.m_mediaPipeline->setTextTrackIdentifier(std::string(4096, 'x')); });
    observe("getTextTrackIdentifier before play", [](ReferencePlayer &probe)
        {
        std::string textTrack;
        return probe.m_mediaPipeline->getTextTrackIdentifier(textTrack);
        });
    observe("setBufferingLimit zero", [](ReferencePlayer &probe) { return probe.m_mediaPipeline->setBufferingLimit(0); });
    observe("getBufferingLimit before play", [](ReferencePlayer &probe)
        {
        uint32_t limit = 0;
        return probe.m_mediaPipeline->getBufferingLimit(limit);
        });
    observe("setUseBuffering before play", [](ReferencePlayer &probe)
        { return probe.m_mediaPipeline->setUseBuffering(true); });
    observe("getUseBuffering before play", [](ReferencePlayer &probe)
        {
        bool useBuffering = false;
        return probe.m_mediaPipeline->getUseBuffering(useBuffering);
        });
    observe("setStreamSyncMode invalid source/mode", [](ReferencePlayer &probe)
        { return probe.m_mediaPipeline->setStreamSyncMode(-1, -1); });
    observe("getStreamSyncMode before play", [](ReferencePlayer &probe)
        {
        int32_t streamSyncMode = 0;
        return probe.m_mediaPipeline->getStreamSyncMode(streamSyncMode);
        });
    observe("setSourcePosition invalid source/rate", [](ReferencePlayer &probe)
        { return probe.m_mediaPipeline->setSourcePosition(-1, -1, true, 0.0, 0); });
    observe("setSubtitleOffset invalid source/position", [](ReferencePlayer &probe)
        { return probe.m_mediaPipeline->setSubtitleOffset(-1, -1); });
    observe("getDuration before play", [](ReferencePlayer &probe)
        {
        int64_t duration = 0;
        return probe.m_mediaPipeline->getDuration(duration);
        });
    observe("getPosition before play", [](ReferencePlayer &probe)
        {
        int64_t position = 0;
        return probe.m_mediaPipeline->getPosition(position);
        });
    return failures;
}

std::size_t ReferencePlayer::runSegmentSubmissionNegativeCases()
{
    std::size_t failures = 0;
    using MediaSegment = firebolt::rialto::IMediaPipeline::MediaSegment;
    using MediaSegmentVideo = firebolt::rialto::IMediaPipeline::MediaSegmentVideo;
    const uint32_t staleRequestId = std::numeric_limits<uint32_t>::max();

    auto emptySegmentProbe = createAttachedPipelineProbe();
    std::unique_ptr<MediaSegment> emptySegment;
    const auto emptySegmentResult = emptySegmentProbe
        ? emptySegmentProbe->m_mediaPipeline->addSegment(staleRequestId, emptySegment)
        : firebolt::rialto::AddSegmentStatus::ERROR;
    failures += !reportInitializationCheck("addSegment empty segment/stale request", "reject segment",
                                           "status=" + std::to_string(static_cast<int>(emptySegmentResult)),
                                           emptySegmentProbe && emptySegmentResult != firebolt::rialto::AddSegmentStatus::OK);
    destroyPipelineProbe(emptySegmentProbe);

    auto invalidSegmentProbe = createAttachedPipelineProbe();
    std::unique_ptr<MediaSegment> invalidSegment =
        std::make_unique<MediaSegmentVideo>(-1, -1, -1, 0, 0);
    const auto invalidSegmentResult = invalidSegmentProbe
        ? invalidSegmentProbe->m_mediaPipeline->addSegment(staleRequestId, invalidSegment)
        : firebolt::rialto::AddSegmentStatus::ERROR;
    failures += !reportInitializationCheck("addSegment invalid source/stale request", "reject segment",
                                           "status=" + std::to_string(static_cast<int>(invalidSegmentResult)),
                                           invalidSegmentProbe && invalidSegmentResult != firebolt::rialto::AddSegmentStatus::OK);
    destroyPipelineProbe(invalidSegmentProbe);

    const auto observeStaleHaveData = [this, &failures, staleRequestId](const std::string &name,
                                                                       firebolt::rialto::MediaSourceStatus status)
    {
        auto probe = createAttachedPipelineProbe();
        const bool result = probe && probe->m_mediaPipeline->haveData(status, staleRequestId);
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

std::size_t ReferencePlayer::runCapabilityNegativeCases()
{
    std::size_t failures = 0;
    auto pipelineFactory = firebolt::rialto::IMediaPipelineCapabilitiesFactory::createFactory();
    if (!pipelineFactory)
    {
        log("[NEG] SKIP capability cases: media pipeline capabilities factory unavailable");
        return failures;
    }
    auto capabilities = pipelineFactory->createMediaPipelineCapabilities();
    if (!capabilities)
    {
        failures += !reportInitializationCheck("createMediaPipelineCapabilities", "create object",
                                               "returned null", false);
        return failures;
    }

    const auto observeMimeList = [this, &failures, &capabilities](firebolt::rialto::MediaSourceType sourceType,
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
        log("[NEG] SKIP key-system capabilities: factory or capabilities unavailable");
    }
    else
    {
        const auto systems = keyCapabilities->getSupportedKeySystems();
        reportInitializationCheck("getSupportedKeySystems", "return a list",
                                  std::to_string(systems.size()) + " key system(s)", true);
    }
    return failures;
}

std::size_t ReferencePlayer::runWebAudioNegativeCases()
{
    std::size_t failures = 0;
    auto factory = firebolt::rialto::IWebAudioPlayerFactory::createFactory();
    if (!factory)
    {
        log("[NEG] SKIP WebAudio cases: factory unavailable");
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

    auto probe = std::make_shared<ReferencePlayer>(Options{});
    const auto client = std::static_pointer_cast<firebolt::rialto::IWebAudioPlayerClient>(probe);
    const std::weak_ptr<firebolt::rialto::IWebAudioPlayerClient> validClient = client;
    const std::weak_ptr<firebolt::rialto::IWebAudioPlayerClient> emptyClient;

    auto emptyClientPlayer = factory->createWebAudioPlayer(emptyClient, "audio/x-raw", 1, validConfig);
    failures += !reportInitializationCheck("createWebAudioPlayer empty client", "reject",
                                           emptyClientPlayer ? "created" : "rejected", !emptyClientPlayer);
    emptyClientPlayer.reset();

    std::weak_ptr<firebolt::rialto::IWebAudioPlayerClient> expiredClient;
    {
        auto expiredOwner = std::make_shared<ReferencePlayer>(Options{});
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

    probe->m_webAudioPlayer = factory->createWebAudioPlayer(validClient, "audio/x-raw", 1, validConfig);
    if (!probe->m_webAudioPlayer)
    {
        failures += !reportInitializationCheck("createWebAudioPlayer valid setup", "create player", "returned null",
                                               false);
        return failures;
    }

    const bool negativeVolumeAccepted = probe->m_webAudioPlayer->setVolume(-0.1);
    failures += !reportInitializationCheck("WebAudio setVolume below range", "reject",
                                           negativeVolumeAccepted ? "accepted" : "rejected",
                                           !negativeVolumeAccepted);
    const bool excessiveVolumeAccepted = probe->m_webAudioPlayer->setVolume(1.5);
    failures += !reportInitializationCheck("WebAudio setVolume above range", "reject",
                                           excessiveVolumeAccepted ? "accepted" : "rejected",
                                           !excessiveVolumeAccepted);

    double volume = 0.0;
    const bool gotVolume = probe->m_webAudioPlayer->getVolume(volume);
    reportInitializationCheck("WebAudio getVolume after invalid values", "return safely",
                              gotVolume ? std::to_string(volume) : "failed", true);

    uint32_t preferredFrames = 0;
    uint32_t maximumFrames = 0;
    bool deferredPlay = false;
    const bool gotDeviceInfo = probe->m_webAudioPlayer->getDeviceInfo(preferredFrames, maximumFrames, deferredPlay);
    reportInitializationCheck("WebAudio getDeviceInfo", "return safely",
                              gotDeviceInfo ? std::to_string(preferredFrames) + "/" +
                                                  std::to_string(maximumFrames)
                                            : "failed",
                              true);

    uint32_t availableFrames = 0;
    uint32_t delayFrames = 0;
    auto shmInfo = std::make_shared<firebolt::rialto::WebAudioShmInfo>();
    const bool gotBufferInfo = probe->m_webAudioPlayer->getBufferAvailable(availableFrames, shmInfo);
    reportInitializationCheck("WebAudio getBufferAvailable before play", "return safely",
                              gotBufferInfo ? std::to_string(availableFrames) + " frame(s)" : "failed", true);
    const bool gotDelay = probe->m_webAudioPlayer->getBufferDelay(delayFrames);
    reportInitializationCheck("WebAudio getBufferDelay before play", "return safely",
                              gotDelay ? std::to_string(delayFrames) + " frame(s)" : "failed", true);

    const bool pausedBeforePlay = probe->m_webAudioPlayer->pause();
    reportInitializationCheck("WebAudio pause before play", "return safely",
                              pausedBeforePlay ? "true" : "false", true);
    const bool played = probe->m_webAudioPlayer->play();
    reportInitializationCheck("WebAudio play", "return safely", played ? "true" : "false", true);
    const bool eos = probe->m_webAudioPlayer->setEos();
    reportInitializationCheck("WebAudio setEos", "return safely", eos ? "true" : "false", true);
    const bool repeatedEos = probe->m_webAudioPlayer->setEos();
    reportInitializationCheck("WebAudio repeated setEos", "return safely", repeatedEos ? "true" : "false", true);

    uint8_t emptyData = 0;
    const bool zeroFrameWrite = probe->m_webAudioPlayer->writeBuffer(0, &emptyData);
    reportInitializationCheck("WebAudio zero-frame write", "return safely",
                              zeroFrameWrite ? "true" : "false", true);
    std::array<uint8_t, 4> oneFrame{};
    const bool writeAfterEos = probe->m_webAudioPlayer->writeBuffer(1, oneFrame.data());
    reportInitializationCheck("WebAudio write after EOS", "return safely",
                              writeAfterEos ? "true" : "false", true);
    return failures;
}

std::size_t ReferencePlayer::runLoggingNegativeCases()
{
    auto factory = firebolt::rialto::IClientLogControlFactory::createFactory();
    if (!factory)
    {
        log("[NEG] SKIP logging cases: log-control factory unavailable");
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
/******** Negative checks for media pipeline and web audio creation end here *******/

} // namespace reference_player

int main(int argc, char **argv)
{
    reference_player::Options options;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--url" && i + 1 < argc) options.url = argv[++i];
        else if (arg == "--mime" && i + 1 < argc) options.mimeType = argv[++i];
        else if (arg == "--live") options.isLive = true;
        else if (arg == "--no-validate-capabilities") options.validateCapabilities = false;
        else if (arg == "--web-audio") options.useWebAudio = true;
        else if (arg == "--no-playback") options.enablePlayback = false;
        else if (arg == "--set-volume" && i + 1 < argc) options.desiredVolume = std::stod(argv[++i]);
        else if (arg == "--seek" && i + 1 < argc) options.seekPosition = std::stoll(argv[++i]);
        else if (arg == "--priority" && i + 1 < argc) options.priority = static_cast<uint32_t>(std::stoul(argv[++i]));
        else if (arg == "--help")
        {
            std::cout << "Usage: rialto_reference_player [options]\n"
                         "  --url <url>             Media URL; a local file:// .mp4 (H.264) is demuxed and fed over MSE\n"
                         "  --mime <mime>           Media MIME type (default: video/mp4)\n"
                         "  --live                  Mark media as live\n"
                         "  --web-audio             Also create an IWebAudioPlayer and validate the PCM API\n"
                         "  --set-volume <v>        Set media volume to [0.0, 1.0]\n"
                         "  --seek <ns>            Seek to a position in nanoseconds\n"
                         "  --priority <n>          Audio player priority (default: 1)\n"
                         "  --no-playback          Validate contract without calling play()\n"
                         "  --no-validate-capabilities Skip capability tests\n"
                         "  --help                 Print this help text\n";
            return 0;
        }
    }

    if (options.url.empty())
    {
        std::cout << "No URL provided; using a synthetic contract-only validation flow.\n";
    }

    auto player = reference_player::ReferencePlayer::create(options);
    if (!player)
    {
        std::cerr << "Failed to initialize the Rialto reference player." << std::endl;
        return 1;
    }

    return player->run();
}
