#pragma once

#include <atomic>
#include <cstdint>

namespace rdkai
{
namespace tts_shm
{

// Must remain binary-compatible with the EdgeAI TtsShmProtocol.h.
constexpr uint32_t kMagic = 0x54545331; // "TTS1"
constexpr uint32_t kVersion = 1;

constexpr uint32_t kSlotCount = 32;
constexpr uint32_t kMaxSamplesPerSlot = 16384;

enum class SampleFormat : uint16_t
{
    S16LE = 1
};

struct Slot
{
    std::atomic<uint32_t> ready{0};

    uint64_t generation{0};
    uint64_t sequence{0};

    int64_t ptsUs{0};

    uint32_t sampleRate{0};
    uint32_t sampleCount{0};

    uint16_t channels{1};
    SampleFormat format{SampleFormat::S16LE};

    int16_t pcm[kMaxSamplesPerSlot];
};

struct Ring
{
    uint32_t magic{kMagic};
    uint32_t version{kVersion};

    std::atomic<uint32_t> writeIndex{0};
    std::atomic<uint32_t> readIndex{0};

    std::atomic<uint64_t> generation{0};

    Slot slots[kSlotCount];
};

} // namespace tts_shm
} // namespace rdkai
