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

#ifndef FIREBOLT_RIALTO_SERVER_I_GST_PLAYER_H_
#define FIREBOLT_RIALTO_SERVER_I_GST_PLAYER_H_

#include <MediaCommon.h>
#include <memory>
#include <stdint.h>
#include <string>

#include "IDataReader.h"
#include "IDecryptionService.h"
#include "IGstPlayerClient.h"
#include "IMediaPipeline.h"

namespace firebolt::rialto::server
{
class IGstPlayer;

/**
 * @brief IGstPlayer factory class, returns a concrete implementation of IGstPlayer
 */
class IGstPlayerFactory
{
public:
    IGstPlayerFactory() = default;
    virtual ~IGstPlayerFactory() = default;

    /**
     * @brief Gets the IGstPlayerFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IGstPlayerFactory> getFactory();

    /**
     * @brief Creates a IGstPlayer object.
     *
     * @param[in] client            : The gstreamer player client.
     * @param[in] decryptionService : The decryption service.
     * @param[in] type              : The media type the gstreamer player shall support.
     * @param[in] videoRequirements : The video requirements for the playback.
     *
     * @retval the new player instance or null on error.
     */
    virtual std::unique_ptr<IGstPlayer> createGstPlayer(IGstPlayerClient *client, IDecryptionService &decryptionService,
                                                        MediaType type, const VideoRequirements &videoRequirements) = 0;
};

class IGstPlayer
{
public:
    IGstPlayer() = default;
    virtual ~IGstPlayer() = default;

    IGstPlayer(const IGstPlayer &) = delete;
    IGstPlayer &operator=(const IGstPlayer &) = delete;
    IGstPlayer(IGstPlayer &&) = delete;
    IGstPlayer &operator=(IGstPlayer &&) = delete;

    /**
     * @brief Initialise gstreamer player.
     *
     * Gstreamer should be initalised at the start of the program.
     * Gstreamer shall be passed the pointers to the main argc and argv
     * variables so that it can process its own command line options.
     *
     * @param[in] argc    : The count of command line arguments.
     * @param[in] argv    : Vector of C strings each containing a command line argument.
     */
    static bool initalise(int argc, char **argv);

    /**
     * @brief Attaches a source to gstreamer.
     *
     * @param[in] mediaSource : The media source.
     *
     */
    virtual void attachSource(std::unique_ptr<IMediaPipeline::MediaSource> &mediaSource) = 0;

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
     * Once the backend is successfully playing it should notify the
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
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_GST_PLAYER_H_
