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

#ifndef FIREBOLT_RIALTO_I_MEDIA_PIPELINE_H_
#define FIREBOLT_RIALTO_I_MEDIA_PIPELINE_H_

/**
 * @file IMediaPipeline.h
 *
 * The definition of the IMediaPipeline interface.
 *
 * This interface defines the public API of Rialto for playback of AV content.
 */

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "IMediaPipelineClient.h"
#include <MediaCommon.h>

namespace firebolt::rialto
{
class IMediaPipeline;

/**
 * @brief IMediaPipeline factory class, returns a concrete implementation of IMediaPipeline
 */
class IMediaPipelineFactory
{
public:
    IMediaPipelineFactory() = default;
    virtual ~IMediaPipelineFactory() = default;

    /**
     * @brief Create a IMediaPipelineFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IMediaPipelineFactory> createFactory();

    /**
     * @brief IMediaPipeline factory method, returns a concrete implementation of IMediaPipeline
     *
     * @param[in] client            : The Rialto media player client.
     * @param[in] videoRequirements : The video decoder requirements for the MediaPipeline session
     *
     * @retval the new backend instance or null on error.
     */
    virtual std::unique_ptr<IMediaPipeline> createMediaPipeline(std::weak_ptr<IMediaPipelineClient> client,
                                                                const VideoRequirements &videoRequirements) const = 0;
};

/**
 * @brief The definition of the IMediaPipeline interface.
 *
 * This interface defines the public API of Rialto for playback of AV content which
 * should be implemented by both Rialto Client & Rialto Server.
 */
class IMediaPipeline
{
public:
    IMediaPipeline() = default;
    virtual ~IMediaPipeline() = default;

    IMediaPipeline(const IMediaPipeline &) = delete;
    IMediaPipeline &operator=(const IMediaPipeline &) = delete;
    IMediaPipeline(IMediaPipeline &&) = delete;
    IMediaPipeline &operator=(IMediaPipeline &&) = delete;

    /**
     * @brief A class that represents a source of media data.
     */
    class MediaSource
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~MediaSource() {}

        /**
         * @brief Create a copy
         */
        virtual std::unique_ptr<MediaSource> copy() const = 0;

        /**
         * @brief Return the source type.
         */
        virtual MediaSourceType getType() const = 0;

        /**
         * @brief Return the MIME type.
         */
        std::string getMimeType() const { return m_mimeType; }

        /**
         * @brief Return the source config type.
         */
        SourceConfigType getConfigType() const { return m_configType; }

        /**
         * @brief Return the source id.
         */
        int32_t getId() const { return m_id; }

        /**
         * @brief Set the source id.
         */
        void setId(int32_t id) { m_id = id; }

        /**
         * @brief Gets the segment alignment
         */
        SegmentAlignment getSegmentAlignment() const { return m_alignment; }

        /**
         * @brief Gets the codec data
         */
        const CodecData &getCodecData() const { return m_codecData; }

        /**
         * @brief Gets the stream format
         */
        StreamFormat getStreamFormat() const { return m_streamFormat; }

    protected:
        /**
         * @brief Default constructor.
         *
         * @param[out] id           : The source ID will be set by a successful call to attachSource()
         * @param[in]  configType   : The source config type.
         * @param[in]  mimeType     : The mime type string.
         * @param[in]  alignment    : The alignment of media segment.
         * @param[in]  streamFormat : The stream format
         * @param[in]  codecData    : The additional data for decoder
         */
        explicit MediaSource(int32_t id = 0, SourceConfigType configType = SourceConfigType::UNKNOWN,
                             const std::string &mimeType = std::string(),
                             SegmentAlignment alignment = SegmentAlignment::UNDEFINED,
                             StreamFormat streamFormat = StreamFormat::UNDEFINED,
                             const CodecData &codecData = CodecData())
            : m_id(id), m_configType(configType), m_mimeType(mimeType), m_alignment(alignment),
              m_streamFormat(streamFormat), m_codecData(codecData)
        {
        }
        /**
         * @brief The source id. Parameter will be set by a successful call to attachSource()
         */
        int32_t m_id;

        /**
         * @brief The source config type.
         */
        SourceConfigType m_configType;

