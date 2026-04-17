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
    PCM,
    AAC,
    MPEG_AUDIO,
    MP3,
    ALAC,
    SBC,
    DOLBY_AC3,
    DOLBY_AC4,
    DOLBY_MAT,
    DOLBY_TRUEHD,
    FLAC,
    VORBIS,
    OPUS,
    WMA,
    REALAUDIO,
    USAC,
    DTS,
    AVS
};

/**
 * @brief AAC profile types
 */
enum class AacProfile
{
    LC,
    HE_V1,
    HE_V2,
    ELD,
    X_HE
};

/**
 * @brief Dolby AC3 profile types
 */
enum class DolbyAc3Profile
{
    STANDARD,
    PLUS,
    PLUS_JOC
};

/**
 * @brief Dolby MAT profile types
 */
enum class DolbyMatProfile
{
    V1,
    V2
};

/**
 * @brief WMA profile types
 */
enum class WmaProfile
{
    STANDARD,
    PRO,
    LOSSLESS
};

/**
 * @brief RealAudio profile types
 */
enum class RealAudioProfile
{
    RA8,
    RA10
};

/**
 * @brief USAC (Unified Speech and Audio Coding) profile types
 */
enum class UsacProfile
{
    BASELINE,
    EXTENDED_HE_AAC
};

/**
 * @brief DTS profile types
 */
enum class DtsProfile
{
    CORE,
    HD_HRA,
    HD_MA
};

/**
 * @brief AVS audio profile types
 */
enum class AvsProfile
{
    AVS1_PART2,
    AVS2,
    AVS3
};

/**
 * @brief PCM codec capabilities
 */
struct PcmCapability
{
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief AAC codec capabilities
 */
struct AacCapability
{
    std::vector<AacProfile> profiles;    /**< Supported AAC profiles */
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief MPEG Audio codec capabilities
 */
struct MpegAudioCapability
{
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief MP3 codec capabilities
 */
struct Mp3Capability
{
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief ALAC codec capabilities
 */
struct AlacCapability
{
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief SBC codec capabilities
 */
struct SbcCapability
{
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief Dolby AC3 codec capabilities
 */
struct DolbyAc3Capability
{
    std::vector<DolbyAc3Profile> profiles; /**< Supported Dolby AC3 profiles */
    uint64_t maxBitrateInBps;              /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                  /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;            /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth;   /**< Maximum bit depth (optional) */
};

/**
 * @brief Dolby AC4 codec capabilities
 */
struct DolbyAc4Capability
{
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief Dolby MAT codec capabilities
 */
struct DolbyMatCapability
{
    std::vector<DolbyMatProfile> profiles; /**< Supported Dolby MAT profiles */
    uint64_t maxBitrateInBps;              /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                  /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;            /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth;   /**< Maximum bit depth (optional) */
};

/**
 * @brief Dolby TrueHD codec capabilities
 */
struct DolbyTruehdCapability
{
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief FLAC codec capabilities
 */
struct FlacCapability
{
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief Vorbis codec capabilities
 */
struct VorbisCapability
{
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief Opus codec capabilities
 */
struct OpusCapability
{
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief WMA codec capabilities
 */
struct WmaCapability
{
    std::vector<WmaProfile> profiles;    /**< Supported WMA profiles */
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief RealAudio codec capabilities
 */
struct RealAudioCapability
{
    std::vector<RealAudioProfile> profiles; /**< Supported RealAudio profiles */
    uint64_t maxBitrateInBps;               /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                   /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;             /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth;    /**< Maximum bit depth (optional) */
};

/**
 * @brief USAC codec capabilities
 */
struct UsacCapability
{
    std::vector<UsacProfile> profiles;   /**< Supported USAC profiles */
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief DTS codec capabilities
 */
struct DtsCapability
{
    std::vector<DtsProfile> profiles;    /**< Supported DTS profiles */
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief AVS audio codec capabilities
 */
struct AvsCapability
{
    std::vector<AvsProfile> profiles;    /**< Supported AVS profiles */
    uint64_t maxBitrateInBps;            /**< Maximum bitrate in bits per second */
    uint32_t maxChannels;                /**< Maximum number of channels */
    uint32_t maxSampleRateInHz;          /**< Maximum sample rate in Hz */
    std::optional<uint32_t> maxBitDepth; /**< Maximum bit depth (optional) */
};

/**
 * @brief Audio decoder capability entry for a specific rank
 */
struct AudioDecoderCapability
{
    std::optional<PcmCapability> pcm;                 /**< PCM capabilities (if supported) */
    std::optional<AacCapability> aac;                 /**< AAC capabilities (if supported) */
    std::optional<MpegAudioCapability> mpegAudio;     /**< MPEG Audio capabilities (if supported) */
    std::optional<Mp3Capability> mp3;                 /**< MP3 capabilities (if supported) */
    std::optional<AlacCapability> alac;               /**< ALAC capabilities (if supported) */
    std::optional<SbcCapability> sbc;                 /**< SBC capabilities (if supported) */
    std::optional<DolbyAc3Capability> dolbyAc3;       /**< Dolby AC3 capabilities (if supported) */
    std::optional<DolbyAc4Capability> dolbyAc4;       /**< Dolby AC4 capabilities (if supported) */
    std::optional<DolbyMatCapability> dolbyMat;       /**< Dolby MAT capabilities (if supported) */
    std::optional<DolbyTruehdCapability> dolbyTruehd; /**< Dolby TrueHD capabilities (if supported) */
    std::optional<FlacCapability> flac;               /**< FLAC capabilities (if supported) */
    std::optional<VorbisCapability> vorbis;           /**< Vorbis capabilities (if supported) */
    std::optional<OpusCapability> opus;               /**< Opus capabilities (if supported) */
    std::optional<WmaCapability> wma;                 /**< WMA capabilities (if supported) */
    std::optional<RealAudioCapability> realAudio;     /**< RealAudio capabilities (if supported) */
    std::optional<UsacCapability> usac;               /**< USAC capabilities (if supported) */
    std::optional<DtsCapability> dts;                 /**< DTS capabilities (if supported) */
    std::optional<AvsCapability> avs;                 /**< AVS capabilities (if supported) */
};

/**
 * @brief Audio decoder capabilities container
 */
struct AudioDecoderCapabilities
{
    std::string interfaceVersion;                     /**< Interface version string */
    std::string schemaVersion;                        /**< Schema version (e.g., "0.1.0") */
    std::vector<AudioDecoderCapability> capabilities; /**< List of decoder capabilities */
};

} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_AUDIO_DECODER_CAPABILITIES_H_
