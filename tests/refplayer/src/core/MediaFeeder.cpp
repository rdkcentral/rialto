#include "MediaFeeder.h"

#include <algorithm>
#include <chrono>
#include <vector>

namespace rialto_refplayer
{

using firebolt::rialto::AddSegmentStatus;
using firebolt::rialto::MediaSourceStatus;
using rialto_refplayer::FeederFaultStimulus;

namespace
{
constexpr auto kLateFramePositionWait = std::chrono::seconds{2};
constexpr auto kPlaybackActiveWait = std::chrono::seconds{2};
constexpr auto kUnderflowStall = std::chrono::seconds{2};

bool corruptFirstVideoSlice(std::vector<uint8_t> &accessUnit, const std::vector<uint8_t> &codecData)
{
    if (codecData.size() < 5 || codecData[0] != 1)
    {
        return false;
    }

    const size_t nalLengthSize = static_cast<size_t>(codecData[4] & 0x03) + 1;
    size_t offset = 0;
    while (offset < accessUnit.size())
    {
        if (accessUnit.size() - offset < nalLengthSize)
        {
            return false;
        }

        uint32_t nalLength = 0;
        for (size_t byte = 0; byte < nalLengthSize; ++byte)
        {
            nalLength = (nalLength << 8) | accessUnit[offset++];
        }
        if (nalLength == 0 || nalLength > accessUnit.size() - offset)
        {
            return false;
        }

        const uint8_t nalType = accessUnit[offset] & 0x1f;
        if (nalType >= 1 && nalType <= 5)
        {
            if ((accessUnit[offset] & 0x80) != 0)
            {
                return false;
            }
            accessUnit[offset] |= 0x80;
            return true;
        }
        offset += nalLength;
    }
    return false;
}
} // namespace

MediaFeeder::MediaFeeder(std::shared_ptr<Logger> logger) : m_logger(std::move(logger))
{
}

MediaFeeder::~MediaFeeder()
{
    stop();
}

void MediaFeeder::start(firebolt::rialto::IMediaPipeline &pipeline, const demux::TrackModel &tracks,
                        std::istream &input, int32_t videoSourceId, int32_t audioSourceId,
                        std::optional<FeederFaultControl> faultControl)
{
    stop();

    m_pipeline = &pipeline;
    m_tracks = &tracks;
    m_input = &input;
    m_videoSourceId = videoSourceId;
    m_audioSourceId = audioSourceId;
    m_nextVideoSample = 0;
    m_nextAudioSample = 0;
    m_faultControl = std::move(faultControl);
    m_playbackPositionNs = 0;
    m_lateFrameInjected = false;
    m_decodeErrorInjected = false;
    m_frameRepeatInjected = false;
    m_underflowInjected = false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.clear();
        m_playbackActive = false;
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

void MediaFeeder::updatePlaybackPositionNs(int64_t positionNs)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playbackPositionNs = positionNs;
    }
    m_cv.notify_all();
}

