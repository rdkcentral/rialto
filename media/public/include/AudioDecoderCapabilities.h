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
    DOLBY_EAC3,
    DOLBY_TRUEHD,
    FLAC,
    VORBIS,
    OPUS,
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
    STANDARD
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
 * @brief DolbyEac3 profile types (split from DolbyAc3)
 */
enum class DolbyEac3Profile
{
    PLUS,
    PLUS_JOC
};

/**
 * @brief MPEG Audio profile types
 */
enum class MpegAudioProfile
{
    LAYER_1,
    LAYER_2
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

/**
 * @brief PCM codec capabilities
 */
struct PcmCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

/**
 * @brief AAC codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct AacCapability
{
    std::map<AacProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

/**
 * @brief MPEG Audio codec capabilities (LAYER_1, LAYER_2 per HFP schema v1.0.0)
 */
struct MpegAudioCapability
{
    std::map<MpegAudioProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

/**
 * @brief MP3 codec capabilities
 */
struct Mp3Capability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

/**
 * @brief ALAC codec capabilities
 */
struct AlacCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

/**
 * @brief SBC codec capabilities
 */
struct SbcCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

/**
 * @brief Dolby AC3 codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct DolbyAc3Capability
{
    std::map<DolbyAc3Profile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

/**
 * @brief Dolby AC4 codec capabilities
 */
struct DolbyAc4Capability
{
    AudioProfileCapability base; /**< Single-profile capability */
};



/**
 * @brief Dolby TrueHD codec capabilities
 */
struct DolbyTruehdCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

/**
 * @brief FLAC codec capabilities
 */
struct FlacCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

/**
 * @brief Vorbis codec capabilities
 */
struct VorbisCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};

/**
 * @brief Opus codec capabilities
 */
struct OpusCapability
{
    AudioProfileCapability base; /**< Single-profile capability */
};



/**
 * @brief RealAudio codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct RealAudioCapability
{
    std::map<RealAudioProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

/**
 * @brief USAC codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct UsacCapability
{
    std::map<UsacProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

/**
 * @brief DTS codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct DtsCapability
{
    std::map<DtsProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

/**
 * @brief AVS audio codec capabilities
 *
 * Key: profile enum value. Value: per-profile capability (HFP schema v1.0.0).
 */
struct AvsCapability
{
    std::map<AvsProfile, AudioProfileCapability> profiles; /**< Per-profile capabilities */
};

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
    std::optional<DolbyEac3Capability> dolbyEac3;     /**< Dolby EAC3 capabilities (if supported) */

    std::optional<DolbyTruehdCapability> dolbyTruehd; /**< Dolby TrueHD capabilities (if supported) */
    std::optional<FlacCapability> flac;               /**< FLAC capabilities (if supported) */
    std::optional<VorbisCapability> vorbis;           /**< Vorbis capabilities (if supported) */
    std::optional<OpusCapability> opus;               /**< Opus capabilities (if supported) */

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
