#pragma once

#ifdef REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER
#include "Mp4Demuxer.h"
#endif

#include <cstddef>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#ifdef REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER
#include <deque>
#include <fstream>
#include <thread>
#endif
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#ifdef REFERENCE_PLAYER_USE_MOCKS
namespace firebolt::rialto
{
enum class ApplicationState
{
    UNKNOWN,
    RUNNING,
    INACTIVE
};

enum class MediaType
{
    UNKNOWN,
    MSE
};

enum class MediaSourceType
{
    UNKNOWN,
    AUDIO,
    VIDEO,
    SUBTITLE
};

enum class SourceConfigType
{
    UNKNOWN,
    AUDIO,
    VIDEO,
    VIDEO_DOLBY_VISION,
    SUBTITLE
};

enum class NetworkState
{
    UNKNOWN,
    IDLE,
    BUFFERING,
    BUFFERING_PROGRESS,
    BUFFERED,
    STALLED,
    FORMAT_ERROR,
    NETWORK_ERROR,
    DECODE_ERROR
};

enum class PlaybackState
{
    UNKNOWN,
    IDLE,
    PLAYING,
    PAUSED,
    SEEKING,
    SEEK_DONE,
    STOPPED,
    END_OF_STREAM,
    FAILURE
};

enum class WebAudioPlayerState
{
    UNKNOWN,
    IDLE,
    PLAYING,
    PAUSED,
    END_OF_STREAM,
    FAILURE
};

struct VideoRequirements
{
    uint32_t maxWidth{1920};
    uint32_t maxHeight{1080};
};

struct WebAudioPcmConfig
{
    uint32_t rate{48000};
    uint32_t channels{2};
    uint32_t sampleSize{16};
    bool isBigEndian{false};
    bool isSigned{true};
    bool isFloat{false};
};

union WebAudioConfig
{
    WebAudioPcmConfig pcm;
};

struct WebAudioShmInfo
{
    uint32_t offsetMain{0};
    uint32_t lengthMain{0};
    uint32_t offsetWrap{0};
    uint32_t lengthWrap{0};
};

class IControlClient
{
public:
    virtual ~IControlClient() = default;
    virtual void notifyApplicationState(ApplicationState state) = 0;
};

class IControl
{
public:
    virtual ~IControl() = default;
    virtual bool registerClient(std::weak_ptr<IControlClient> client, ApplicationState &appState) = 0;
};

class MockControl final : public IControl
{
public:
    bool registerClient(std::weak_ptr<IControlClient> client, ApplicationState &appState) override
    {
        m_client = client.lock();
        appState = ApplicationState::RUNNING;
        if (m_client)
        {
            m_client->notifyApplicationState(appState);
        }
        return true;
    }

private:
    std::shared_ptr<IControlClient> m_client;
};

class IControlFactory
{
public:
    static std::shared_ptr<IControlFactory> createFactory();
    virtual std::shared_ptr<IControl> createControl() const = 0;
};

struct MediaPlayerShmInfo
{
    uint32_t maxMetadataBytes{0};
    uint32_t metadataOffset{0};
    uint32_t mediaDataOffset{0};
    uint32_t maxMediaBytes{0};
};

struct QosInfo
{
    uint64_t processed{0};
    uint64_t dropped{0};
};

struct PlaybackInfo
{
    int64_t currentPosition{-1};
    double volume{1.0};
};

enum class PlaybackError
{
    UNKNOWN,
    DECRYPTION,
    OUTPUT_PROTECTION
};

class IMediaPipelineClient
{
public:
    virtual ~IMediaPipelineClient() = default;
    virtual void notifyDuration(int64_t duration) = 0;
    virtual void notifyPosition(int64_t position) = 0;
    virtual void notifyNativeSize(uint32_t width, uint32_t height, double aspect = 1.0) = 0;
    virtual void notifyNetworkState(NetworkState state) = 0;
    virtual void notifyPlaybackState(PlaybackState state) = 0;
    virtual void notifyVideoData(bool hasData) = 0;
    virtual void notifyAudioData(bool hasData) = 0;
    virtual void notifyNeedMediaData(int32_t sourceId, size_t frameCount, uint32_t needDataRequestId,
                                     const std::shared_ptr<MediaPlayerShmInfo> &shmInfo) = 0;
    virtual void notifyCancelNeedMediaData(int32_t sourceId) = 0;
    virtual void notifyQos(int32_t sourceId, const QosInfo &qosInfo) = 0;
    virtual void notifyBufferUnderflow(int32_t sourceId) = 0;
    virtual void notifyFirstFrameReceived(int32_t sourceId) = 0;
    virtual void notifyPlaybackError(int32_t sourceId, PlaybackError error) = 0;
    virtual void notifySourceFlushed(int32_t sourceId) = 0;
    virtual void notifyPlaybackInfo(const PlaybackInfo &playbackInfo) = 0;
};

class IMediaPipeline
{
public:
    class MediaSource
    {
    public:
        virtual ~MediaSource() = default;
        virtual std::unique_ptr<MediaSource> copy() const = 0;
        virtual MediaSourceType getType() const { return MediaSourceType::UNKNOWN; }
        void setId(int32_t id) { m_id = id; }
        int32_t getId() const { return m_id; }

