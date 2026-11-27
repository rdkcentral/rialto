#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace rialto_refplayer::demux
{

struct Sample
{
    uint64_t offset{0};
    uint32_t size{0};
    int64_t ptsNs{0};
    int64_t durationNs{0};
};

// Codec of a track. Unknown means "the legacy default for the slot": H.264 for video, AAC for audio.
enum class Codec
{
    Unknown,
    H264,
    H265Hvc1, // parameter sets only in the hvcC record
    H265Hev1, // parameter sets may also appear in the stream
    Vp9,
    Av1,
    Aac,
    Opus,
    Eac3,
    Mp3,
    Flac,
    Pcm
};

// Sample layout of an uncompressed PCM audio track. Samples are always interleaved.
enum class PcmFormat
{
    None,
    S8,
    S16LE,
    S16BE,
    S24LE,
    S24BE,
    S32LE,
    S32BE
};

struct Track
{
    std::vector<Sample> samples;
    // Video: avcC (H.264), hvcC (H.265), vpcC record without its FullBox header (VP9), av1C (AV1).
    // Audio: AudioSpecificConfig (AAC), OpusHead (Opus), dec3 payload (E-AC-3). MP3, FLAC and PCM have none.
    std::vector<uint8_t> codecData;
    Codec codec{Codec::Unknown};
    // FLAC: "fLaC" marker followed by each metadata block with its 4-byte header, as GStreamer expects.
    std::vector<std::vector<uint8_t>> streamHeader;
    PcmFormat pcmFormat{PcmFormat::None}; // PCM only
    int32_t width{0};               // video
    int32_t height{0};              // video
    int32_t sampleRate{0};          // audio
    int32_t channels{0};            // audio

    // End of the last sample, which is the track's length when the container has no other figure.
    int64_t endNs() const
    {
        int64_t end = 0;
        for (const Sample &sample : samples)
        {
            end = std::max(end, sample.ptsNs + sample.durationNs);
        }
        return end;
    }
};

// What a demuxer produced. Sources are built from this, never from fixed sizes.
struct TrackModel
{
    Track video;
    Track audio;
    bool hasAudio{false};

    // The longer of the two tracks. 0 when there are no samples.
    int64_t durationNs() const { return std::max(video.endNs(), hasAudio ? audio.endNs() : 0); }
};

} // namespace rialto_refplayer::demux
