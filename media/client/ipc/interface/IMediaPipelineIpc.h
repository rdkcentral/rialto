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

#ifndef FIREBOLT_RIALTO_CLIENT_I_MEDIA_PIPELINE_IPC_H_
#define FIREBOLT_RIALTO_CLIENT_I_MEDIA_PIPELINE_IPC_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "IMediaPipeline.h"
#include "IMediaPipelineIpcClient.h"
#include "MediaCommon.h"

namespace firebolt::rialto::client
{
class IMediaPipelineIpc;
class IIpcClient;

/**
 * @brief IMediaPipelineIpc factory class, returns a concrete implementation of IMediaPipelineIpc
 */
class IMediaPipelineIpcFactory
{
public:
    IMediaPipelineIpcFactory() = default;
    virtual ~IMediaPipelineIpcFactory() = default;

    /**
     * @brief Gets the IMediaPipelineIpcFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IMediaPipelineIpcFactory> getFactory();

    /**
     * @brief Creates a IMediaPipelineIpc object.
     *
     * @param[in] client            : The Rialto ipc media player client.
     * @param[in] videoRequirements : The video decoder requirements for the MediaPipeline session.
     *
     * @retval the new media player ipc instance or null on error.
     */
    virtual std::unique_ptr<IMediaPipelineIpc> createMediaPipelineIpc(IMediaPipelineIpcClient *client,
                                                                      const VideoRequirements &videoRequirements,
                                                                      std::weak_ptr<IIpcClient> ipcClient = {}) = 0;
};

/**
 * @brief The definition of the IMediaPipelineIpc interface.
 *
 * This interface defines the media player ipc APIs that are used to communicate with the Rialto server.
 */
class IMediaPipelineIpc
{
public:
    IMediaPipelineIpc() = default;
    virtual ~IMediaPipelineIpc() = default;

    IMediaPipelineIpc(const IMediaPipelineIpc &) = delete;
    IMediaPipelineIpc &operator=(const IMediaPipelineIpc &) = delete;
    IMediaPipelineIpc(IMediaPipelineIpc &&) = delete;
    IMediaPipelineIpc &operator=(IMediaPipelineIpc &&) = delete;

    /**
     * @brief Request to attach the source to the server backend.
     *
     * @param[in] source    : The source.
     * @param[out] sourceId : The unique id of the media source.
     *
     * @retval true on success.
     */
    virtual bool attachSource(const std::unique_ptr<IMediaPipeline::MediaSource> &source, int32_t &sourceId) = 0;

    /**
     * @brief Request to remove the source to the server backend.
     *
     * @param[in] sourceId : The unique id of the media source.
     *
     * @retval true on success.
     */
    virtual bool removeSource(int32_t sourceId) = 0;

    virtual bool allSourcesAttached() = 0;

    /**
     * @brief Request to load the media pipeline.
     *
     * @param[in] type     : The media type.
     * @param[in] mimeType : The MIME type.
     * @param[in] url      : The URL.
     *
     * @retval true on success.
     */
    virtual bool load(MediaType type, const std::string &mimeType, const std::string &url) = 0;

    /**
     * @brief Request to set the coordinates of the video window.
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
     * @brief Request play on the playback session.
     *
     * @retval true on success.
     */
    virtual bool play() = 0;

    /**
     * @brief Request pause on the playback session.
     *
     * @retval true on success.
     */
    virtual bool pause() = 0;

    /**
     * @brief Request stop on the playback session.
     *
     * @retval true on success.
     */
    virtual bool stop() = 0;

    /**
     * @brief Notify server that the data has been written to the shared memory.
     *
     * @param[in] status    : The status.
     * @param[in] requestId : The Need data request id.
     *
     * @retval true on success.
     */
    virtual bool haveData(MediaSourceStatus status, uint32_t numFrames, uint32_t requestId) = 0;

    /**
     * @brief Request new playback position.
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
     * @brief Request new playback rate.
     *
     * @param[in] rate : The playback rate.
     *
     * @retval true on success.
     */
    virtual bool setPlaybackRate(double rate) = 0;

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

    /**
     * @brief Set mute status of pipeline.
     *
     * Change mute status of media source
     *
     * @param[in] sourceId Source, which mute status should be changed
     * @param[in] mute   Desired mute state, true=muted, false=not muted
     *
     * @retval true on success false otherwise
     */
    virtual bool setMute(int32_t sourceId, bool mute) = 0;

    /**
     * @brief Get current mute status of the media source
     *
     * @param[in] sourceId Source, which mute status should be fetched
     * @param[out] mute   Current mute state
     *
     * @retval true on success false otherwise
     */
    virtual bool getMute(int32_t sourceId, bool &mute) = 0;

    /**
     * @brief Flushes a source.
     *
     * This method is called by Rialto Client to flush out all queued data for a media source stream.
     *
     * @param[in] sourceId : The source id. Value should be set to the MediaSource.id returned after attachSource()
     * @param[in] resetTime : True if time should be reset
     *
     * @retval true on success.
     */
    virtual bool flush(int32_t sourceId, bool resetTime) = 0;

    /**
     * @brief Set the source position in nanoseconds.
     *
     * This method sets the start position for a source.
     *
     * @param[in] sourceId  : The source id. Value should be set to the MediaSource.id returned after attachSource()
     * @param[in] position : The position in nanoseconds.
     *
     * @retval true on success.
     */
    virtual bool setSourcePosition(int32_t sourceId, int64_t position) = 0;

    /**
     * @brief Process audio gap
     *
     * This method handles audio gap in order to avoid audio pops during transitions.
     *
     * @param[in] position : Audio pts fade pts value
     * @param[in] duration : Audio pts fade duration
     * @param[in] level    : Audio pts fade target level [0-1]
     *
     * @retval true on success.
     */
    virtual bool processAudioGap(int64_t position, uint32_t duration, uint32_t level) = 0;
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_MEDIA_PIPELINE_IPC_H_
