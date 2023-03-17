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

#ifndef FIREBOLT_RIALTO_SERVER_I_GST_GENERIC_PLAYER_H_
#define FIREBOLT_RIALTO_SERVER_I_GST_GENERIC_PLAYER_H_

#include <MediaCommon.h>
#include <memory>
#include <stdint.h>
#include <string>

#include "IDataReader.h"
#include "IDecryptionService.h"
#include "IGstGenericPlayerClient.h"
#include "IMediaPipeline.h"
#include "IRdkGstreamerUtilsWrapper.h"

namespace firebolt::rialto::server
{
class IGstGenericPlayer;

/**
 * @brief IGstGenericPlayer factory class, returns a concrete implementation of IGstGenericPlayer
 */
class IGstGenericPlayerFactory
{
public:
    IGstGenericPlayerFactory() = default;
    virtual ~IGstGenericPlayerFactory() = default;

    /**
     * @brief Gets the IGstGenericPlayerFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IGstGenericPlayerFactory> getFactory();

    /**
     * @brief Creates a IGstGenericPlayer object.
     *
     * @param[in] client            : The gstreamer player client.
     * @param[in] decryptionService : The decryption service.
     * @param[in] type              : The media type the gstreamer player shall support.
     * @param[in] videoRequirements : The video requirements for the playback.
     *
     * @retval the new player instance or null on error.
     */
    virtual std::unique_ptr<IGstGenericPlayer>
    createGstGenericPlayer(IGstGenericPlayerClient *client, IDecryptionService &decryptionService, MediaType type,
                           const VideoRequirements &videoRequirements,
                           const std::shared_ptr<IRdkGstreamerUtilsWrapperFactory> &rdkGstreamerUtilsWrapperFactory) = 0;
};

class IGstGenericPlayer
{
public:
    IGstGenericPlayer() = default;
    virtual ~IGstGenericPlayer() = default;

    IGstGenericPlayer(const IGstGenericPlayer &) = delete;
    IGstGenericPlayer &operator=(const IGstGenericPlayer &) = delete;
    IGstGenericPlayer(IGstGenericPlayer &&) = delete;
    IGstGenericPlayer &operator=(IGstGenericPlayer &&) = delete;

    /**
     * @brief Attaches a source to gstreamer.
     *
     * @param[in] mediaSource : The media source.
     *
     */
    virtual void attachSource(const std::unique_ptr<IMediaPipeline::MediaSource> &mediaSource) = 0;

    /**
     * @brief Unattaches a source.
     *
     * @param[in] mediaSourceType : The media source type.
     *
     */
    virtual void removeSource(const MediaSourceType &mediaSourceType) = 0;

    virtual void allSourcesAttached() = 0;

    /**
     * @brief Starts playback of the media.
     *
     * This method is considered to be asychronous and MUST NOT block
     * but should request playback and then return.
     *
     * Once the backend is successfully playing it should notify the
     * media player client of playback state PlaybackState::PLAYING.
     *
     */
    virtual void play() = 0;

    /**
     * @brief Pauses playback of the media.
     *
     * This method is considered to be asychronous and MUST NOT block
     * but should request the playback pause and then return.
     *
     * Once the backend is successfully paused it should notify the
     * media player client of playback state PlaybackState::PAUSED.
     *
     */
    virtual void pause() = 0;

    /**
     * @brief Stops playback of the media.
     *
     * This method is considered to be asychronous and MUST NOT block
     * but should request the playback stop and then return.
     *
     * Once the backend is successfully stopped it should notify the
     * media player client of playback state PlaybackState::STOPPED.
     *
     */
    virtual void stop() = 0;

    /**
     * @brief Sets video geometry
     *
     * @param[in] x      : X position of rectangle on video
     * @param[in] y      : Y position of rectangle on video
     * @param[in] width  : width of rectangle
     * @param[in] height : height of rectangle
     *
     */
    virtual void setVideoGeometry(int x, int y, int width, int height) = 0;

    /**
     * @brief Queues the end of stream notification at the end of the gstreamer buffers.
     *
     * @param[in] type : the media source type to set eos
     *
     */
    virtual void setEos(const firebolt::rialto::MediaSourceType &type) = 0;

    /**
     * @brief Attaches new samples
     *
     * This method is considered to be asychronous and MUST NOT block
     * but should request to attach new sample and then return.
     */
    virtual void attachSamples(const IMediaPipeline::MediaSegmentVector &mediaSegments) = 0;

    /**
     * @brief Attaches new samples
     *
     * This method is considered to be asychronous and MUST NOT block
     * but should request to attach new sample and then return.
     */
    virtual void attachSamples(const std::shared_ptr<IDataReader> &dataReader) = 0;

    /**
     * @brief Set the playback position in nanoseconds.
     *
     * If playback has not started this method sets the start position
     * for playback. If playback has started this method performs a seek.
     *
     * @param[in] position : The playback position in nanoseconds.
     *
     */
    virtual void setPosition(std::int64_t position) = 0;

    /**
     * @brief Get the playback position in nanoseconds.
     *
     * @param[out] position : The playback position in nanoseconds.
     *
     * @retval True on success
     */
    virtual bool getPosition(std::int64_t &position) = 0;

    /**
     * @brief Set the playback rate.
     *
     * @param[in] rate : The playback rate.
     *
     */
    virtual void setPlaybackRate(double rate) = 0;

    /**
     * @brief Requests to render a prerolled frame
     *
     */
    virtual void renderFrame() = 0;

    /**
     * @brief Set level and transition of audio attenuation.
     *        Sets the current volume for the pipeline (0.0 silent -> 1.0 full volume)
     *
     * @param[in] volume : Target volume level (0.0 - 1.0)
     */
    virtual void setVolume(double volume) = 0;

    /**
     * @brief Get current audio level. Fetches the current volume level for the pipeline.
     *
     * @param[out] volume : Current volume level (range 0.0 - 1.0)
     *
     * @retval True on success
     */
    virtual bool getVolume(double &volume) = 0;
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_GST_GENERIC_PLAYER_H_