    protected:
        explicit MediaSource(SourceConfigType configType = SourceConfigType::UNKNOWN,
                             const std::string &mimeType = std::string(), bool hasDrm = true)
            : m_id(0), m_configType(configType), m_mimeType(mimeType), m_hasDrm(hasDrm)
        {
        }
        int32_t m_id{0};
        SourceConfigType m_configType{SourceConfigType::UNKNOWN};
        std::string m_mimeType;
        bool m_hasDrm{true};
    };

    class MediaSourceVideo : public MediaSource
    {
    public:
        MediaSourceVideo(const std::string &mimeType, bool hasDrm = true, int32_t width = 1920, int32_t height = 1080)
            : MediaSource(SourceConfigType::VIDEO, mimeType, hasDrm), m_width(width), m_height(height)
        {
        }
        std::unique_ptr<MediaSource> copy() const override { return std::make_unique<MediaSourceVideo>(*this); }
        MediaSourceType getType() const override { return MediaSourceType::VIDEO; }
        int32_t getWidth() const { return m_width; }
        int32_t getHeight() const { return m_height; }

    private:
        int32_t m_width{1920};
        int32_t m_height{1080};
    };

    virtual ~IMediaPipeline() = default;
    virtual std::weak_ptr<IMediaPipelineClient> getClient() = 0;
    virtual bool load(MediaType type, const std::string &mimeType, const std::string &url, bool isLive) = 0;
    virtual bool attachSource(const std::unique_ptr<MediaSource> &source) = 0;
    virtual bool removeSource(int32_t id) = 0;
    virtual bool allSourcesAttached() = 0;
    virtual bool play(bool &async) = 0;
    virtual bool pause() = 0;
    virtual bool stop() = 0;
    virtual bool setPlaybackRate(double rate) = 0;
    virtual bool setPosition(int64_t position) = 0;
    virtual bool getPosition(int64_t &position) = 0;
    virtual bool setVideoWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual bool setVolume(double targetVolume, uint32_t volumeDuration = 0, int easeType = 0) = 0;
    virtual bool getVolume(double &currentVolume) = 0;
    virtual bool setMute(int32_t sourceId, bool mute) = 0;
    virtual bool getMute(int32_t sourceId, bool &mute) = 0;
    virtual bool setTextTrackIdentifier(const std::string &textTrackIdentifier) = 0;
    virtual bool getTextTrackIdentifier(std::string &textTrackIdentifier) = 0;
    virtual bool setLowLatency(bool lowLatency) = 0;
    virtual bool setSync(bool sync) = 0;
    virtual bool getSync(bool &sync) = 0;
    virtual bool setSyncOff(bool syncOff) = 0;
    virtual bool setStreamSyncMode(int32_t sourceId, int32_t streamSyncMode) = 0;
    virtual bool getStreamSyncMode(int32_t &streamSyncMode) = 0;
    virtual bool flush(int32_t sourceId, bool resetTime, bool &async) = 0;
    virtual bool setSourcePosition(int32_t sourceId, int64_t position, bool resetTime = false, double appliedRate = 1.0,
                                   uint64_t stopPosition = 0ULL) = 0;
    virtual bool setSubtitleOffset(int32_t sourceId, int64_t position) = 0;
    virtual bool setBufferingLimit(uint32_t limitBufferingMs) = 0;
    virtual bool getBufferingLimit(uint32_t &limitBufferingMs) = 0;
    virtual bool setUseBuffering(bool useBuffering) = 0;
    virtual bool getUseBuffering(bool &useBuffering) = 0;
    virtual bool getDuration(int64_t &duration) = 0;
};

class IMediaPipelineFactory
{
public:
    static std::shared_ptr<IMediaPipelineFactory> createFactory();
    virtual std::unique_ptr<IMediaPipeline> createMediaPipeline(std::weak_ptr<IMediaPipelineClient> client,
                                                                const VideoRequirements &videoRequirements) const = 0;
};

class MockMediaPipeline final : public IMediaPipeline
{
public:
    explicit MockMediaPipeline(std::weak_ptr<IMediaPipelineClient> client = {}) : m_client(client) {}