        /**
         * @brief The MIME type.
         */
        std::string m_mimeType;

        /**
         * @brief The alignment of media segment
         */
        SegmentAlignment m_alignment;

        /**
         * @brief The stream format
         */
        StreamFormat m_streamFormat;

        /**
         * @brief Additional data for decoder
         */
        CodecData m_codecData;
    };

    /**
     * @brief A class that represents media source audio derived from MediaSource class, which represents the source of
     * media data
     */

    class MediaSourceAudio : public MediaSource
    {
    public:
        /**
         * @brief Constructor for audio specific configuration.
         *
         * @param[out] id           : The source ID will be set by a successful call to attachSource()
         * @param[in]  mimeType     : The mime type string.
         * @param[in]  audioConfig  : The audio specific configuration.
         * @param[in]  alignment    : The alignment of media segment.
         * @param[in]  streamFormat : The stream format
         * @param[in]  codecData    : The additional data for decoder
         */
        MediaSourceAudio(int32_t id, const std::string &mimeType, const AudioConfig &audioConfig = AudioConfig(),
                         SegmentAlignment alignment = SegmentAlignment::UNDEFINED,
                         StreamFormat streamFormat = StreamFormat::UNDEFINED, const CodecData &codecData = CodecData())
            : MediaSource(id, SourceConfigType::AUDIO, mimeType, alignment, streamFormat, codecData),
              m_audioConfig(audioConfig)
        {
        }

        ~MediaSourceAudio() {}

        MediaSourceType getType() const override { return MediaSourceType::AUDIO; }
        std::unique_ptr<MediaSource> copy() const override { return std::make_unique<MediaSourceAudio>(*this); }

        /**
         * @brief Gets the audio specific configuration
         *
         * @retval audio specific configuration
         */
        const AudioConfig &getAudioConfig() const { return m_audioConfig; }

    protected:
        /**
         * @brief Variable that stores the audio specific configuration
         */
        AudioConfig m_audioConfig;
    };

    /**
     * @brief A class that represents media source video derived from MediaSource class, which represents the source of
     * media data
     */

    class MediaSourceVideo : public MediaSource
    {
    public:
        /**
         * @brief Constructor for video specific configuration.
         *
         * @param[out] id           : The source ID will be set by a successful call to attachSource()
         * @param[in]  mimeType     : The mime type string.
         * @param[in]  alignment    : The alignment of media segment.
         * @param[in]  streamFormat : The stream format
         * @param[in]  codecData    : The additional data for decoder
         */
        MediaSourceVideo(int32_t id, const std::string &mimeType,
                         SegmentAlignment alignment = SegmentAlignment::UNDEFINED,
                         StreamFormat streamFormat = StreamFormat::UNDEFINED, const CodecData &codecData = CodecData())
            : MediaSource(id, SourceConfigType::VIDEO, mimeType, alignment, streamFormat, codecData)
        {
        }
        ~MediaSourceVideo() {}

        MediaSourceType getType() const override { return MediaSourceType::VIDEO; }
        std::unique_ptr<MediaSource> copy() const { return std::make_unique<MediaSourceVideo>(*this); }

    protected:
        /**
         * @brief Constructor for video specific configuration.
         *
         * @param[out] id               : The source ID will be set by a successful call to attachSource()
         * @param[in]  mimeType         : The mime type string.
         * @param[in]  sourceConfigType : The source config type
         * @param[in]  alignment        : The alignment of media segment.
         * @param[in]  streamFormat     : The stream format
         * @param[in]  codecData        : The additional data for decoder
         */
        MediaSourceVideo(int32_t id, SourceConfigType sourceConfigType, const std::string &mimeType,
                         SegmentAlignment alignment = SegmentAlignment::UNDEFINED,
                         StreamFormat streamFormat = StreamFormat::UNDEFINED, const CodecData &codecData = CodecData())
            : MediaSource(id, sourceConfigType, mimeType, alignment, streamFormat, codecData)
        {
        }
    };

    /**
     * @brief A class that represents media source video dolby vision derived from media source video data
     */

