#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

namespace reference_player::mp4_demuxer
{
struct Sample
{
    uint64_t offset;
    uint32_t size;
    int64_t ptsNs;
    int64_t durationNs;
};

enum class Codec
{
    Unknown,
    H264,
    H265Hvc1,
    H265Hev1,
    Vp9,
    Av1,
    Aac,
    Opus,
    Eac3
};

struct VideoTrack
{
    std::vector<Sample> samples;
    std::vector<uint8_t> codecData;
    Codec codec{Codec::H264};
    int32_t width{0};
    int32_t height{0};
};

struct AudioTrack
{
    std::vector<Sample> samples;
    std::vector<uint8_t> codecData;
    Codec codec{Codec::Aac};
    int32_t sampleRate{0};
    int32_t channels{0};
};

struct Tracks
{
    VideoTrack video;
    AudioTrack audio;
    bool hasAudio{false};
};

bool isLocalMp4(const std::string &url);
bool parse(std::istream &file, Tracks &tracks, std::string &error);
} // namespace reference_player::mp4_demuxer