#include "SourceManager.h"

#include "IMediaPipeline.h"

#include <fstream>

namespace rialto_refplayer
{
namespace
{

constexpr const char *kVideoMime = "video/mp4";
constexpr const char *kAudioMime = "audio/mp4";

} // namespace

std::unique_ptr<std::istream> openLocalFile(const std::string &url)
{
    std::string path = url;
    constexpr const char *kScheme = "file://";
    if (path.rfind(kScheme, 0) == 0)
    {
        path.erase(0, std::string(kScheme).size());
    }
    auto file = std::make_unique<std::ifstream>(path, std::ios::binary);
    if (!*file)
    {
        return nullptr;
    }
    return file;
}

SourceManager::SourceManager(std::shared_ptr<Logger> logger, Dependencies dependencies)
    : m_logger(std::move(logger)), m_dependencies(std::move(dependencies))
{
}

SourceManager::~SourceManager()
{
    m_input.reset();
}

bool SourceManager::open(const std::string &url, const std::string &mimeType, std::string &error)
{
    removeAll(nullptr);

    auto demuxer = m_dependencies.createDemuxer(url, mimeType);
    if (!demuxer)
    {
        error = "no demuxer accepts " + url;
        return false;
    }

    auto input = m_dependencies.openStream(url);
    if (!input)
    {
        error = "cannot open " + url;
        return false;
    }

    demux::TrackModel model;
    if (!demuxer->parse(*input, model, error))
    {
        return false;
    }

    m_logger->info("Loaded " + url + ": H.264 " + std::to_string(model.video.width) + "x" +
                   std::to_string(model.video.height) + ", " + std::to_string(model.video.samples.size()) +
                   " samples, avcC " + std::to_string(model.video.codecData.size()) + " bytes");
    if (model.hasAudio)
    {
        m_logger->info("Loaded AAC audio: " + std::to_string(model.audio.sampleRate) + " Hz, " +
                       std::to_string(model.audio.channels) + " channels, " +
                       std::to_string(model.audio.samples.size()) + " samples");
    }
    else
    {
        m_logger->info("No supported AAC (mp4a) audio track found; continuing video-only");
    }

    m_tracks = std::move(model);
    m_input = std::move(input);
    return true;
}

bool SourceManager::attach(firebolt::rialto::IMediaPipeline &pipeline)
{
    namespace rialto = firebolt::rialto;

    std::shared_ptr<rialto::CodecData> codecData;
    if (!m_tracks.video.codecData.empty())
    {
        codecData = std::make_shared<rialto::CodecData>();
        codecData->data = m_tracks.video.codecData;
        codecData->type = rialto::CodecDataType::BUFFER;
    }
    std::unique_ptr<rialto::IMediaPipeline::MediaSource> video =
        std::make_unique<rialto::IMediaPipeline::MediaSourceVideo>(
            kVideoMime, false, m_tracks.video.width, m_tracks.video.height, rialto::SegmentAlignment::AU,
            rialto::StreamFormat::AVC, codecData);
    if (!pipeline.attachSource(video))
    {
        m_logger->error("IMediaPipeline::attachSource(video) returned false");
        return false;
    }
    m_videoSourceId = video->getId();
    m_logger->info("Source attached with id " + std::to_string(m_videoSourceId));

    if (m_tracks.hasAudio)
    {
        rialto::AudioConfig audioConfig{};
        audioConfig.numberOfChannels = static_cast<uint32_t>(m_tracks.audio.channels);
        audioConfig.sampleRate = static_cast<uint32_t>(m_tracks.audio.sampleRate);
        audioConfig.codecSpecificConfig = m_tracks.audio.codecData;

        std::unique_ptr<rialto::IMediaPipeline::MediaSource> audio =
            std::make_unique<rialto::IMediaPipeline::MediaSourceAudio>(kAudioMime, false, audioConfig,
                                                                       rialto::SegmentAlignment::AU);
        if (!pipeline.attachSource(audio))
        {
            m_logger->error("IMediaPipeline::attachSource(audio) returned false");
            return false;
        }
        m_audioSourceId = audio->getId();
        m_logger->info("Audio source attached with id " + std::to_string(m_audioSourceId));
    }
    return true;
}

void SourceManager::removeAll(firebolt::rialto::IMediaPipeline *pipeline)
{
    if (pipeline)
    {
        if (m_videoSourceId != 0)
        {
            pipeline->removeSource(m_videoSourceId);
        }
        if (m_audioSourceId != 0)
        {
            pipeline->removeSource(m_audioSourceId);
        }
    }
    m_videoSourceId = 0;
    m_audioSourceId = 0;
    m_tracks = demux::TrackModel{};
    m_input.reset();
}

} // namespace rialto_refplayer
