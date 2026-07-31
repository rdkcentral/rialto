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

#ifndef FIREBOLT_RIALTO_VIDEO_DECODER_CAPABILITIES_H_
#define FIREBOLT_RIALTO_VIDEO_DECODER_CAPABILITIES_H_

/**
 * @file VideoDecoderCapabilities.h
 *
 * Video decoder capabilities types
 *
 */

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace firebolt::rialto
{
/**
 * @brief Dynamic range types
 */
enum class DynamicRange
{
    SDR,         /**< Standard Dynamic Range */
    HLG,         /**< Hybrid Log-Gamma */
    HDR10,       /**< HDR10 high dynamic range */
    HDR10PLUS,   /**< HDR10+ high dynamic range */
    DOLBY_VISION /**< Dolby Vision */
};

/**
 * @brief MPEG2 video profile types
 */
enum class Mpeg2ProfileType
{
    MPEG2_MAIN,  /**< MPEG2 Main profile */
    MPEG2_SIMPLE /**< MPEG2 Simple profile */
};

/**
 * @brief MPEG2 video level types
 */
enum class Mpeg2Level
{
    MPEG2_LEVEL_LOW,  /**< MPEG2 Low level */
    MPEG2_LEVEL_MAIN, /**< MPEG2 Main level */
    MPEG2_LEVEL_HIGH  /**< MPEG2 High level */
};

/**
 * @brief H.264/AVC profile types
 */
enum class H264ProfileType
{
    H264_BASELINE, /**< H.264 Baseline profile */
    H264_MAIN,     /**< H.264 Main profile */
    H264_HIGH      /**< H.264 High profile */
};

/**
 * @brief H.264/AVC level types
 */
enum class H264Level
{
    H264_LEVEL_3,   /**< H.264 Level 3.0 */
    H264_LEVEL_3_1, /**< H.264 Level 3.1 */
    H264_LEVEL_4,   /**< H.264 Level 4.0 */
    H264_LEVEL_4_1, /**< H.264 Level 4.1 */
    H264_LEVEL_5,   /**< H.264 Level 5.0 */
    H264_LEVEL_5_1, /**< H.264 Level 5.1 */
    H264_LEVEL_5_2  /**< H.264 Level 5.2 */
};

/**
 * @brief H.265/HEVC profile types
 */
enum class H265ProfileType
{
    H265_MAIN,         /**< H.265 Main profile */
    H265_MAIN_10,      /**< H.265 Main 10 profile */
    H265_MAIN_10_HDR10 /**< H.265 Main 10 HDR10 profile */
};

/**
 * @brief H.265/HEVC level types
 */
enum class H265Level
{
    H265_LEVEL_4,   /**< H.265 Level 4.0 */
    H265_LEVEL_4_1, /**< H.265 Level 4.1 */
    H265_LEVEL_5,   /**< H.265 Level 5.0 */
    H265_LEVEL_5_1, /**< H.265 Level 5.1 */
    H265_LEVEL_5_2, /**< H.265 Level 5.2 */
    H265_LEVEL_6,   /**< H.265 Level 6.0 */
    H265_LEVEL_6_1, /**< H.265 Level 6.1 */
    H265_LEVEL_6_2  /**< H.265 Level 6.2 */
};

/**
 * @brief VP9 profile types
 */
enum class Vp9ProfileType
{
    VP9_PROFILE_0, /**< VP9 Profile 0 */
    VP9_PROFILE_1, /**< VP9 Profile 1 */
    VP9_PROFILE_2, /**< VP9 Profile 2 */
    VP9_PROFILE_3  /**< VP9 Profile 3 */
};

/**
 * @brief VP9 level types
 */
enum class Vp9Level
{
    VP9_LEVEL_1,   /**< VP9 Level 1 */
    VP9_LEVEL_1_1, /**< VP9 Level 1.1 */
    VP9_LEVEL_2,   /**< VP9 Level 2 */
    VP9_LEVEL_2_1, /**< VP9 Level 2.1 */
    VP9_LEVEL_3,   /**< VP9 Level 3 */
    VP9_LEVEL_3_1, /**< VP9 Level 3.1 */
    VP9_LEVEL_4,   /**< VP9 Level 4 */
    VP9_LEVEL_4_1, /**< VP9 Level 4.1 */
    VP9_LEVEL_5,   /**< VP9 Level 5 */
    VP9_LEVEL_5_1, /**< VP9 Level 5.1 */
    VP9_LEVEL_5_2, /**< VP9 Level 5.2 */
    VP9_LEVEL_6,   /**< VP9 Level 6 */
    VP9_LEVEL_6_1, /**< VP9 Level 6.1 */
    VP9_LEVEL_6_2  /**< VP9 Level 6.2 */
};

/**
 * @brief AV1 profile types
 */
enum class Av1ProfileType
{
    AV1_MAIN, /**< AV1 Main profile */
    AV1_HIGH  /**< AV1 High profile */
};

/**
 * @brief AV1 level types
 */
enum class Av1Level
{
    AV1_LEVEL_4_0, /**< AV1 Level 4.0 */
    AV1_LEVEL_4_1, /**< AV1 Level 4.1 */
    AV1_LEVEL_5_0, /**< AV1 Level 5.0 */
    AV1_LEVEL_5_1, /**< AV1 Level 5.1 */
    AV1_LEVEL_5_2, /**< AV1 Level 5.2 */
    AV1_LEVEL_6_0, /**< AV1 Level 6.0 */
    AV1_LEVEL_6_1, /**< AV1 Level 6.1 */
    AV1_LEVEL_6_2  /**< AV1 Level 6.2 */
};

/**
 * @brief Profile and level information for MPEG2
 */
struct Mpeg2Profile
{
    Mpeg2ProfileType type;    /**< Profile type */
    Mpeg2Level maxLevel;      /**< Maximum supported level */
    uint64_t maxBitrateInBps; /**< Maximum bitrate in bits per second */
};

/** @brief Equality operator for Mpeg2Profile. */
inline bool operator==(const Mpeg2Profile &lhs, const Mpeg2Profile &rhs)
{
    return lhs.type == rhs.type && lhs.maxLevel == rhs.maxLevel && lhs.maxBitrateInBps == rhs.maxBitrateInBps;
}

/**
 * @brief Profile and level information for H.264/AVC
 */
struct H264Profile
{
    H264ProfileType type;     /**< Profile type */
    H264Level maxLevel;       /**< Maximum supported level */
    uint64_t maxBitrateInBps; /**< Maximum bitrate in bits per second */
};

/** @brief Equality operator for H264Profile. */
inline bool operator==(const H264Profile &lhs, const H264Profile &rhs)
{
    return lhs.type == rhs.type && lhs.maxLevel == rhs.maxLevel && lhs.maxBitrateInBps == rhs.maxBitrateInBps;
}

/**
 * @brief Profile and level information for H.265/HEVC
 */
struct H265Profile
{
    H265ProfileType type;     /**< Profile type */
    H265Level maxLevel;       /**< Maximum supported level */
    uint64_t maxBitrateInBps; /**< Maximum bitrate in bits per second */
};

/** @brief Equality operator for H265Profile. */
inline bool operator==(const H265Profile &lhs, const H265Profile &rhs)
{
    return lhs.type == rhs.type && lhs.maxLevel == rhs.maxLevel && lhs.maxBitrateInBps == rhs.maxBitrateInBps;
}

/**
 * @brief Profile and level information for VP9
 */
struct Vp9Profile
{
    Vp9ProfileType type;      /**< Profile type */
    Vp9Level maxLevel;        /**< Maximum supported level */
    uint64_t maxBitrateInBps; /**< Maximum bitrate in bits per second */
};

/**
 * @brief Profile and level information for AV1
 */
struct Av1Profile
{
    Av1ProfileType type;      /**< Profile type */
    Av1Level maxLevel;        /**< Maximum supported level */
    uint64_t maxBitrateInBps; /**< Maximum bitrate in bits per second */
};

/** @brief Equality operator for Vp9Profile. */
inline bool operator==(const Vp9Profile &lhs, const Vp9Profile &rhs)
{
    return lhs.type == rhs.type && lhs.maxLevel == rhs.maxLevel && lhs.maxBitrateInBps == rhs.maxBitrateInBps;
}

/** @brief Equality operator for Av1Profile. */
inline bool operator==(const Av1Profile &lhs, const Av1Profile &rhs)
{
    return lhs.type == rhs.type && lhs.maxLevel == rhs.maxLevel && lhs.maxBitrateInBps == rhs.maxBitrateInBps;
}

/**
 * @brief Per-codec capability for MPEG-2: profiles and per-codec dynamic ranges.
 */
struct Mpeg2CodecCapability
{
    std::vector<Mpeg2Profile> profiles;      /**< Supported MPEG2 profiles */
    std::vector<DynamicRange> dynamicRanges; /**< Dynamic ranges supported by this codec */
};

/** @brief Equality operator for Mpeg2CodecCapability. */
inline bool operator==(const Mpeg2CodecCapability &lhs, const Mpeg2CodecCapability &rhs)
{
    return lhs.profiles == rhs.profiles && lhs.dynamicRanges == rhs.dynamicRanges;
}

/**
 * @brief Per-codec capability for H.264/AVC: profiles and per-codec dynamic ranges.
 */
struct H264CodecCapability
{
    std::vector<H264Profile> profiles;       /**< Supported H.264 profiles */
    std::vector<DynamicRange> dynamicRanges; /**< Dynamic ranges supported by this codec */
};

/** @brief Equality operator for H264CodecCapability. */
inline bool operator==(const H264CodecCapability &lhs, const H264CodecCapability &rhs)
{
    return lhs.profiles == rhs.profiles && lhs.dynamicRanges == rhs.dynamicRanges;
}

/**
 * @brief Per-codec capability for H.265/HEVC: profiles and per-codec dynamic ranges.
 */
struct H265CodecCapability
{
    std::vector<H265Profile> profiles;       /**< Supported H.265 profiles */
    std::vector<DynamicRange> dynamicRanges; /**< Dynamic ranges supported by this codec */
};

/** @brief Equality operator for H265CodecCapability. */
inline bool operator==(const H265CodecCapability &lhs, const H265CodecCapability &rhs)
{
    return lhs.profiles == rhs.profiles && lhs.dynamicRanges == rhs.dynamicRanges;
}

/**
 * @brief Per-codec capability for VP9: profiles and per-codec dynamic ranges.
 */
struct Vp9CodecCapability
{
    std::vector<Vp9Profile> profiles;        /**< Supported VP9 profiles */
    std::vector<DynamicRange> dynamicRanges; /**< Dynamic ranges supported by this codec */
};

/** @brief Equality operator for Vp9CodecCapability. */
inline bool operator==(const Vp9CodecCapability &lhs, const Vp9CodecCapability &rhs)
{
    return lhs.profiles == rhs.profiles && lhs.dynamicRanges == rhs.dynamicRanges;
}

/**
 * @brief Per-codec capability for AV1: profiles and per-codec dynamic ranges.
 */
struct Av1CodecCapability
{
    std::vector<Av1Profile> profiles;        /**< Supported AV1 profiles */
    std::vector<DynamicRange> dynamicRanges; /**< Dynamic ranges supported by this codec */
};

/** @brief Equality operator for Av1CodecCapability. */
inline bool operator==(const Av1CodecCapability &lhs, const Av1CodecCapability &rhs)
{
    return lhs.profiles == rhs.profiles && lhs.dynamicRanges == rhs.dynamicRanges;
}

/**
 * @brief Codec capabilities for all supported video codecs.
 *
 * Each field is std::nullopt when the codec is absent from the HFP config file.
 * Dynamic range information is per-codec (HFP schema v1.0.0).
 */
struct VideoCodecCapabilities
{
    std::optional<Mpeg2CodecCapability> mpeg2; /**< MPEG2 capability (nullopt if absent) */
    std::optional<H264CodecCapability> h264;   /**< H.264 capability (nullopt if absent) */
    std::optional<H265CodecCapability> h265;   /**< H.265 capability (nullopt if absent) */
    std::optional<Vp9CodecCapability> vp9;     /**< VP9 capability (nullopt if absent) */
    std::optional<Av1CodecCapability> av1;     /**< AV1 capability (nullopt if absent) */
};

/** @brief Equality operator for VideoCodecCapabilities. */
inline bool operator==(const VideoCodecCapabilities &lhs, const VideoCodecCapabilities &rhs)
{
    return lhs.mpeg2 == rhs.mpeg2 && lhs.h264 == rhs.h264 && lhs.h265 == rhs.h265 && lhs.vp9 == rhs.vp9 &&
           lhs.av1 == rhs.av1;
}

/**
 * @brief Decoder capability entry.
 *
 * Dynamic range is per-codec inside codecCapabilities (HFP schema v1.0.0).
 * There is no shared top-level dynamicRanges field.
 */
struct VideoDecoderCapability
{
    VideoCodecCapabilities codecCapabilities; /**< Per-codec capabilities */
};

/** @brief Equality operator for VideoDecoderCapability. */
inline bool operator==(const VideoDecoderCapability &lhs, const VideoDecoderCapability &rhs)
{
    return lhs.codecCapabilities == rhs.codecCapabilities;
}

/**
 * @brief Video decoder capabilities container
 */
struct VideoDecoderCapabilities
{
    std::string interfaceVersion;                     /**< Interface version string */
    std::string schemaVersion;                        /**< Schema version (e.g., "0.1.0") */
    std::vector<VideoDecoderCapability> capabilities; /**< List of decoder capabilities */
};

/** @brief Equality operator for VideoDecoderCapabilities. */
inline bool operator==(const VideoDecoderCapabilities &lhs, const VideoDecoderCapabilities &rhs)
{
    return lhs.interfaceVersion == rhs.interfaceVersion && lhs.schemaVersion == rhs.schemaVersion &&
           lhs.capabilities == rhs.capabilities;
}

} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_VIDEO_DECODER_CAPABILITIES_H_