    class MediaSourceVideoDolbyVision : public MediaSourceVideo
    {
    public:
        /**
         * @brief Constructor for dolby vision specific configuration.
         *
         * @param[out] id                : The source ID will be set by a successful call to attachSource()
         * @param[in] mimeType           : The mime type string.
         * @param[in] dolbyVisionProfile : The dolby vision profile
         * @param[in] alignment          : The alignment of media segment.
         * @param[in] streamFormat       : The stream format
         * @param[in] codecData          : The additional data for decoder
         */
        MediaSourceVideoDolbyVision(int32_t id, const std::string &mimeType, int32_t dolbyVisionProfile,
                                    SegmentAlignment alignment = SegmentAlignment::UNDEFINED,
                                    StreamFormat streamFormat = StreamFormat::UNDEFINED,
                                    const CodecData &codecData = CodecData())
            : MediaSourceVideo(id, SourceConfigType::VIDEO_DOLBY_VISION, mimeType, alignment, streamFormat, codecData),
              m_dolbyVisionProfile(dolbyVisionProfile)
        {
        }
        ~MediaSourceVideoDolbyVision() {}
        std::unique_ptr<MediaSource> copy() const { return std::make_unique<MediaSourceVideoDolbyVision>(*this); }

        /**
         * @brief Gets the dolby vision profile
         *
         * @retval dolby vision profile
         */
        uint32_t getDolbyVisionProfile() const { return m_dolbyVisionProfile; }

    protected:
        /**
         * @brief Variable that stores the Dolby Vision Profile
         */
        uint32_t m_dolbyVisionProfile;
    };

    /**
     * @brief A class that represents a media segment
     */
    class MediaSegment
    {
    public:
        /**
         * @brief Default constructor.
         *
         * @param[in] sourceId      : The source id. Value should be set to the MediaSource.id returned after attachSource()
         * @param[in] type          : The source type.
         * @param[in] timeStamp     : The timestamp in nanoseconds.
         * @param[in] duration      : The duration in nanoseconds.
         */
        MediaSegment(int32_t sourceId = 0, MediaSourceType type = MediaSourceType::UNKNOWN, int64_t timeStamp = 0,
                     int64_t duration = 0)
            : m_sourceId(sourceId), m_type(type), m_data(nullptr), m_dataLength(0u), m_timeStamp(timeStamp),
              m_duration(duration),
              m_encrypted(false), m_mediaKeySessionId{0}, m_initWithLast15{0}, m_alignment{SegmentAlignment::UNDEFINED}
        {
        }

        /**
         * @brief Virtual destructor.
         */
        virtual ~MediaSegment() {}

        /**
         * @brief Makes a shallow copy of the segment
         *
         * @retval Shallow copy of the segment
         */
        virtual std::unique_ptr<MediaSegment> copy() const { return std::make_unique<MediaSegment>(*this); }

        /**
         * @brief Return the source id.
         *
         * @retval the source id.
         */
        int32_t getId() const { return m_sourceId; }

        /**
         * @brief The source type for the data.
         *
         * @retval the source type.
         */
        MediaSourceType getType() const { return m_type; }

        /**
         * @brief Returns a pointer to the data.
         *
         * @retval the data.
         */
        const uint8_t *getData() const { return m_data; }

        /**
         * @brief Returns a the data length.
         *
         * @retval the data.
         */
        uint32_t getDataLength() const { return m_dataLength; }

        /**
         * @brief Returns the time stamp.
         *
         * @retval the timestamp in nanoseconds.
         */
        int64_t getTimeStamp() const { return m_timeStamp; }

        /**
         * @brief Sets the time stamp (value in nanoseconds).
         */
        void setTimeStamp(int64_t timeStamp) { m_timeStamp = timeStamp; }

        /**
         * @brief Returns the duration.
         *
         * @retval the duration in nanoseconds.
         */
        int64_t getDuration() const { return m_duration; }

        /**
         * @brief Returns a pointer to the extra data.
         *
         * @retval the data.
         */
        const std::vector<uint8_t> &getExtraData() const { return m_extraData; }

        /**
         * @brief Indicates that the data is encrypted.
         *
         * @retval true if the data is encrypted.
         */
        bool isEncrypted() const { return m_encrypted; }

        /**
         * @brief Returns the media key session id. Empty if unencrypted.
         *
         * @retval the media key session id.
         */
        const int32_t getMediaKeySessionId() const { return m_mediaKeySessionId; }

