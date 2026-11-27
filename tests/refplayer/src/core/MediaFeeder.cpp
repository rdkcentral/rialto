#include "MediaFeeder.h"

#include <algorithm>
#include <vector>

namespace rialto_refplayer
{

using firebolt::rialto::AddSegmentStatus;
using firebolt::rialto::MediaSourceStatus;

MediaFeeder::MediaFeeder(std::shared_ptr<Logger> logger) : m_logger(std::move(logger))
{
}

MediaFeeder::~MediaFeeder()
{
    stop();
}

void MediaFeeder::start(firebolt::rialto::IMediaPipeline &pipeline, const demux::TrackModel &tracks,
                        std::istream &input, int32_t videoSourceId, int32_t audioSourceId)
{
    stop();

    m_pipeline = &pipeline;
    m_tracks = &tracks;
    m_input = &input;
    m_videoSourceId = videoSourceId;
    m_audioSourceId = audioSourceId;
    m_nextVideoSample = 0;
    m_nextAudioSample = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.clear();
        m_stop = false;
        m_active = true;
    }
    m_thread = std::thread(&MediaFeeder::run, this);
    m_logger->debug("Feeder thread started");
}

void MediaFeeder::stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_active)
        {
            return;
        }
        m_stop = true;
        m_active = false;
        m_queue.clear();
    }
    m_cv.notify_all();
    if (m_thread.joinable())
    {
        m_thread.join();
        m_logger->debug("Feeder thread stopped");
    }
}

void MediaFeeder::enqueue(int32_t sourceId, size_t frameCount, uint32_t requestId)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_active)
        {
            return;
        }
        m_queue.push_back(Request{sourceId, frameCount, requestId});
    }
    m_cv.notify_one();
}

void MediaFeeder::cancel(int32_t sourceId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.erase(std::remove_if(m_queue.begin(), m_queue.end(),
                                 [sourceId](const Request &r) { return r.sourceId == sourceId; }),
                  m_queue.end());
}

void MediaFeeder::run()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (true)
    {
        m_cv.wait(lock, [this] { return m_stop || !m_queue.empty(); });
        if (m_stop)
        {
            return;
        }
        const Request request = m_queue.front();
        m_queue.pop_front();
        lock.unlock();
        feedRequest(request);
        lock.lock();
    }
}

void MediaFeeder::feedRequest(const Request &request)
{
    const bool isAudio = m_tracks->hasAudio && request.sourceId == m_audioSourceId;
    const demux::Track &track = isAudio ? m_tracks->audio : m_tracks->video;
    size_t &nextSample = isAudio ? m_nextAudioSample : m_nextVideoSample;
    const char *kind = isAudio ? "audio" : "video";

    // Each frame's buffer must stay alive until haveData(). Anyone who reuses a single
    // buffer here breaks addSegment(), which keeps the pointer.
    std::vector<std::vector<uint8_t>> buffers;
    MediaSourceStatus status = MediaSourceStatus::OK;
    size_t sent = 0;

    while (sent < request.frameCount && nextSample < track.samples.size())
    {
        const demux::Sample &sample = track.samples[nextSample];
        std::vector<uint8_t> frame(sample.size);
        m_input->clear();
        m_input->seekg(static_cast<std::streamoff>(sample.offset));
        if (!m_input->read(reinterpret_cast<char *>(frame.data()), static_cast<std::streamsize>(sample.size)))
        {
            m_logger->error(std::string("Failed to read ") + kind + " sample " + std::to_string(nextSample));
            status = MediaSourceStatus::ERROR;
            break;
        }
        buffers.push_back(std::move(frame));

        std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSegment> segment;
        if (isAudio)
        {
            auto audio = std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentAudio>(
                request.sourceId, sample.ptsNs, sample.durationNs, track.sampleRate, track.channels);
            audio->setData(sample.size, buffers.back().data());
            audio->setSegmentAlignment(firebolt::rialto::SegmentAlignment::AU);
            segment = std::move(audio);
        }
        else
        {
            auto video = std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentVideo>(
                request.sourceId, sample.ptsNs, sample.durationNs, track.width, track.height);
            video->setData(sample.size, buffers.back().data());
            video->setSegmentAlignment(firebolt::rialto::SegmentAlignment::AU);
            segment = std::move(video);
        }

        const AddSegmentStatus result = m_pipeline->addSegment(request.requestId, segment);
        if (result == AddSegmentStatus::NO_SPACE)
        {
            // Keep the sample for the next request. Only an unfit first sample is an error.
            buffers.pop_back();
            if (sent == 0)
            {
                m_logger->error(std::string(kind) + " sample " + std::to_string(nextSample) +
                                " does not fit in the shared buffer");
                status = MediaSourceStatus::ERROR;
            }
            break;
        }
        if (result != AddSegmentStatus::OK)
        {
            m_logger->error(std::string("addSegment() failed for ") + kind + " sample " +
                            std::to_string(nextSample));
            status = MediaSourceStatus::ERROR;
            break;
        }
        ++nextSample;
        ++sent;
    }

    if (status == MediaSourceStatus::OK && nextSample >= track.samples.size())
    {
        status = MediaSourceStatus::EOS;
    }
    if (!m_pipeline->haveData(status, request.requestId))
    {
        m_logger->warning("haveData() returned false for request " + std::to_string(request.requestId));
    }
}

} // namespace rialto_refplayer
