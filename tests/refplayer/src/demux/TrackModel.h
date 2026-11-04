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

struct Track
{
    std::vector<Sample> samples;
    std::vector<uint8_t> codecData; // avcC for video, AudioSpecificConfig for audio
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
