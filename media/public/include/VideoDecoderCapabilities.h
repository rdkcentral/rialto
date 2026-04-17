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
 * @brief Codec capabilities for a specific video codec
 */
struct VideoCodecCapabilities
{
    std::vector<Mpeg2Profile> mpeg2Profiles; /**< MPEG2 profiles (if codec is MPEG2) */
    std::vector<H264Profile> h264Profiles;   /**< H.264 profiles (if codec is H264) */
    std::vector<H265Profile> h265Profiles;   /**< H.265 profiles (if codec is H265) */
    std::vector<Vp9Profile> vp9Profiles;     /**< VP9 profiles (if codec is VP9) */
    std::vector<Av1Profile> av1Profiles;     /**< AV1 profiles (if codec is AV1) */
};

/**
 * @brief Decoder capability entry for a specific rank
 */
struct VideoDecoderCapability
{
    VideoCodecCapabilities codecCapabilities; /**< List of supported codec capabilities */
    std::vector<DynamicRange> dynamicRanges;  /**< List of supported dynamic ranges */
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
