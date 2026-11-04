// Mock of the Rialto public media pipeline interface. Mirrors the subset of
// /usr/include/rialto/IMediaPipeline.h that the reference player calls. Mock mode only.
//
// MockMediaPipeline behaves like a minimal server. It answers each call with the
// notification a real server would send, on its own callback thread, so the
// library's threading is exercised: play() yields PLAYING, setPosition() yields
// SEEKING then SEEK_DONE, attached sources receive needData, and EOS arrives after
// the last sample is acknowledged. Counters record pipeline lifetime and feed traffic.
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace firebolt::rialto
{

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

enum class PlaybackError
{
    UNKNOWN,
    DECRYPTION,
    OUTPUT_PROTECTION
};

enum class SegmentAlignment
{
    UNDEFINED,
    NAL,
    AU
};

enum class StreamFormat
{
    UNDEFINED,
    AVC,
    HEVC
};

enum class CodecDataType
{
    UNDEFINED,
    BUFFER,
    STRING
};

enum class AddSegmentStatus
{
    OK,
    ERROR,
    NO_SPACE
};

enum class MediaSourceStatus
{
    OK,
    EOS,
    ERROR,
    NO_AVAILABLE_SAMPLES
};

struct VideoRequirements
{
    uint32_t maxWidth{1920};
    uint32_t maxHeight{1080};
};

struct CodecData
{
    std::vector<uint8_t> data;
    CodecDataType type{CodecDataType::UNDEFINED};
};

struct AudioConfig
{
    uint32_t numberOfChannels{0};
    uint32_t sampleRate{0};
    std::vector<uint8_t> codecSpecificConfig;
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
        void setId(int32_t id) { m_id = id; }
        int32_t getId() const { return m_id; }
        virtual MediaSourceType getType() const { return MediaSourceType::UNKNOWN; }

    protected:
        MediaSource(SourceConfigType configType, const std::string &mimeType, bool hasDrm)
            : m_configType(configType), m_mimeType(mimeType), m_hasDrm(hasDrm)
        {
        }
        int32_t m_id{0};
        SourceConfigType m_configType{SourceConfigType::UNKNOWN};
        std::string m_mimeType;
        bool m_hasDrm{false};
    };

    class MediaSourceVideo : public MediaSource
    {
    public:
        MediaSourceVideo(const std::string &mimeType, bool hasDrm = true, int32_t width = 1920,
                         int32_t height = 1080, SegmentAlignment alignment = SegmentAlignment::UNDEFINED,
                         StreamFormat format = StreamFormat::UNDEFINED,
                         std::shared_ptr<CodecData> codecData = nullptr)
            : MediaSource(SourceConfigType::VIDEO, mimeType, hasDrm), m_width(width), m_height(height),
              m_alignment(alignment), m_format(format), m_codecData(std::move(codecData))
        {
        }
        MediaSourceType getType() const override { return MediaSourceType::VIDEO; }
        int32_t getWidth() const { return m_width; }
        int32_t getHeight() const { return m_height; }

    private:
        int32_t m_width{0};
        int32_t m_height{0};
        SegmentAlignment m_alignment{SegmentAlignment::UNDEFINED};
        StreamFormat m_format{StreamFormat::UNDEFINED};
        std::shared_ptr<CodecData> m_codecData;
    };

    class MediaSourceAudio : public MediaSource
    {
    public:
        MediaSourceAudio(const std::string &mimeType, bool hasDrm, const AudioConfig &config,
                         SegmentAlignment alignment)
            : MediaSource(SourceConfigType::AUDIO, mimeType, hasDrm), m_config(config), m_alignment(alignment)
        {
        }
        MediaSourceType getType() const override { return MediaSourceType::AUDIO; }

    private:
        AudioConfig m_config;
        SegmentAlignment m_alignment{SegmentAlignment::UNDEFINED};
    };

    class MediaSegment
    {
    public:
        virtual ~MediaSegment() = default;
        void setData(uint32_t size, uint8_t *data)
        {
            m_size = size;
            m_data = data;
        }
        void setSegmentAlignment(SegmentAlignment alignment) { m_alignment = alignment; }
        uint32_t size() const { return m_size; }

    protected:
        MediaSegment(int32_t sourceId, int64_t timestamp, int64_t duration)
            : m_sourceId(sourceId), m_timestamp(timestamp), m_duration(duration)
        {
        }
        int32_t m_sourceId{0};
        int64_t m_timestamp{0};
        int64_t m_duration{0};
        uint32_t m_size{0};
        uint8_t *m_data{nullptr};
        SegmentAlignment m_alignment{SegmentAlignment::UNDEFINED};
    };

    class MediaSegmentVideo : public MediaSegment
    {
    public:
        MediaSegmentVideo(int32_t sourceId, int64_t timestamp, int64_t duration, int32_t width, int32_t height)
            : MediaSegment(sourceId, timestamp, duration), m_width(width), m_height(height)
        {
        }

    private:
        int32_t m_width{0};
        int32_t m_height{0};
    };

    class MediaSegmentAudio : public MediaSegment
    {
    public:
        MediaSegmentAudio(int32_t sourceId, int64_t timestamp, int64_t duration, int32_t sampleRate,
                          int32_t channels)
            : MediaSegment(sourceId, timestamp, duration), m_sampleRate(sampleRate), m_channels(channels)
        {
        }

    private:
        int32_t m_sampleRate{0};
        int32_t m_channels{0};
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
    virtual bool setSourcePosition(int32_t sourceId, int64_t position, bool resetTime = false,
                                   double appliedRate = 1.0, uint64_t stopPosition = 0ULL) = 0;
    virtual bool setSubtitleOffset(int32_t sourceId, int64_t position) = 0;
    virtual bool setBufferingLimit(uint32_t limitBufferingMs) = 0;
    virtual bool getBufferingLimit(uint32_t &limitBufferingMs) = 0;
    virtual bool setUseBuffering(bool useBuffering) = 0;
    virtual bool getUseBuffering(bool &useBuffering) = 0;
    virtual bool getDuration(int64_t &duration) = 0;
    virtual AddSegmentStatus addSegment(uint32_t needDataRequestId,
                                        const std::unique_ptr<MediaSegment> &segment) = 0;
    virtual bool haveData(MediaSourceStatus status, uint32_t needDataRequestId) = 0;
};

// Test-visible counters for the scripted server. Mock mode only.
struct MockPipelineCounters
{
    std::atomic<int> livePipelines{0};
    std::atomic<int> segmentsAccepted{0};
    std::atomic<int> eosAcknowledged{0};
    std::atomic<int> stopsIssued{0};
    // Needdata requests the server will still issue; -1 is unlimited. A test sets a small
    // budget to hold the pipeline in PLAYING without reaching EOS.
    std::atomic<int> requestBudget{-1};
    // Delay before the server acknowledges stop(), in ms. A real server takes time here, which
    // is the window in which a load() can arrive while the stop is still in progress.
    std::atomic<int> stopDelayMs{0};
};
inline MockPipelineCounters g_mockPipelineCounters;

class MockMediaPipeline final : public IMediaPipeline
{
public:
    explicit MockMediaPipeline(std::weak_ptr<IMediaPipelineClient> client) : m_client(std::move(client))
    {
        ++g_mockPipelineCounters.livePipelines;
        m_worker = std::thread(&MockMediaPipeline::workerLoop, this);
    }

    ~MockMediaPipeline() override
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shutdown = true;
            m_jobs.clear();
        }
        m_cv.notify_all();
        if (m_worker.joinable())
        {
            m_worker.join();
        }
        --g_mockPipelineCounters.livePipelines;
    }

    std::weak_ptr<IMediaPipelineClient> getClient() override { return m_client; }

    bool load(MediaType type, const std::string &mimeType, const std::string &url, bool isLive) override
    {
        (void)type; (void)mimeType; (void)url; (void)isLive;
        return true;
    }

    bool attachSource(const std::unique_ptr<MediaSource> &source) override
    {
        if (!source)
        {
            return false;
        }
        source->setId(++m_nextSourceId);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sourceIds.push_back(source->getId());
        return true;
    }

    bool removeSource(int32_t id) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sourceIds.erase(std::remove(m_sourceIds.begin(), m_sourceIds.end(), id), m_sourceIds.end());
        return true;
    }

    bool allSourcesAttached() override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const int32_t id : m_sourceIds)
        {
            enqueueLocked([this, id](IMediaPipelineClient &c) { requestData(c, id); });
        }
        return true;
    }

    bool play(bool &async) override
    {
        async = true;
        post([](IMediaPipelineClient &c) {
            c.notifyPlaybackState(PlaybackState::PLAYING);
            c.notifyPosition(0);
        });
        return true;
    }

    bool pause() override
    {
        post([](IMediaPipelineClient &c) { c.notifyPlaybackState(PlaybackState::PAUSED); });
        return true;
    }

    bool stop() override
    {
        ++g_mockPipelineCounters.stopsIssued;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_jobs.clear(); // a stopped server sends nothing but STOPPED
        }
        const int delayMs = g_mockPipelineCounters.stopDelayMs.load();
        post([delayMs](IMediaPipelineClient &c) {
            if (delayMs > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            }
            c.notifyPlaybackState(PlaybackState::STOPPED);
        });
        return true;
    }

    bool setPlaybackRate(double rate) override { (void)rate; return true; }

    bool setPosition(int64_t position) override
    {
        post([position](IMediaPipelineClient &c) {
            c.notifyPlaybackState(PlaybackState::SEEKING);
            c.notifyPosition(position);
            c.notifyPlaybackState(PlaybackState::SEEK_DONE);
        });
        return true;
    }

    bool getPosition(int64_t &position) override { position = 0; return true; }

    bool setVideoWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override
    {
        (void)x; (void)y; (void)width; (void)height;
        return true;
    }

    bool setVolume(double targetVolume, uint32_t volumeDuration = 0, int easeType = 0) override
    {
        (void)volumeDuration; (void)easeType;
        m_volume = targetVolume;
        return true;
    }

    bool getVolume(double &currentVolume) override { currentVolume = m_volume; return true; }

    bool setMute(int32_t sourceId, bool mute) override
    {
        m_mute[sourceId] = mute;
        return true;
    }

    bool getMute(int32_t sourceId, bool &mute) override
    {
        const auto it = m_mute.find(sourceId);
        mute = it != m_mute.end() && it->second;
        return true;
    }
    bool setTextTrackIdentifier(const std::string &id) override { (void)id; return true; }
    bool getTextTrackIdentifier(std::string &id) override { id = "eng"; return true; }
    bool setLowLatency(bool lowLatency) override { (void)lowLatency; return true; }
    bool setSync(bool sync) override { (void)sync; return true; }
    bool getSync(bool &sync) override { sync = true; return true; }
    bool setSyncOff(bool syncOff) override { (void)syncOff; return true; }
    bool setStreamSyncMode(int32_t sourceId, int32_t mode) override { (void)sourceId; (void)mode; return true; }
    bool getStreamSyncMode(int32_t &mode) override { mode = 1; return true; }
    bool flush(int32_t sourceId, bool resetTime, bool &async) override
    {
        (void)sourceId; (void)resetTime; async = false;
        return true;
    }
    bool setSourcePosition(int32_t sourceId, int64_t position, bool resetTime = false, double appliedRate = 1.0,
                           uint64_t stopPosition = 0ULL) override
    {
        (void)sourceId; (void)position; (void)resetTime; (void)appliedRate; (void)stopPosition;
        return true;
    }
    bool setSubtitleOffset(int32_t sourceId, int64_t position) override { (void)sourceId; (void)position; return true; }
    bool setBufferingLimit(uint32_t limitBufferingMs) override { (void)limitBufferingMs; return true; }
    bool getBufferingLimit(uint32_t &limitBufferingMs) override { limitBufferingMs = 1500; return true; }
    bool setUseBuffering(bool useBuffering) override { (void)useBuffering; return true; }
    bool getUseBuffering(bool &useBuffering) override { useBuffering = true; return true; }
    bool getDuration(int64_t &duration) override { duration = 10000000000LL; return true; }

    AddSegmentStatus addSegment(uint32_t needDataRequestId, const std::unique_ptr<MediaSegment> &segment) override
    {
        (void)needDataRequestId;
        if (!segment)
        {
            return AddSegmentStatus::ERROR;
        }
        ++g_mockPipelineCounters.segmentsAccepted;
        return AddSegmentStatus::OK;
    }

    bool haveData(MediaSourceStatus status, uint32_t needDataRequestId) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto request = m_requestSource.find(needDataRequestId);
        if (request == m_requestSource.end())
        {
            return false;
        }
        const int32_t sourceId = request->second;
        m_requestSource.erase(request);

        if (status == MediaSourceStatus::EOS)
        {
            ++g_mockPipelineCounters.eosAcknowledged;
            m_eosSources.insert(sourceId);
            if (m_eosSources.size() == m_sourceIds.size())
            {
                enqueueLocked([](IMediaPipelineClient &c) { c.notifyPlaybackState(PlaybackState::END_OF_STREAM); });
            }
        }
        else if (status == MediaSourceStatus::OK)
        {
            // The server asks again for the source that was just served.
            enqueueLocked([this, sourceId](IMediaPipelineClient &c) { requestData(c, sourceId); });
        }
        return true;
    }