        /**
         * @brief Returns the key id. Empty if unencrypted.
         *
         * @retval the key id.
         */
        const std::vector<uint8_t> &getKeyId() const { return m_keyId; }

        /**
         * @brief Returns the initialisation vector. Empty if unencrypted.
         *
         * @retval the initialisation vector.
         */
        const std::vector<uint8_t> &getInitVector() const { return m_initVector; }

        /**
         * @brief Returns the sub samples. Empty if unencrypted.
         *
         * @retval the sub samples.
         */
        const std::vector<SubSamplePair> &getSubSamples() const { return m_subSamples; }

        /**
         * @brief Returns the initWithLast15 value
         *
         * @retval the initWithLast15 value.
         */
        const uint32_t getInitWithLast15() const { return m_initWithLast15; }

        /**
         * @brief Returns the segment alignment
         *
         * @retval the segment alignment
         */
        const SegmentAlignment getSegmentAlignment() const { return m_alignment; }

        /**
         * @brief Gets the codec data
         *
         * @retval the codec data
         */
        const CodecData &getCodecData() const { return m_codecData; }

    protected:
        /**
         * @brief The source id.
         */
        int32_t m_sourceId;

        /**
         * @brief The source type.
         */
        MediaSourceType m_type;

        /**
         * @brief The data
         */
        const uint8_t *m_data;

        /**
         * @brief The data length
         */
        uint32_t m_dataLength;

        /**
         * @brief The time stamp.
         */
        int64_t m_timeStamp;

        /**
         * @brief The duration.
         */
        int64_t m_duration;

        /**
         * @brief Additional data for decoder
         */
        CodecData m_codecData;

        /**
         * @brief The data
         */
        std::vector<uint8_t> m_extraData;

        /**
         * @brief Indicates the data is encrypted.
         */
        bool m_encrypted;

        /**
         * @brief Key session ID to use for decryption - only required for Netflix.
         */
        int32_t m_mediaKeySessionId;

        /**
         * @brief The encryption key id.
         */
        std::vector<uint8_t> m_keyId;

        /**
         * @brief The encryption key initialisation vector.
         */
        std::vector<uint8_t> m_initVector;

        /**
         * @brief The sub-sample pairs.
         */
        std::vector<SubSamplePair> m_subSamples;

        /**
         * @brief Whether decryption context needs to be initialized with
         * last 15 bytes. Currently this only applies to PlayReady DRM.
         */
        uint32_t m_initWithLast15;

        /**
         * @brief The alignment of media segment
         */
        SegmentAlignment m_alignment;

    public:
        /**
         * @brief Sets the segment data.
         *
         * @warning Note that the caller must guarantee that the buffer referenced by 'data' must remain
         * valid until the corresponding call to haveData() has completed (at which point the data will
         * have been copied out).
         *
         * @note This is for performance reasons to avoid multiple copies of AV data. A raw pointer is
         * used for the same reason since most runtimes will expose a raw pointer to the data.
         *
         * @retval true on success.
         */
        bool setData(uint32_t dataLength, const uint8_t *data)
        {
            m_dataLength = dataLength;
            m_data = data;
            return true;
        }

        /**
         * @brief Sets the extra data.
         *
         * @retval true on success.
         */
        bool setExtraData(const std::vector<uint8_t> &extraData)
        {
            m_extraData = extraData;
            return true;
        }

        /**
         * @brief Sets the segment alignment
         *
         * @param[in] alignment : The new segment alignment
         */
        void setSegmentAlignment(const SegmentAlignment &alignment) { m_alignment = alignment; }

        /**
         * @brief Sets new codec_data for the segment.
         *
         * @note Should only be called if the codec data changes
         *
         * @param[in] codecData  The updated codec data for the source
         */
        void setCodecData(const CodecData &codecData) { m_codecData = codecData; }

        /**
         * @brief Sets the encrypted flag.
         *
         * @param[in] encrypted : Set true to indicated encrypted data.
         */
        void setEncrypted(bool encrypted) { m_encrypted = encrypted; }

