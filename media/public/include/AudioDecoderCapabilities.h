/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIREBOLT_RIALTO_AUDIO_DECODER_CAPABILITIES_H_
#define FIREBOLT_RIALTO_AUDIO_DECODER_CAPABILITIES_H_

/**
 * @file AudioDecoderCapabilities.h
 *
 * Audio decoder capabilities types
 *
 */

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace firebolt::rialto
{
/**
 * @brief Supported audio codecs
 */
enum class AudioCodec
{
    PCM,          /**< PCM audio codec */
    AAC,          /**< AAC audio codec */
    MPEG_AUDIO,   /**< MPEG audio codec */
    MP3,          /**< MP3 audio codec */
    ALAC,         /**< ALAC audio codec */
    SBC,          /**< SBC audio codec */
    DOLBY_AC3,    /**< Dolby AC3 audio codec */
    DOLBY_AC4,    /**< Dolby AC4 audio codec */
    DOLBY_EAC3,   /**< Dolby E-AC3 audio codec */
    DOLBY_TRUEHD, /**< Dolby TrueHD audio codec */
    FLAC,         /**< FLAC audio codec */
    VORBIS,       /**< Vorbis audio codec */
    OPUS,         /**< Opus audio codec */
    REALAUDIO,    /**< RealAudio codec */
    USAC,         /**< USAC audio codec */
    DTS,          /**< DTS audio codec */
    AVS           /**< AVS audio codec */
};

/**
 * @brief AAC profile types
 */
enum class AacProfile
{
    LC,    /**< Low Complexity profile */
    HE_V1, /**< High Efficiency v1 profile */
    HE_V2, /**< High Efficiency v2 profile */
    ELD,   /**< Enhanced Low Delay profile */
    X_HE   /**< Extended High Efficiency profile */
};

/**
 * @brief Dolby AC3 profile types
 */
enum class DolbyAc3Profile
{
    STANDARD /**< Standard Dolby AC3 profile */
};

/**
 * @brief RealAudio profile types
 */
enum class RealAudioProfile
{
    RA8, /**< RealAudio 8 profile */
    RA10 /**< RealAudio 10 profile */
};

/**
 * @brief USAC (Unified Speech and Audio Coding) profile types
 */
enum class UsacProfile
{
    BASELINE,       /**< USAC Baseline profile */
    EXTENDED_HE_AAC /**< USAC Extended HE-AAC profile */
};

/**
 * @brief DTS profile types
 */
enum class DtsProfile
{
    CORE,   /**< DTS Core profile */
    HD_HRA, /**< DTS-HD High Resolution Audio profile */
    HD_MA   /**< DTS-HD Master Audio profile */
};

/**
 * @brief AVS audio profile types
 */
enum class AvsProfile
{
    AVS1_PART2, /**< AVS1 Part 2 profile */
    AVS2,       /**< AVS2 profile */
    AVS3        /**< AVS3 profile */
};

/**
 * @brief DolbyEac3 profile types (split from DolbyAc3)
 */
enum class DolbyEac3Profile
{
    PLUS,    /**< Dolby E-AC3 Plus profile */
    PLUS_JOC /**< Dolby E-AC3 Plus with JOC profile */
};

/**
 * @brief MPEG Audio profile types
 */
enum class MpegAudioProfile
{
    LAYER_1, /**< MPEG Audio Layer 1 */
    LAYER_2  /**< MPEG Audio Layer 2 */
};

/**
 * @brief Per-profile audio capability fields (HFP schema v1.0.0).
 *
 * Used as the value type in named-profile codec capability maps and as
 * the single `base` field in single-profile codec capability structs.
 * All fields are required (not optional) per HFP schema v1.0.0.
 */
struct AudioProfileCapability
{
    uint64_t maxBitrateInBps;   /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;       /**< Maximum number of channels */
    uint32_t maxSampleRateInHz; /**< Maximum sample rate in Hz */
    uint32_t maxBitDepth;       /**< Maximum bit depth */
};

inline bool operator==(const AudioProfileCapability &lhs, const AudioProfileCapability &rhs)
{
    return lhs.maxBitrateInBps == rhs.maxBitrateInBps && lhs.maxChannels == rhs.maxChannels &&
           lhs.maxSampleRateInHz == rhs.maxSampleRateInHz && lhs.maxBitDepth == rhs.maxBitDepth;
}

/**
 * @brief PCM codec capabilities
 */
struct PcmCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

inline bool operator==(const PcmCapability &lhs, const PcmCapability &rhs)
{
    return lhs.base == rhs.base;
}

/**
 * @brief AAC codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct AacCapability
{
    std::map<AacProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

inline bool operator==(const AacCapability &lhs, const AacCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

/**
 * @brief MPEG Audio codec capabilities (LAYER_1, LAYER_2 per HFP schema v1.0.0)
 */
struct MpegAudioCapability
{
    std::map<MpegAudioProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

inline bool operator==(const MpegAudioCapability &lhs, const MpegAudioCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

/**
 * @brief MP3 codec capabilities
 */
struct Mp3Capability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

inline bool operator==(const Mp3Capability &lhs, const Mp3Capability &rhs)
{
    return lhs.base == rhs.base;
}

/**
 * @brief ALAC codec capabilities
 */
struct AlacCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

inline bool operator==(const AlacCapability &lhs, const AlacCapability &rhs)
{
    return lhs.base == rhs.base;
}

/**
 * @brief SBC codec capabilities
 */
struct SbcCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

inline bool operator==(const SbcCapability &lhs, const SbcCapability &rhs)
{
    return lhs.base == rhs.base;
}

/**
 * @brief Dolby AC3 codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct DolbyAc3Capability
{
    std::map<DolbyAc3Profile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

inline bool operator==(const DolbyAc3Capability &lhs, const DolbyAc3Capability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

/**
 * @brief Dolby AC4 codec capabilities
 */
struct DolbyAc4Capability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

inline bool operator==(const DolbyAc4Capability &lhs, const DolbyAc4Capability &rhs)
{
    return lhs.base == rhs.base;
}

/**
 * @brief Dolby TrueHD codec capabilities
 */
struct DolbyTruehdCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

inline bool operator==(const DolbyTruehdCapability &lhs, const DolbyTruehdCapability &rhs)
{
    return lhs.base == rhs.base;
}

/**
 * @brief FLAC codec capabilities
 */
struct FlacCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

inline bool operator==(const FlacCapability &lhs, const FlacCapability &rhs)
{
    return lhs.base == rhs.base;
}

/**
 * @brief Vorbis codec capabilities
 */
struct VorbisCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

inline bool operator==(const VorbisCapability &lhs, const VorbisCapability &rhs)
{
    return lhs.base == rhs.base;
}

/**
 * @brief Opus codec capabilities
 */
struct OpusCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

inline bool operator==(const OpusCapability &lhs, const OpusCapability &rhs)
{
    return lhs.base == rhs.base;
}

/**
 * @brief RealAudio codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct RealAudioCapability
{
    std::map<RealAudioProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

inline bool operator==(const RealAudioCapability &lhs, const RealAudioCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

/**
 * @brief USAC codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct UsacCapability
{
    std::map<UsacProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

inline bool operator==(const UsacCapability &lhs, const UsacCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

/**
 * @brief DTS codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct DtsCapability
{
    std::map<DtsProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

inline bool operator==(const DtsCapability &lhs, const DtsCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

/**
 * @brief AVS audio codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct AvsCapability
{
    std::map<AvsProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

inline bool operator==(const AvsCapability &lhs, const AvsCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

/**
 * @brief Audio decoder capability entry for a specific rank
 */
/**
 * @brief DolbyEac3 codec capabilities (split from DolbyAc3, HFP schema v1.0.0)
 */
struct DolbyEac3Capability
{
    std::map<DolbyEac3Profile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

inline bool operator==(const DolbyEac3Capability &lhs, const DolbyEac3Capability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

struct AudioDecoderCapability
{
    std::optional<PcmCapability> pcm;             /**< PCM capabilities (if supported) */
    std::optional<AacCapability> aac;             /**< AAC capabilities (if supported) */
    std::optional<MpegAudioCapability> mpegAudio; /**< MPEG Audio capabilities (if supported) */
    std::optional<Mp3Capability> mp3;             /**< MP3 capabilities (if supported) */
    std::optional<AlacCapability> alac;           /**< ALAC capabilities (if supported) */
    std::optional<SbcCapability> sbc;             /**< SBC capabilities (if supported) */
    std::optional<DolbyAc3Capability> dolbyAc3;   /**< Dolby AC3 capabilities (if supported) */
    std::optional<DolbyAc4Capability> dolbyAc4;   /**< Dolby AC4 capabilities (if supported) */
    std::optional<DolbyEac3Capability> dolbyEac3; /**< Dolby EAC3 capabilities (if supported) */

    std::optional<DolbyTruehdCapability> dolbyTruehd; /**< Dolby TrueHD capabilities (if supported) */
    std::optional<FlacCapability> flac;               /**< FLAC capabilities (if supported) */
    std::optional<VorbisCapability> vorbis;           /**< Vorbis capabilities (if supported) */
    std::optional<OpusCapability> opus;               /**< Opus capabilities (if supported) */

    std::optional<RealAudioCapability> realAudio; /**< RealAudio capabilities (if supported) */
    std::optional<UsacCapability> usac;           /**< USAC capabilities (if supported) */
    std::optional<DtsCapability> dts;             /**< DTS capabilities (if supported) */
    std::optional<AvsCapability> avs;             /**< AVS capabilities (if supported) */
};

inline bool operator==(const AudioDecoderCapability &lhs, const AudioDecoderCapability &rhs)
{
    return lhs.pcm == rhs.pcm && lhs.aac == rhs.aac && lhs.mpegAudio == rhs.mpegAudio && lhs.mp3 == rhs.mp3 &&
           lhs.alac == rhs.alac && lhs.sbc == rhs.sbc && lhs.dolbyAc3 == rhs.dolbyAc3 && lhs.dolbyAc4 == rhs.dolbyAc4 &&
           lhs.dolbyEac3 == rhs.dolbyEac3 && lhs.dolbyTruehd == rhs.dolbyTruehd && lhs.flac == rhs.flac &&
           lhs.vorbis == rhs.vorbis && lhs.opus == rhs.opus && lhs.realAudio == rhs.realAudio && lhs.usac == rhs.usac &&
           lhs.dts == rhs.dts && lhs.avs == rhs.avs;
}

/**
 * @brief Audio decoder capabilities container
 */
struct AudioDecoderCapabilities
{
    std::string interfaceVersion;                     /**< Interface version string */
    std::string schemaVersion;                        /**< Schema version (e.g., "0.1.0") */
    std::vector<AudioDecoderCapability> capabilities; /**< List of decoder capabilities */
};

inline bool operator==(const AudioDecoderCapabilities &lhs, const AudioDecoderCapabilities &rhs)
{
    return lhs.interfaceVersion == rhs.interfaceVersion && lhs.schemaVersion == rhs.schemaVersion &&
           lhs.capabilities == rhs.capabilities;
}

} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_AUDIO_DECODER_CAPABILITIES_H_
