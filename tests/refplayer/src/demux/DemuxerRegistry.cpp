#include "IDemuxer.h"

#include "Mp4Demuxer.h"

namespace rialto_refplayer::demux
{
namespace
{

std::vector<Sample> toSamples(std::vector<reference_player::mp4_demuxer::Sample> &&parsed)
{
    std::vector<Sample> samples;
    samples.reserve(parsed.size());
    for (const auto &sample : parsed)
    {
        samples.push_back(Sample{sample.offset, sample.size, sample.ptsNs, sample.durationNs});
    }
    return samples;
}

Codec toCodec(reference_player::mp4_demuxer::Codec codec)
{
    using Parsed = reference_player::mp4_demuxer::Codec;
    switch (codec)
    {
    case Parsed::H264:
        return Codec::H264;
    case Parsed::H265Hvc1:
        return Codec::H265Hvc1;
    case Parsed::H265Hev1:
        return Codec::H265Hev1;
    case Parsed::Vp9:
        return Codec::Vp9;
    case Parsed::Av1:
        return Codec::Av1;
    case Parsed::Aac:
        return Codec::Aac;
    case Parsed::Opus:
        return Codec::Opus;
    case Parsed::Eac3:
        return Codec::Eac3;
    case Parsed::Unknown:
        break;
    }
    return Codec::Unknown;
}

// Adapts the MP4 parser's own track types to the library's track model.
class Mp4DemuxerAdapter final : public IDemuxer
{
public:
    bool parse(std::istream &input, TrackModel &model, std::string &error) override
    {
        reference_player::mp4_demuxer::Tracks tracks;
        if (!reference_player::mp4_demuxer::parse(input, tracks, error))
        {
            return false;
        }

        model.video.samples = toSamples(std::move(tracks.video.samples));
        model.video.codecData = std::move(tracks.video.codecData);
        model.video.codec = toCodec(tracks.video.codec);
        model.video.width = tracks.video.width;
        model.video.height = tracks.video.height;

        model.hasAudio = tracks.hasAudio;
        model.audio = Track{};
        if (tracks.hasAudio)
        {
            model.audio.samples = toSamples(std::move(tracks.audio.samples));
            model.audio.codecData = std::move(tracks.audio.codecData);
            model.audio.codec = toCodec(tracks.audio.codec);
            model.audio.sampleRate = tracks.audio.sampleRate;
            model.audio.channels = tracks.audio.channels;
        }
        return true;
    }
};

} // namespace

std::unique_ptr<IDemuxer> createDemuxer(const std::string &url, const std::string &mimeType)
{
    (void)mimeType;
    if (reference_player::mp4_demuxer::isLocalMp4(url))
    {
        return std::make_unique<Mp4DemuxerAdapter>();
    }
    return nullptr;
}

} // namespace rialto_refplayer::demux