        /**
         * @brief Sets the media key session id.
         *
         * @param[in] mksId : the media key session id.
         */
        void setMediaKeySessionId(int32_t mksId) { m_mediaKeySessionId = mksId; }

        /**
         * @brief Sets the key id.
         *
         * @param[in] keyId : The key id.
         */
        void setKeyId(const std::vector<uint8_t> &keyId) { m_keyId = keyId; }

        /**
         * @brief Sets the encryption initialisation vector.
         *
         * @param[in] initVector : The initialisation vector.
         */
        void setInitVector(const std::vector<uint8_t> &initVector) { m_initVector = initVector; }

        /**
         * @brief Adds a sub-sample pair to the sub samples.
         *
         * @param[in] numClearBytes     : The number of clear bytes.
         * @param[in] numEncryptedBytes : The number of encrypted bytes.
         */
        void addSubSample(size_t numClearBytes, size_t numEncryptedBytes)
        {
            m_subSamples.emplace_back(SubSamplePair{numClearBytes, numEncryptedBytes});
        }

        /**
         * @brief Sets initWithLast15 value
         *
         * @param[in] initWithLast15 : initWithLast15 value
         */
        void setInitWithLast15(uint32_t initWithLast15) { m_initWithLast15 = initWithLast15; }

        /**
         * @brief Copies the data from other to this.
         */
        void copy(const MediaSegment &other);
    };

    /**
     * @brief A class that represents media source audio data
     */
    class MediaSegmentAudio : public MediaSegment
    {
    public:
        /**
         * @brief Default constructor.
         *
         * @param[in] sourceId         : The source id. Value should be set to the MediaSource.id returned after attachSource()
         * @param[in] timeStamp        : The timestamp in nanoseconds.
         * @param[in] duration         : The duration in nanoseconds.
         * @param[in] sampleRate       : The sample rate in samples per second.
         * @param[in] numberOfChannels : The number of audio channels.
         */
        MediaSegmentAudio(int32_t sourceId = 0, int64_t timeStamp = 0, int64_t duration = 0, int32_t sampleRate = 0,
                          int32_t numberOfChannels = 0)
            : MediaSegment(sourceId, MediaSourceType::AUDIO, timeStamp, duration), m_sampleRate(sampleRate),
              m_numberOfChannels(numberOfChannels)
        {
        }

        /**
         * @brief Copy constructor.
         */
        MediaSegmentAudio(const MediaSegmentAudio &other) : MediaSegment(other)
        {
            m_sampleRate = other.m_sampleRate;
            m_numberOfChannels = other.m_numberOfChannels;
        }

        /**
         * @brief Makes a shallow copy of the segment
         *
         * @retval Shallow copy of the segment
         */
        std::unique_ptr<MediaSegment> copy() const override { return std::make_unique<MediaSegmentAudio>(*this); }

        /**
         * @brief Return the audio sample rate.
         *
         * @retval the sample rate in samples per second.
         */
        int32_t getSampleRate() const { return m_sampleRate; }

        /**
         * @brief Return the number of audio channels.
         *
         * @retval the number of channels.
         */
        int32_t getNumberOfChannels() const { return m_numberOfChannels; }

        /**
         * @brief Copy assignment operator.
         *
         * @retval the copy.
         */
        MediaSegmentAudio &operator=(const MediaSegmentAudio &other)
        {
            copy(other);
            return *this;
        }

    protected:
        /**
         * @brief Copies the data from other to this.
         */
        void copy(const MediaSegmentAudio &other);

        /**
         * @brief The audio sample rate.
         */
        int32_t m_sampleRate;

        /**
         * @brief The number of audio channels.
         */
        int32_t m_numberOfChannels;
    };

    /**
     * @brief A class that represents media source video data
     */
    class MediaSegmentVideo : public MediaSegment
    {
    public:
        /**
         * @brief Default constructor.
         *
         * @param[in] sourceId  : The source id. Value should be set to the MediaSource.id returned after attachSource()
         * @param[in] timeStamp : The timestamp in nanoseconds.
         * @param[in] duration  : The duration in nanoseconds.
         * @param[in] width     : The video width in pixels.
         * @param[in] height    : The video height in pixels.
         */
        MediaSegmentVideo(int32_t sourceId = 0, int64_t timeStamp = 0, int64_t duration = 0, int32_t width = 0,
                          int32_t height = 0)
            : MediaSegment(sourceId, MediaSourceType::VIDEO, timeStamp, duration), m_width(width), m_height(height)
        {
        }

