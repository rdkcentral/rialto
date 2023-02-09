/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#ifndef FIREBOLT_RIALTO_MEDIA_COMMON_H_
#define FIREBOLT_RIALTO_MEDIA_COMMON_H_

/**
 * @file MediaCommon.h
 *
 * The definition of the Rialto Common types
 *
 */

#include <stddef.h>
#include <stdint.h>
#include <utility>
#include <vector>

namespace firebolt::rialto
{
/**
 * @brief The value of an invalid key session.
 */
constexpr int32_t kInvalidSessionId{-1};

/**
 * @brief The value of an invalid audio channels number.
 */
constexpr uint32_t kInvalidAudioChannels{0};

/**
 * @brief The value of an invalid audio sampling rate.
 */
constexpr uint32_t kInvalidAudioSampleRate{0};

/**
 * @brief The supported types of media source.
 */
enum class MediaSourceType
{
    UNKNOWN,
    AUDIO,
    VIDEO
};

/**
 * @brief The supported types of config types.
 */
enum class SourceConfigType
{
    UNKNOWN,
    AUDIO,
    VIDEO,
    VIDEO_DOLBY_VISION
};

/**
 * @brief The supported audio ease types.
 */
enum class AudioEaseType
{
    LINEAR = 0,
    INCUBIC,
    OUTCUBIC
};

/**
 * @brief The media type of media to be played.
 */
enum class MediaType
{
    UNKNOWN, /**< Media type not known. */
    MSE      /**< Media is MSE and will request data. */
};

/**
 * @brief The media source status. This is the status of the source
 *        after a read.
 */
enum class MediaSourceStatus
{
    OK,                  /**< Source data provided without error. */
    EOS,                 /**< Source reached the end of stream. */
    ERROR,               /**< There was an error providing source data. */
    CODEC_CHANGED,       /**< The codec has changed and the decoder must be reconfigured */
    NO_AVAILABLE_SAMPLES /**< Could not retrieve media samples. */
};

/**
 * @brief The Network State
 *
 * The network state reflects the state of the network. For backend
 * streaming, say using MediaPipelineURLDelegate, this is important
 * as the backend uses the network to obtain the media data directly.
 *
 * For streaming that uses the browser to obtain data, say Media Source
 * Extensions playback, only the states NetworkState::IDLE,
 * NetworkState::BUFFERED and NetworkState::DECODE_ERROR should be
 * indicated by the backend.
 */
enum class NetworkState
{
    UNKNOWN,            /**< An unknown or undefined network state. */
    IDLE,               /**< The network is idle. */
    BUFFERING,          /**< The network is buffering data before playing. */
    BUFFERING_PROGRESS, /**< The network is buffering data whilst playing. */
    BUFFERED,           /**< All the data is buffered. */
    STALLED,            /**< The network has stalled but may recover. */
    FORMAT_ERROR,       /**< The data is the wrong format. */
    NETWORK_ERROR,      /**< There has been a network error. Playback stops. */
    DECODE_ERROR        /**< There has been a decode error of the data. */
};

/**
 * @brief The Playback State
 *
 * The player will start IDLE. Once play() has been called the player
 * will be PLAYING, or once pause() has been called the player will be
 * PAUSED. A seek() request will result in SEEKING and once the seek
 * is complete FLUSHED will be issued followed by PLAYING. The STOPPED
 * state will be issued after a stop() request.
 */
enum class PlaybackState
{
    UNKNOWN,       /**< An unknown or undefined playback state. */
    IDLE,          /**< The backend player is idle. */
    PLAYING,       /**< The backend player is playing media. */
    PAUSED,        /**< The backend player is paused. */
    SEEKING,       /**< The backend player is seeking a new playback position. */
    FLUSHED,       /**< The backend player has flushed the media data. */
    STOPPED,       /**< The backend player has stopped playback. */
    END_OF_STREAM, /**< The backend player has got to the end of playback. */
    FAILURE        /**< The backend player failed to set playback state. */
};

/**
 * @brief Audio specific configuration
 */
struct AudioConfig
{
    uint32_t numberOfChannels = kInvalidAudioChannels; /**< The number of channels. */
    uint32_t sampleRate = kInvalidAudioSampleRate;     /**< The sampling rate.*/
    std::vector<uint8_t> codecSpecificConfig;          /**The audio specific config. Zero length if no specific config*/
};

/**
 * @brief AddSegmentStatus
 *
 * The add segment status. This is the status adding new segment to Rialto
 */
enum class AddSegmentStatus
{
    OK,       /**< Segment accepted. */
    NO_SPACE, /**< Too many frames sent or Rialto does not currently have space for this segment. */
    ERROR     /**< Unexpected error. */
};

/**
 * @brief A pair describing the clear and encrypted bytes
 *        in a sub-sample.
 */
struct SubSamplePair
{
    size_t numClearBytes;     /**< The number of clear bytes in the sample. */
    size_t numEncryptedBytes; /**< The number of encrypted bytes in the sample. */
};

/**
 * @brief Video decoder requirements used to allocate a suitable decoder for a MediaPipeline session
 */
struct VideoRequirements
{
    uint32_t maxWidth;  /**< Maximum width of video frames to be decoded. */
    uint32_t maxHeight; /**< Maximum height of video frames to be decoded. */
};

/**
 * @brief Information about the shared memory required for writting data.
 */
struct MediaPlayerShmInfo
{
    uint32_t maxMetadataBytes; /**< The maximum amount of metadata that can be written. */
    uint32_t metadataOffset;   /**< The offset to write the metadata. */
    uint32_t mediaDataOffset;  /**< The offset to write the media data. */
    uint32_t maxMediaBytes;    /**< The maximum amount of mediadata that can be written. */
};

/**
 * @brief The information provided in a QOS update.
 */
struct QosInfo
{
    uint64_t processed; /**< The total number of video frames/audio samples processed since MediaPipeline:load. */
    uint64_t dropped;   /**< The total number of video frames/audio samples dropped since MediaPipeline:load. */
};

/**
 * @brief The error return status for session management methods.
 */
enum class MediaKeyErrorStatus
{
    OK,             /**< No error. */
    FAIL,           /**< An unspecified error occurred. */
    BAD_SESSION_ID, /**< The session id is not recognised. */
    NOT_SUPPORTED,  /**< The request parameters are not supported. */
    INVALID_STATE   /**< The object is in an invalid state for the operation. */
};

/**
 * @brief The media key session type.
 */
enum class KeySessionType
{
    UNKNOWN,                   /**< The session type is unknown. */
    TEMPORARY,                 /**< The session is a temporary session. */
    PERSISTENT_LICENCE,        /**< The session is a persistent session. */
    PERSISTENT_RELEASE_MESSAGE /**< The session's persistent licence should be released. */
};

/**
 * @brief The init data type.
 */
enum class InitDataType
{
    UNKNOWN,  /**< The init data type is unknown. */
    CENC,     /**< The init data is in CENC format. */
    KEY_IDS,  /**< The init data is key ids. */
    WEBM,     /**< The init data is in WEBM format. */
    DRMHEADER /**< The init data is in DrmHeader format. */
};

/**
 * @brief The key status.
 */
enum class KeyStatus
{
    USABLE,
    EXPIRED,
    OUTPUT_RESTRICTED,
    PENDING,
    INTERNAL_ERROR,
    RELEASED
};

/**
 * @brief The alignment of media segment
 */
enum class SegmentAlignment
{
    UNDEFINED,
    NAL,
    AU
};

/**
 * @brief The Stream Format of media segment
 */
enum class StreamFormat
{
    UNDEFINED,
    RAW,
    AVC,
    BYTE_STREAM
};

/**
 * @brief A vector of key ID/key status pairs.
 */
typedef std::vector<std::pair<std::vector<unsigned char>, KeyStatus>> KeyStatusVector;

/**
 * @brief Information about the shared memory required for writting data for the web audio playback.
 */
struct WebAudioShmInfo
{
    uint32_t offsetMain; /**< The offset to start writing the audio data. */
    uint32_t lengthMain; /**< The maximum number of bytes to write at offsetMain. */
    uint32_t offsetWrap; /**< The offset to continue writing the audio data if buffer wrapped. */
    uint32_t lengthWrap; /**< The maximum number of bytes to write at offsetWrap. */
};

/**
 * @brief Pcm config information.
 */
struct WebAudioPcmConfig
{
    uint32_t rate;
    uint32_t channels;
    uint32_t sampleSize;
    bool isBigEndian;
    bool isSigned;
    bool isFloat;
};

/**
 * @brief Type dependent configuration data.
 */
union WebAudioConfig
{
    WebAudioPcmConfig pcm;
};

/**
 * @brief The Web Audio Player State.
 */
enum class WebAudioPlayerState
{
    UNKNOWN,       /**< An unknown or undefined playback state. */
    IDLE,          /**< The player is ready to play media. */
    PLAYING,       /**< The player is playing media. */
    PAUSED,        /**< The player is has paused media playback. */
    END_OF_STREAM, /**< The player has got to the end of playback. */
    FAILURE        /**< The player failed to set playback state. */
};

} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_MEDIA_COMMON_H_
