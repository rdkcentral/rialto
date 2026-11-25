#include "SourceManager.h"

#include "IMediaPipeline.h"

#include <fstream>

namespace rialto_refplayer
{
namespace rialto = firebolt::rialto;

namespace
{

// How a codec is described to the Rialto server. Unknown maps to the legacy H.264 and AAC settings.
struct VideoProfile
{
    const char *name;
    const char *mime;
    rialto::StreamFormat format;
    rialto::SegmentAlignment alignment;
    const char *configName;
};

struct AudioProfile
{
    const char *name;
    const char *mime;
    rialto::StreamFormat format;
    bool codecDataInCaps; // AAC only: its AudioSpecificConfig goes on the caps as codec_data
};

VideoProfile videoProfile(demux::Codec codec)
{
    switch (codec)
    {
    case demux::Codec::H265Hvc1:
        return {"H.265 (hvc1)", "video/h265", rialto::StreamFormat::HVC1, rialto::SegmentAlignment::AU, "hvcC"};
    case demux::Codec::H265Hev1:
        return {"H.265 (hev1)", "video/h265", rialto::StreamFormat::HEV1, rialto::SegmentAlignment::AU, "hvcC"};
    // VP9 and AV1 caps carry no alignment or stream-format field.
    case demux::Codec::Vp9:
        return {"VP9", "video/x-vp9", rialto::StreamFormat::UNDEFINED, rialto::SegmentAlignment::UNDEFINED, "vpcC"};
    case demux::Codec::Av1:
        return {"AV1", "video/x-av1", rialto::StreamFormat::UNDEFINED, rialto::SegmentAlignment::UNDEFINED, "av1C"};
    default:
        return {"H.264", "video/h264", rialto::StreamFormat::AVC, rialto::SegmentAlignment::AU, "avcC"};
    }
}

AudioProfile audioProfile(demux::Codec codec)
{
    switch (codec)
    {
    case demux::Codec::Opus:
        return {"Opus", "audio/x-opus", rialto::StreamFormat::UNDEFINED, false};
    case demux::Codec::Eac3:
        return {"E-AC-3", "audio/x-eac3", rialto::StreamFormat::UNDEFINED, false};
    default:
        return {"AAC", "audio/mp4", rialto::StreamFormat::RAW, true};
    }
}

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

    const VideoProfile video = videoProfile(model.video.codec);
    m_logger->info("Loaded " + url + ": " + video.name + " " + std::to_string(model.video.width) + "x" +
                   std::to_string(model.video.height) + ", " + std::to_string(model.video.samples.size()) +
                   " samples, " + video.configName + " " + std::to_string(model.video.codecData.size()) + " bytes");
    if (model.hasAudio)
    {
        m_logger->info(std::string("Loaded ") + audioProfile(model.audio.codec).name +
                       " audio: " + std::to_string(model.audio.sampleRate) + " Hz, " +
                       std::to_string(model.audio.channels) + " channels, " +
                       std::to_string(model.audio.samples.size()) + " samples");
    }
    else
    {
        m_logger->info("No supported audio track (AAC, Opus or E-AC-3) found; continuing video-only");
    }

    m_tracks = std::move(model);
    m_input = std::move(input);
    return true;
}

bool SourceManager::attach(firebolt::rialto::IMediaPipeline &pipeline)
{
    const VideoProfile videoKind = videoProfile(m_tracks.video.codec);
    std::shared_ptr<rialto::CodecData> codecData;
    if (!m_tracks.video.codecData.empty())
    {
        codecData = std::make_shared<rialto::CodecData>();
        codecData->data = m_tracks.video.codecData;
        codecData->type = rialto::CodecDataType::BUFFER;
    }
    std::unique_ptr<rialto::IMediaPipeline::MediaSource> video =
        std::make_unique<rialto::IMediaPipeline::MediaSourceVideo>(
            videoKind.mime, false, m_tracks.video.width, m_tracks.video.height, videoKind.alignment,
            videoKind.format, codecData);
    if (!pipeline.attachSource(video))
    {
        m_logger->error("IMediaPipeline::attachSource(video) returned false");
        return false;
    }
    m_videoSourceId = video->getId();
    m_logger->info("Source attached with id " + std::to_string(m_videoSourceId));

    if (m_tracks.hasAudio)
    {
        const AudioProfile audioKind = audioProfile(m_tracks.audio.codec);
        rialto::AudioConfig audioConfig{};
        audioConfig.numberOfChannels = static_cast<uint32_t>(m_tracks.audio.channels);
        audioConfig.sampleRate = static_cast<uint32_t>(m_tracks.audio.sampleRate);
        // AAC keeps its AudioSpecificConfig here as before, and Opus reads its OpusHead from here.
        // E-AC-3 needs no codec data.
        if (m_tracks.audio.codec != demux::Codec::Eac3)
        {
            audioConfig.codecSpecificConfig = m_tracks.audio.codecData;
        }

        // The Rialto server only puts codec_data and stream-format on the audio caps from these
        // arguments, not from audioConfig. Raw MP4 AAC frames have no ADTS headers, so without
        // the AudioSpecificConfig and stream-format=raw the decoder cannot start.
        std::shared_ptr<rialto::CodecData> audioCodecData;
        if (audioKind.codecDataInCaps && !m_tracks.audio.codecData.empty())
        {
            audioCodecData = std::make_shared<rialto::CodecData>();
            audioCodecData->data = m_tracks.audio.codecData;
            audioCodecData->type = rialto::CodecDataType::BUFFER;
        }
        std::unique_ptr<rialto::IMediaPipeline::MediaSource> audio =
            std::make_unique<rialto::IMediaPipeline::MediaSourceAudio>(audioKind.mime, false, audioConfig,
                                                                       rialto::SegmentAlignment::AU,
                                                                       audioKind.format, audioCodecData);
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