    std::weak_ptr<IMediaPipelineClient> getClient() override { return m_client; }
    bool load(MediaType type, const std::string &mimeType, const std::string &url, bool isLive) override
    {
        (void)type; (void)mimeType; (void)url; (void)isLive; return true;
    }
    bool attachSource(const std::unique_ptr<MediaSource> &source) override
    {
        if (source)
        {
            source->setId(++m_nextSourceId);
        }
        return true;
    }
    bool removeSource(int32_t id) override { (void)id; return true; }
    bool allSourcesAttached() override { return true; }
    bool play(bool &async) override { async = false; return true; }
    bool pause() override { return true; }
    bool stop() override { return true; }
    bool setPlaybackRate(double rate) override { (void)rate; return true; }
    bool setPosition(int64_t position) override { (void)position; return true; }
    bool getPosition(int64_t &position) override { position = 0; return true; }
    bool setVideoWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override
    {
        (void)x; (void)y; (void)width; (void)height; return true;
    }
    bool setVolume(double targetVolume, uint32_t volumeDuration = 0, int easeType = 0) override
    {
        (void)targetVolume; (void)volumeDuration; (void)easeType; return true;
    }
    bool getVolume(double &currentVolume) override { currentVolume = 0.5; return true; }
    bool setMute(int32_t sourceId, bool mute) override { (void)sourceId; (void)mute; return true; }
    bool getMute(int32_t sourceId, bool &mute) override { (void)sourceId; mute = false; return true; }
    bool setTextTrackIdentifier(const std::string &textTrackIdentifier) override { (void)textTrackIdentifier; return true; }
    bool getTextTrackIdentifier(std::string &textTrackIdentifier) override { textTrackIdentifier = "eng"; return true; }
    bool setLowLatency(bool lowLatency) override { (void)lowLatency; return true; }
    bool setSync(bool sync) override { (void)sync; return true; }
    bool getSync(bool &sync) override { sync = true; return true; }
    bool setSyncOff(bool syncOff) override { (void)syncOff; return true; }
    bool setStreamSyncMode(int32_t sourceId, int32_t streamSyncMode) override { (void)sourceId; (void)streamSyncMode; return true; }
    bool getStreamSyncMode(int32_t &streamSyncMode) override { streamSyncMode = 1; return true; }
    bool flush(int32_t sourceId, bool resetTime, bool &async) override { (void)sourceId; (void)resetTime; async = false; return true; }
    bool setSourcePosition(int32_t sourceId, int64_t position, bool resetTime = false, double appliedRate = 1.0,
                           uint64_t stopPosition = 0ULL) override
    {
        (void)sourceId; (void)position; (void)resetTime; (void)appliedRate; (void)stopPosition; return true;
    }
    bool setSubtitleOffset(int32_t sourceId, int64_t position) override { (void)sourceId; (void)position; return true; }
    bool setBufferingLimit(uint32_t limitBufferingMs) override { (void)limitBufferingMs; return true; }
    bool getBufferingLimit(uint32_t &limitBufferingMs) override { limitBufferingMs = 1500; return true; }
    bool setUseBuffering(bool useBuffering) override { (void)useBuffering; return true; }
    bool getUseBuffering(bool &useBuffering) override { useBuffering = true; return true; }
    bool getDuration(int64_t &duration) override { duration = 30000000000LL; return true; }

private:
    std::weak_ptr<IMediaPipelineClient> m_client;
    int32_t m_nextSourceId{0};
};

class IWebAudioPlayerClient
{
public:
    virtual ~IWebAudioPlayerClient() = default;
    virtual void notifyState(WebAudioPlayerState state) = 0;
};

class IWebAudioPlayer
{
public:
    virtual ~IWebAudioPlayer() = default;
    virtual bool play() = 0;
    virtual bool pause() = 0;
    virtual bool setEos() = 0;
    virtual bool getBufferAvailable(uint32_t &availableFrames, std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo) = 0;
    virtual bool getBufferDelay(uint32_t &delayFrames) = 0;
    virtual bool writeBuffer(const uint32_t numberOfFrames, void *data) = 0;
    virtual bool getDeviceInfo(uint32_t &preferredFrames, uint32_t &maximumFrames, bool &supportDeferredPlay) = 0;
    virtual bool setVolume(double volume) = 0;
    virtual bool getVolume(double &volume) = 0;
    virtual std::weak_ptr<IWebAudioPlayerClient> getClient() = 0;
};

class IWebAudioPlayerFactory
{
public:
    static std::shared_ptr<IWebAudioPlayerFactory> createFactory();
    virtual std::unique_ptr<IWebAudioPlayer> createWebAudioPlayer(std::weak_ptr<IWebAudioPlayerClient> client,
                                                                  const std::string &audioMimeType,
                                                                  const uint32_t priority,
                                                                  std::weak_ptr<const WebAudioConfig> config) const = 0;
};

class MockWebAudioPlayer final : public IWebAudioPlayer
{
public:
    explicit MockWebAudioPlayer(std::weak_ptr<IWebAudioPlayerClient> client = {}) : m_client(client) {}