private:
    using Job = std::function<void(IMediaPipelineClient &)>;

    void requestData(IMediaPipelineClient &client, int32_t sourceId)
    {
        uint32_t requestId = 0;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            int budget = g_mockPipelineCounters.requestBudget.load();
            if (budget == 0)
            {
                return;
            }
            if (budget > 0)
            {
                g_mockPipelineCounters.requestBudget = budget - 1;
            }
            requestId = ++m_nextRequestId;
            m_requestSource[requestId] = sourceId;
        }
        client.notifyNeedMediaData(sourceId, 8, requestId, nullptr);
    }

    void post(Job job)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        enqueueLocked(std::move(job));
    }

    void enqueueLocked(Job job)
    {
        m_jobs.push_back(std::move(job));
        m_cv.notify_one();
    }

    void workerLoop()
    {
        while (true)
        {
            Job job;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] { return m_shutdown || !m_jobs.empty(); });
                if (m_shutdown)
                {
                    return;
                }
                job = std::move(m_jobs.front());
                m_jobs.pop_front();
            }
            if (const auto client = m_client.lock())
            {
                job(*client);
            }
        }
    }

    std::weak_ptr<IMediaPipelineClient> m_client;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<Job> m_jobs;
    std::vector<int32_t> m_sourceIds;
    bool m_shutdown{false};
    double m_volume{0.5};
    std::map<int32_t, bool> m_mute;
    int32_t m_nextSourceId{0};
    uint32_t m_nextRequestId{0};
    std::map<uint32_t, int32_t> m_requestSource;
    std::set<int32_t> m_eosSources;
    std::thread m_worker;
};

class IMediaPipelineFactory
{
public:
    virtual ~IMediaPipelineFactory() = default;
    static std::shared_ptr<IMediaPipelineFactory> createFactory();
    virtual std::unique_ptr<IMediaPipeline> createMediaPipeline(std::weak_ptr<IMediaPipelineClient> client,
                                                                const VideoRequirements &videoRequirements) const = 0;
};

class MockMediaPipelineFactory final : public IMediaPipelineFactory
{
public:
    std::unique_ptr<IMediaPipeline> createMediaPipeline(std::weak_ptr<IMediaPipelineClient> client,
                                                        const VideoRequirements &videoRequirements) const override
    {
        (void)videoRequirements;
        return std::make_unique<MockMediaPipeline>(std::move(client));
    }
};

inline std::shared_ptr<IMediaPipelineFactory> IMediaPipelineFactory::createFactory()
{
    return std::make_shared<MockMediaPipelineFactory>();
}

} // namespace firebolt::rialto
