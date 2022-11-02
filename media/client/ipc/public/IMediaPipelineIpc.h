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

#include <MediaCommon.h>

#include "IMediaPipelineIpcClient.h"

namespace firebolt::rialto::client
{
class IMediaPipelineIpc;

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
                                                                      const VideoRequirements &videoRequirements) = 0;
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
     * @param[in] type      : The type of media.
     * @param[in] caps      : The capabilities of the media type.
     * @param[out] sourceId : The unique id of the media source.
     *
     * @retval true on success.
     */
    virtual bool attachSource(MediaSourceType type, const std::string &caps, int32_t &sourceId) = 0;

    /**
     * @brief Request to remove the source to the server backend.
     *
     * @param[in] sourceId : The unique id of the media source.
     *
     * @retval true on success.
     */
    virtual bool removeSource(int32_t sourceId) = 0;

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
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_MEDIA_PIPELINE_IPC_H_