    bool play() override { return true; }
    bool pause() override { return true; }
    bool setEos() override { return true; }
    bool getBufferAvailable(uint32_t &availableFrames, std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo) override
    {
        webAudioShmInfo = std::make_shared<WebAudioShmInfo>();
        webAudioShmInfo->lengthMain = 4096;
        availableFrames = 512;
        return true;
    }
    bool getBufferDelay(uint32_t &delayFrames) override { delayFrames = 0U; return true; }
    bool writeBuffer(const uint32_t numberOfFrames, void *data) override { (void)numberOfFrames; (void)data; return true; }
    bool getDeviceInfo(uint32_t &preferredFrames, uint32_t &maximumFrames, bool &supportDeferredPlay) override
    {
        preferredFrames = 256;
        maximumFrames = 4096;
        supportDeferredPlay = true;
        return true;
    }
    bool setVolume(double volume) override { (void)volume; return true; }
    bool getVolume(double &volume) override { volume = 0.5; return true; }
    std::weak_ptr<IWebAudioPlayerClient> getClient() override { return m_client; }

private:
    std::weak_ptr<IWebAudioPlayerClient> m_client;
};

class IMediaPipelineCapabilities
{
public:
    virtual ~IMediaPipelineCapabilities() = default;
    virtual std::vector<std::string> getSupportedMimeTypes(MediaSourceType sourceType) = 0;
    virtual bool isMimeTypeSupported(const std::string &mimeType) = 0;
    virtual std::vector<std::string> getSupportedProperties(MediaSourceType mediaType,
                                                            const std::vector<std::string> &propertyNames) = 0;
    virtual bool isVideoMaster(bool &isVideoMaster) = 0;
};

class MockMediaPipelineCapabilities final : public IMediaPipelineCapabilities
{
public:
    std::vector<std::string> getSupportedMimeTypes(MediaSourceType sourceType) override
    {
        if (sourceType == MediaSourceType::VIDEO)
        {
            return {"video/mp4"};
        }
        if (sourceType == MediaSourceType::AUDIO)
        {
            return {"audio/mp4"};
        }
        return {};
    }
    bool isMimeTypeSupported(const std::string &mimeType) override { return mimeType == "video/mp4" || mimeType == "audio/mp4"; }
    std::vector<std::string> getSupportedProperties(MediaSourceType mediaType,
                                                   const std::vector<std::string> &propertyNames) override
    {
        (void)mediaType; return propertyNames;
    }
    bool isVideoMaster(bool &isVideoMaster) override { isVideoMaster = true; return true; }
};

class IMediaPipelineCapabilitiesFactory
{
public:
    static std::shared_ptr<IMediaPipelineCapabilitiesFactory> createFactory();
    virtual std::unique_ptr<IMediaPipelineCapabilities> createMediaPipelineCapabilities() const = 0;
};

class IMediaKeysCapabilities
{
public:
    virtual ~IMediaKeysCapabilities() = default;
    virtual std::vector<std::string> getSupportedKeySystems() = 0;
    virtual bool supportsKeySystem(const std::string &keySystem) = 0;
    virtual bool getSupportedKeySystemVersion(const std::string &keySystem, std::string &version) = 0;
    virtual bool isServerCertificateSupported(const std::string &keySystem) = 0;
};

class MockMediaKeysCapabilities final : public IMediaKeysCapabilities
{
public:
    std::vector<std::string> getSupportedKeySystems() override { return {"com.widevine.alpha", "com.microsoft.playready"}; }
    bool supportsKeySystem(const std::string &keySystem) override { return keySystem == "com.widevine.alpha"; }
    bool getSupportedKeySystemVersion(const std::string &keySystem, std::string &version) override
    {
        (void)keySystem;
        version = "1.0";
        return true;
    }
    bool isServerCertificateSupported(const std::string &keySystem) override { return keySystem == "com.widevine.alpha"; }
};

class IMediaKeysCapabilitiesFactory
{
public:
    static std::shared_ptr<IMediaKeysCapabilitiesFactory> createFactory();
    virtual std::shared_ptr<IMediaKeysCapabilities> getMediaKeysCapabilities() const = 0;
};

class IClientLogHandler
{
public:
    enum class Level
    {
        Fatal,
        Error,
        Warning,
        Milestone,
        Info,
        Debug,
        External
    };
    virtual ~IClientLogHandler() = default;
    virtual void log(Level level, const std::string &file, int line, const std::string &function,
                     const std::string &message) = 0;
};

class IClientLogControl
{
public:
    virtual ~IClientLogControl() = default;
    virtual bool registerLogHandler(const std::shared_ptr<IClientLogHandler> &handler, bool ignoreLogLevels) = 0;
};

class MockClientLogControl final : public IClientLogControl
{
public:
    bool registerLogHandler(const std::shared_ptr<IClientLogHandler> &handler, bool ignoreLogLevels) override
    {
        (void)handler; (void)ignoreLogLevels; return true;
    }
};

class IClientLogControlFactory
{
public:
    static std::shared_ptr<IClientLogControlFactory> createFactory();
    virtual IClientLogControl &createClientLogControl() = 0;
};
} // namespace firebolt::rialto
#else
#include "IClientLogControl.h"
#include "IClientLogHandler.h"
#include "IControl.h"
#include "IMediaPipeline.h"
#include "IMediaPipelineCapabilities.h"
#include "IMediaKeysCapabilities.h"
#include "IWebAudioPlayer.h"
#endif

