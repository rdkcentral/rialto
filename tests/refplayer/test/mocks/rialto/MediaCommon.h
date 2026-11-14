#pragma once

#include <cstdint>
#include <optional>

namespace firebolt::rialto
{

enum class MetricType : uint32_t
{
    VIDEO_FRAME_DROP = 1,
    AUDIO_FRAME_DROP = 2,
    VIDEO_DECODE_ERROR = 3,
    AUDIO_DECODE_ERROR = 4,
    VIDEO_FRAME_REPEAT = 5,
    AUDIO_GAP = 6,
    BUFFER_UNDERFLOW = 7
};

enum class MetricPhase : uint32_t
{
    OCCURRENCE = 1,
    EPISODE_STARTED = 2,
    EPISODE_RESOLVED = 3
};

struct MetricEvent
{
    MetricType type{MetricType::VIDEO_FRAME_DROP};
    MetricPhase phase{MetricPhase::OCCURRENCE};
    int32_t sourceId{-1};
    std::optional<int64_t> presentationTimeStampNs;
    std::optional<uint64_t> count;
    std::optional<uint64_t> durationNs;
    uint64_t observedAtMonotonicNs{0};
    int64_t observedAtUnixNs{0};
};

} // namespace firebolt::rialto