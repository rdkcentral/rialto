#pragma once

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
    Eac3
};

struct Track
{
    std::vector<Sample> samples;
    // Video: avcC (H.264), hvcC (H.265), vpcC record without its FullBox header (VP9), av1C (AV1).
    // Audio: AudioSpecificConfig (AAC), OpusHead (Opus), dec3 payload (E-AC-3).
    std::vector<uint8_t> codecData;
    Codec codec{Codec::Unknown};
    int32_t width{0};               // video
    int32_t height{0};              // video
    int32_t sampleRate{0};          // audio
    int32_t channels{0};            // audio
};

// What a demuxer produced. Sources are built from this, never from fixed sizes.
struct TrackModel
{
    Track video;
    Track audio;
    bool hasAudio{false};
};

} // namespace rialto_refplayer::demux