        /**
         * @brief Copy constructor.
         */
        MediaSegmentVideo(const MediaSegmentVideo &other) : MediaSegment(other)
        {
            m_width = other.m_width;
            m_height = other.m_height;
        }

        /**
         * @brief Makes a shallow copy of the segment
         *
         * @retval Shallow copy of the segment
         */
        std::unique_ptr<MediaSegment> copy() const override { return std::make_unique<MediaSegmentVideo>(*this); }

        /**
         * @brief Return the video width.
         *
         * @retval the video width in pixels.
         */
        int32_t getWidth() const { return m_width; }

        /**
         * @brief Return the video height.
         *
         * @retval the video height in pixels.
         */
        int32_t getHeight() const { return m_height; }

        /**
         * @brief Copy assignment operator.
         *
         * @retval the copy.
         */
        MediaSegmentVideo &operator=(const MediaSegmentVideo &other)
        {
            copy(other);
            return *this;
        }

    protected:
        /**
         * @brief Copies the data from other to this.
         */
        void copy(const MediaSegmentVideo &other);

        /**
         * @brief The video width in pixels.
         */
        int32_t m_width;

        /**
         * @brief The video height in pixels.
         */
        int32_t m_height;
    };

    /**
     * @brief A vector that contains one or more media segments.
     */
    typedef std::vector<std::unique_ptr<MediaSegment>> MediaSegmentVector;

    /**
     * @brief Returns the media player client.
     *
     * @retval The media player client.
     */
    virtual std::weak_ptr<IMediaPipelineClient> getClient() = 0;

    /**
     * @brief Loads the media and backend delegate.
     *
     * This method loads the media and backend appropriate for the media.
     * The media type determines the backend delegate to use to play back
     * the media. The MIME type confirms the type and CODECs for the media.
     * The URL will comprise the media URL for types MediaType::URL and
     * MediaType::HLS. For MediaType::MSE the URL will comprise a blob URL
     * as the data is loaded by the browser.
     *
     * @param[in] type     : The media type.
     * @param[in] mimeType : The MIME type.
     * @param[in] url      : The URL.
     */
    virtual bool load(MediaType type, const std::string &mimeType, const std::string &url) = 0;

    /**
     * @brief Attaches a source stream to the backend.
     *
     * This method is called by Rialto Client to attach a media source stream to
     * the backend. It is only called when Media Source Extensions are
     * being used. I.e. if the MediaType value in load() is
     * MediaType::MSE.
     *
     * @param[in] source : The source.
     *
     * @retval true on success.
     */
    virtual bool attachSource(const std::unique_ptr<MediaSource> &source) = 0;

    /**
     * @brief Unattaches a source.
     *
     * This method is called by Rialto Client to detach a media source stream from
     * the backend. It is only called when Media Source Extensions are
     * being used. I.e. if the MediaType value in load() is
     * MediaType::MSE.
     *
     * @param[in] id : The source id. Value should be set to the MediaSource.id returned after attachSource()
     *
     * @retval true on success.
     */
    virtual bool removeSource(int32_t id) = 0;

    /**
     * @brief Starts playback of the media.
     *
     * This method is considered to be asychronous and MUST NOT block
     * but should request playback and then return.
     *
     * Once the backend is successfully playing it should notify the
     * media player client of playback state
     * IMediaPipelineClient::PlaybackState::PLAYING.
     *
     * @retval true on success.
     */
    virtual bool play() = 0;

    /**
     * @brief Pauses playback of the media.
     *
     * This method is considered to be asychronous and MUST NOT block
     * but should request the playback pause and then return.
     *
     * Once the backend is successfully playing it should notify the
     * media player client of playback state
     *IMediaPipelineClient::PlaybackState::PAUSED.
     *
     * @retval true on success.
     */
    virtual bool pause() = 0;

