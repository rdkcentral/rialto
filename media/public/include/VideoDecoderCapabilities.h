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
#include <map>
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
    SDR,
    HLG,
    HDR10,
    HDR10PLUS,
    DOLBY_VISION
};

/**
 * @brief MPEG2 video profile types
 */
enum class Mpeg2ProfileType
{
    MPEG2_MAIN,
    MPEG2_SIMPLE
};

/**
 * @brief MPEG2 video level types
 */
enum class Mpeg2Level
{
    MPEG2_LEVEL_LOW,
    MPEG2_LEVEL_MAIN,
    MPEG2_LEVEL_HIGH
};

/**
 * @brief H.264/AVC profile types
 */
enum class H264ProfileType
{
    H264_BASELINE,
    H264_MAIN,
    H264_HIGH
};

/**
 * @brief H.264/AVC level types
 */
enum class H264Level
{
    H264_LEVEL_3,
    H264_LEVEL_3_1,
    H264_LEVEL_4,
    H264_LEVEL_4_1,
    H264_LEVEL_5,
    H264_LEVEL_5_1,
    H264_LEVEL_5_2
};

/**
 * @brief H.265/HEVC profile types
 */
enum class H265ProfileType
{
    H265_MAIN,
    H265_MAIN_10,
    H265_MAIN_10_HDR10
};

/**
 * @brief H.265/HEVC level types
 */
enum class H265Level
{
    H265_LEVEL_4,
    H265_LEVEL_4_1,
    H265_LEVEL_5,
    H265_LEVEL_5_1,
    H265_LEVEL_5_2,
    H265_LEVEL_6,
    H265_LEVEL_6_1,
    H265_LEVEL_6_2
};

/**
 * @brief VP9 profile types
 */
enum class Vp9ProfileType
{
    VP9_PROFILE_0,
    VP9_PROFILE_1,
    VP9_PROFILE_2,
    VP9_PROFILE_3
};

/**
 * @brief VP9 level types
 */
enum class Vp9Level
{
    VP9_LEVEL_1,
    VP9_LEVEL_1_1,
    VP9_LEVEL_2,
    VP9_LEVEL_2_1,
    VP9_LEVEL_3,
    VP9_LEVEL_3_1,
    VP9_LEVEL_4,
    VP9_LEVEL_4_1,
    VP9_LEVEL_5,
    VP9_LEVEL_5_1,
    VP9_LEVEL_5_2,
    VP9_LEVEL_6,
    VP9_LEVEL_6_1,
    VP9_LEVEL_6_2
};

/**
 * @brief AV1 profile types
 */
enum class Av1ProfileType
{
    AV1_MAIN,
    AV1_HIGH
};

/**
 * @brief AV1 level types
 */
enum class Av1Level
{
    AV1_LEVEL_4_0,
    AV1_LEVEL_4_1,
    AV1_LEVEL_5_0,
    AV1_LEVEL_5_1,
    AV1_LEVEL_5_2,
    AV1_LEVEL_6_0,
    AV1_LEVEL_6_1,
    AV1_LEVEL_6_2
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

/**
 * @brief Profile and level information for H.264/AVC
 */
struct H264Profile
{
    H264ProfileType type;     /**< Profile type */
    H264Level maxLevel;       /**< Maximum supported level */
    uint64_t maxBitrateInBps; /**< Maximum bitrate in bits per second */
};

/**
 * @brief Profile and level information for H.265/HEVC
 */
struct H265Profile
{
    H265ProfileType type;     /**< Profile type */
    H265Level maxLevel;       /**< Maximum supported level */
    uint64_t maxBitrateInBps; /**< Maximum bitrate in bits per second */
};

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

/**
 * @brief Per-codec capability for MPEG-2: profiles and per-codec dynamic ranges.
 */
struct Mpeg2CodecCapability
{
    std::vector<Mpeg2Profile> profiles;       /**< Supported MPEG2 profiles */
    std::vector<DynamicRange> dynamicRanges;  /**< Dynamic ranges supported by this codec */
};

/**
 * @brief Per-codec capability for H.264/AVC: profiles and per-codec dynamic ranges.
 */
struct H264CodecCapability
{
    std::vector<H264Profile> profiles;        /**< Supported H.264 profiles */
    std::vector<DynamicRange> dynamicRanges;  /**< Dynamic ranges supported by this codec */
};

/**
 * @brief Per-codec capability for H.265/HEVC: profiles and per-codec dynamic ranges.
 */
struct H265CodecCapability
{
    std::vector<H265Profile> profiles;        /**< Supported H.265 profiles */
    std::vector<DynamicRange> dynamicRanges;  /**< Dynamic ranges supported by this codec */
};

/**
 * @brief Per-codec capability for VP9: profiles and per-codec dynamic ranges.
 */
struct Vp9CodecCapability
{
    std::vector<Vp9Profile> profiles;         /**< Supported VP9 profiles */
    std::vector<DynamicRange> dynamicRanges;  /**< Dynamic ranges supported by this codec */
};

/**
 * @brief Per-codec capability for AV1: profiles and per-codec dynamic ranges.
 */
struct Av1CodecCapability
{
    std::vector<Av1Profile> profiles;         /**< Supported AV1 profiles */
    std::vector<DynamicRange> dynamicRanges;  /**< Dynamic ranges supported by this codec */
};

/**
 * @brief Codec capabilities for all supported video codecs.
 *
 * Each field is std::nullopt when the codec is absent from the HFP config file.
 * Dynamic range information is per-codec (HFP schema v1.0.0).
 */
struct VideoCodecCapabilities
{
    std::optional<Mpeg2CodecCapability> mpeg2; /**< MPEG2 capability (nullopt if absent) */
    std::optional<H264CodecCapability>  h264;  /**< H.264 capability (nullopt if absent) */
    std::optional<H265CodecCapability>  h265;  /**< H.265 capability (nullopt if absent) */
    std::optional<Vp9CodecCapability>   vp9;   /**< VP9 capability (nullopt if absent) */
    std::optional<Av1CodecCapability>   av1;   /**< AV1 capability (nullopt if absent) */
};

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

/**
 * @brief Video decoder capabilities container
 */
struct VideoDecoderCapabilities
{
    std::string interfaceVersion;                     /**< Interface version string */
    std::string schemaVersion;                        /**< Schema version (e.g., "0.1.0") */
    std::vector<VideoDecoderCapability> capabilities; /**< List of decoder capabilities */
};

} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_VIDEO_DECODER_CAPABILITIES_H_