namespace reference_player
{
struct Options
{
    std::string url;
    std::string mimeType{"video/mp4"};
    bool isLive{false};
    bool validateCapabilities{true};
    bool useWebAudio{false};
    bool enablePlayback{true};
    std::optional<double> desiredVolume;
    std::optional<int64_t> seekPosition;
    uint32_t audioRate{48000};
    uint32_t audioChannels{2};
    uint32_t audioSampleSize{16};
    uint32_t priority{1};
};

class ReferencePlayer : public std::enable_shared_from_this<ReferencePlayer>,
                       public firebolt::rialto::IControlClient,
                       public firebolt::rialto::IMediaPipelineClient,
                       public firebolt::rialto::IWebAudioPlayerClient
{
public:
    explicit ReferencePlayer(Options options = {});
    ~ReferencePlayer() override;

    static std::shared_ptr<ReferencePlayer> create(Options options);
    bool initialize();
    int run();

    void notifyApplicationState(firebolt::rialto::ApplicationState state) override;
    void notifyDuration(int64_t duration) override;
    void notifyPosition(int64_t position) override;
    void notifyNativeSize(uint32_t width, uint32_t height, double aspect = 1.0) override;
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

private:
    bool createControl();
    bool connectControl();
    bool createMediaPipeline();
    bool createMediaPipeline(const firebolt::rialto::VideoRequirements &requirements);
    int runNegativeChecks();
    std::size_t runCreateControlNegativeCases();
    std::size_t runRegisterClientNegativeCases();
    std::size_t runCreateMediaPipelineNegativeCases();
    std::size_t runLoadNegativeCases();
    std::size_t runAttachSourceNegativeCases();
    std::size_t runSourceIdChecks();
    std::size_t runAllSourcesAttachedNegativeCases();
    std::size_t runPlaybackControlNegativeCases();
    std::size_t runPipelinePropertyNegativeCases();
    std::size_t runSegmentSubmissionNegativeCases();
    std::size_t runCapabilityNegativeCases();
    std::size_t runWebAudioNegativeCases();
    std::size_t runLoggingNegativeCases();
    std::shared_ptr<ReferencePlayer> createPipelineProbe();
    std::shared_ptr<ReferencePlayer> createLoadedPipelineProbe();
    std::shared_ptr<ReferencePlayer> createAttachedPipelineProbe();
    void destroyPipelineProbe(std::shared_ptr<ReferencePlayer> &probe);
    bool reportInitializationCheck(const std::string &name, const std::string &expected,
                                   const std::string &observed, bool passed) const;
    bool createWebAudio();
    bool validateContract();
    bool validateAdvancedMediaPipelineFeatures();
    bool validateCapabilities();
    bool validateWebAudio();
    bool validateLogging();
    bool loadWithSource();
    bool playMedia();
    bool waitForPlayback();
    bool waitForPlaybackComplete();
    bool seekIfRequested();

#ifdef REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER
    struct NeedDataRequest
    {
        int32_t sourceId;
        size_t frameCount;
        uint32_t requestId;
    };
    bool loadMp4();
    void feedLoop();
    void feedRequest(const NeedDataRequest &request);
#endif
    void stopFeeder();
    bool applyVolumeIfRequested();

    static std::string toString(firebolt::rialto::ApplicationState state);
    static std::string toString(firebolt::rialto::PlaybackState state);
    static std::string toString(firebolt::rialto::NetworkState state);
    static std::string toString(firebolt::rialto::WebAudioPlayerState state);

    void log(const std::string &message) const;

    Options m_options;
    std::shared_ptr<firebolt::rialto::IControl> m_control;
    std::shared_ptr<firebolt::rialto::IMediaPipeline> m_mediaPipeline;
    std::unique_ptr<firebolt::rialto::IWebAudioPlayer> m_webAudioPlayer;
    std::atomic<bool> m_applicationRunning{false};
    std::atomic<int64_t> m_lastDuration{0};
    std::atomic<int64_t> m_lastPosition{0};
    std::atomic<bool> m_lastVideoData{false};
    std::atomic<bool> m_lastAudioData{false};
    std::atomic<bool> m_playbackStarted{false};
    std::atomic<bool> m_playbackFailed{false};
    std::atomic<bool> m_playbackComplete{false};
    std::atomic<int32_t> m_currentSourceId{0};
    std::mutex m_stateMutex;
    std::condition_variable m_stateCv;

    std::atomic<int32_t> m_audioSourceId{0};
#ifdef REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER
    // MP4 playback: frames are read from m_file and fed over MSE from m_feedThread, not the Rialto IPC thread.
    bool m_feedEnabled{false};
    bool m_feedStop{false};
    std::ifstream m_file;
    std::vector<mp4_demuxer::Sample> m_samples;
    std::vector<mp4_demuxer::Sample> m_audioSamples;
    std::vector<uint8_t> m_codecData;
    std::vector<uint8_t> m_audioCodecData;
    int32_t m_videoWidth{0};
    int32_t m_videoHeight{0};
    int32_t m_audioSampleRate{0};
    int32_t m_audioChannels{0};
    size_t m_nextSample{0};
    size_t m_nextAudioSample{0};
    bool m_audioTrackEnabled{false};
    std::deque<NeedDataRequest> m_needDataQueue;
    std::mutex m_feedMutex;
    std::condition_variable m_feedCv;
    std::thread m_feedThread;
#endif

};
} // namespace reference_player