    /**
     * @brief Stops playback of the media.
     *
     * This method is considered to be asychronous and MUST NOT block
     * but should request the playback stop and then return.
     *
     * Once the backend is successfully stopped it should notify the
     * media player client of playback state
     * IMediaPipelineClient::PlaybackState::STOPPED.
     *
     * @retval true on success.
     */
    virtual bool stop() = 0;

    /**
     * @brief Set the playback rate
     *
     * This method sets the playback rate. The supported playback rates
     * are dependent upon the backend playback method.
     *
     * @param[in] rate : The playback rate.
     *
     * @retval true on success.
     */
    virtual bool setPlaybackRate(double rate) = 0;

    /**
     * @brief Set the playback position in nanoseconds.
     *
     * If playback has not started this method sets the start position
     * for playback. If playback has started this method performs a seek.
     *
     * This method is considered to be asychronous and MUST NOT block
     * but should request the new playback position and then return.
     *
     * Once the backend is seeking it should notify the media player
     * client of playback state
     * IMediaPipelineClient::PlaybackState::SEEKING. When seeking has
     * completed the state IMediaPipelineClient::PlaybackState::FLUSHED
     * should be notified followed by
     * IMediaPipelineClient::PlaybackState::PLAYING.
     *
     * @param[in] position : The playback position in nanoseconds.
     *
     * @retval true on success.
     */
    virtual bool setPosition(int64_t position) = 0;

    /**
     * @brief Get the playback position in nanoseconds.
     *
     * This method is sychronous, it returns current playback position
     *
     * @param[out] position : The playback position in nanoseconds
     *
     * @retval true on success.
     */
    virtual bool getPosition(int64_t &position) = 0;

    /**
     * @brief Sets the coordinates of where the video should be displayed.
     *
     * @param[in] x      : The x position in pixels.
     * @param[in] y      : The y position in pixels.
     * @param[in] width  : The width in pixels.
     * @param[in] height : The height in pixels.
     *
     * @retval true on success.
     */
    virtual bool setVideoWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

    /**
     * @brief Returns data requested using notifyNeedMediaData().
     *
     * This method is called as a result of calling notifyNeedMediaData() on
     * the media client. The status value indicates the success or
     * otherwise of the request. The client must first call addSegment() for
     * for each media segment to be sent.
     * A status of MediaSourceStatus::OK indicates success and there will
     * be data within the data vector. A status of MediaSourceStatus::EOS
     * indicates success but the end of the stream was reached. Again
     * there will be data in the vector. A status of
     * MediaSourceStatus::ERROR indicates an error occurred and no data
     * was returned.
     *
     *
     * @param[in] status : The status
     * @param[in] needDataRequestId : Need data request id
     */
    virtual bool haveData(MediaSourceStatus status, uint32_t needDataRequestId) = 0;

    /**
     * @brief Adds a single segment to Rialto in response to notifyNeedData()
     *
     * This method is should be called by the client as a result of a notifyNeedData()
     * notification. The client should call this API for each segment to be sent in
     * response to the notification and then call haveData() when all segments
     * have been 'added'.
     *
     * If the return code is NO_SPACE the segment has not been accepted but this is
     * not an error. The client should retain the segment until the next notifyNeedData()
     * is received for the source and immediately call haveData() to trigger Rialto
     * to start processing the segments already added.
     *
     * @param[in] needDataRequestId : The status
     * @param[in] mediaSegment : The data returned.
     *
     * @retval status of adding segment
     */
    virtual AddSegmentStatus addSegment(uint32_t needDataRequestId, const std::unique_ptr<MediaSegment> &mediaSegment) = 0;

    /**
     * @brief Requests to render a prerolled frame
     */
    virtual bool renderFrame() = 0;

    /**
     * @brief Set level and transition of audio attenuation.
     *        Sets the current volume for the pipeline (0.0 silent -> 1.0 full volume)
     *
     * @param[in] volume Target volume level (0.0 - 1.0)
     *
     * @retval true on success false otherwise
     */
    virtual bool setVolume(double volume) = 0;

    /**
     * @brief Get current audio level. Fetches the current volume level for the pipeline.
     *
     * @param[out] volume Current volume level (range 0.0 - 1.0)
     *
     * @retval true on success false otherwise
     */
    virtual bool getVolume(double &volume) = 0;
};

}; // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_I_MEDIA_PIPELINE_H_
