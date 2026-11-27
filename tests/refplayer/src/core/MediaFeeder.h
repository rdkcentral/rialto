#pragma once

#include "Logger.h"
#include "TrackModel.h"

#include "IMediaPipeline.h"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <istream>
#include <memory>
#include <mutex>
#include <thread>

namespace rialto_refplayer
{

// Answers needData requests by reading samples and pushing them with addSegment().
// Runs on its own thread. The sample cursors and the input stream belong to this thread
// while it runs, so stop() must return before the pipeline, the track model or the
// stream is touched again.
class MediaFeeder
{
public:
    explicit MediaFeeder(std::shared_ptr<Logger> logger);
    ~MediaFeeder();

    MediaFeeder(const MediaFeeder &) = delete;
    MediaFeeder &operator=(const MediaFeeder &) = delete;

    // Starts a new session. Restarting an active feeder stops the old one first.
    void start(firebolt::rialto::IMediaPipeline &pipeline, const demux::TrackModel &tracks, std::istream &input,
               int32_t videoSourceId, int32_t audioSourceId);

    // Sets the stop flag, wakes the thread and joins it. Idempotent.
    void stop();

    // Called on Rialto IPC threads. Queues the request and returns at once.
    void enqueue(int32_t sourceId, size_t frameCount, uint32_t requestId);

    // Drops queued requests for one source.
    void cancel(int32_t sourceId);

private:
    struct Request
    {
        int32_t sourceId;
        size_t frameCount;
        uint32_t requestId;
    };

    void run();
    void feedRequest(const Request &request);

    std::shared_ptr<Logger> m_logger;

    // Session state, written by start() and read by the feeder thread only.
    firebolt::rialto::IMediaPipeline *m_pipeline{nullptr};
    const demux::TrackModel *m_tracks{nullptr};
    std::istream *m_input{nullptr};
    int32_t m_videoSourceId{0};
    int32_t m_audioSourceId{0};
    size_t m_nextVideoSample{0};
    size_t m_nextAudioSample{0};

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<Request> m_queue;
    bool m_active{false};
    bool m_stop{false};
    std::thread m_thread;
};

} // namespace rialto_refplayer