void MediaFeeder::updatePlaybackActive(bool active)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playbackActive = active;
    }
    m_cv.notify_all();
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
        if (!isAudio && !waitForLateFrameDeadline(sample, nextSample))
        {
            status = MediaSourceStatus::ERROR;
            break;
        }
        if (!isAudio && !waitForFrameRepeatDeadline(sample, nextSample))
        {
            status = MediaSourceStatus::ERROR;
            break;
        }
        if (!isAudio && !stallForUnderflow(nextSample))
        {
            status = MediaSourceStatus::ERROR;
            break;
        }
        std::vector<uint8_t> frame(sample.size);
        m_input->clear();
        m_input->seekg(static_cast<std::streamoff>(sample.offset));
        if (!m_input->read(reinterpret_cast<char *>(frame.data()), static_cast<std::streamsize>(sample.size)))
        {
            m_logger->error(std::string("Failed to read ") + kind + " sample " + std::to_string(nextSample));
            status = MediaSourceStatus::ERROR;
            break;
        }
        const bool corruptSample = !isAudio && !m_decodeErrorInjected && m_faultControl &&
                                   m_faultControl->stimulus == FeederFaultStimulus::VideoDecodeError &&
                                   nextSample == m_faultControl->targetVideoSampleIndex;
        if (corruptSample && !corruptFirstVideoSlice(frame, track.codecData))
        {
            m_logger->error("Cannot inject decode error into video sample " + std::to_string(nextSample) +
                            ": no valid length-prefixed H.264 slice was found");
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
        if (corruptSample)
        {
            m_decodeErrorInjected = true;
            m_logger->info("Decode-error stimulus: corrupted video access unit " + std::to_string(nextSample - 1));
        }
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

bool MediaFeeder::waitForLateFrameDeadline(const demux::Sample &sample, size_t sampleIndex)
{
    if (m_lateFrameInjected || !m_faultControl ||
        m_faultControl->stimulus != FeederFaultStimulus::VideoFrameDrop ||
        sampleIndex != m_faultControl->targetVideoSampleIndex)
    {
        return true;
    }

    const int64_t deadlineNs = sample.ptsNs + std::max<int64_t>(sample.durationNs, 0);
    std::unique_lock<std::mutex> lock(m_mutex);
    const bool deadlinePassed = m_cv.wait_for(lock, kLateFramePositionWait, [this, deadlineNs] {
        return m_stop || m_playbackPositionNs.load() > deadlineNs;
    });
    if (m_stop)
    {
        return false;
    }

    m_lateFrameInjected = true;
    if (deadlinePassed)
    {
        m_logger->info("Late-frame stimulus: sending video sample " + std::to_string(sampleIndex) +
                       " after its presentation deadline");
    }
    else
    {
        m_logger->warning("Late-frame stimulus timed out waiting for playback position; sending video sample " +
                          std::to_string(sampleIndex) + " unchanged");
    }
    return true;
}

bool MediaFeeder::waitForFrameRepeatDeadline(const demux::Sample &sample, size_t sampleIndex)
{
    if (m_frameRepeatInjected || !m_faultControl ||
        m_faultControl->stimulus != FeederFaultStimulus::VideoFrameRepeat ||
        sampleIndex != m_faultControl->targetVideoSampleIndex)
    {
        return true;
    }
    if (sampleIndex == 0)
    {
        m_logger->warning("Frame-repeat stimulus needs a preceding video sample; sending sample 0 unchanged");
        m_frameRepeatInjected = true;
        return true;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    const bool repeatWindowReached = m_cv.wait_for(lock, kLateFramePositionWait, [this, &sample] {
        return m_stop || (m_playbackActive && m_playbackPositionNs.load() >= sample.ptsNs);
    });
    if (m_stop)
    {
        return false;
    }

    m_frameRepeatInjected = true;
    if (repeatWindowReached)
    {
        m_logger->info("Frame-repeat stimulus: sending video sample " + std::to_string(sampleIndex) +
                       " after its presentation time");
    }
    else
    {
        m_logger->warning("Frame-repeat stimulus timed out waiting for active playback; sending video sample " +
                          std::to_string(sampleIndex) + " unchanged");
    }
    return true;
}

bool MediaFeeder::stallForUnderflow(size_t sampleIndex)
{
    if (m_underflowInjected || !m_faultControl ||
        m_faultControl->stimulus != FeederFaultStimulus::BufferUnderflow ||
        sampleIndex != m_faultControl->targetVideoSampleIndex)
    {
        return true;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    const bool playbackStarted = m_cv.wait_for(lock, kPlaybackActiveWait, [this] {
        return m_stop || m_playbackActive;
    });
    if (m_stop)
    {
        return false;
    }
    if (!playbackStarted)
    {
        m_underflowInjected = true;
        m_logger->warning("Underflow stimulus timed out waiting for active playback; sending video sample " +
                          std::to_string(sampleIndex) + " unchanged");
        return true;
    }

    const bool stopped = m_cv.wait_for(lock, kUnderflowStall, [this] { return m_stop; });
    if (stopped)
    {
        return false;
    }
    m_underflowInjected = true;
    m_logger->info("Underflow stimulus: paused video delivery for " +
                   std::to_string(kUnderflowStall.count()) + " seconds during playback");
    return true;
}

} // namespace rialto_refplayer
