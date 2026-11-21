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

#ifndef FIREBOLT_RIALTO_CLIENT_I_VIDEO_FRAME_CAPTURE_IPC_H_
#define FIREBOLT_RIALTO_CLIENT_I_VIDEO_FRAME_CAPTURE_IPC_H_

#include "IVideoFrameCapture.h"

#include <cstdint>
#include <memory>

namespace firebolt::rialto::client
{
class IVideoFrameCaptureIpc;

/**
 * @brief IVideoFrameCaptureIpc factory class.
 */
class IVideoFrameCaptureIpcFactory
{
public:
    IVideoFrameCaptureIpcFactory() = default;
    virtual ~IVideoFrameCaptureIpcFactory() = default;

    /**
     * @brief Gets the IVideoFrameCaptureIpcFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IVideoFrameCaptureIpcFactory> createFactory();

    /**
     * @brief Creates the IPC object for the VideoFrameCaptureModule.
     *
     * @retval the object, or null if the IPC channel cannot be attached (server unreachable).
     */
    virtual std::shared_ptr<IVideoFrameCaptureIpc> createVideoFrameCaptureIpc() const = 0;
};

/**
 * @brief Client side of the private VideoFrameCaptureModule.
 */
class IVideoFrameCaptureIpc
{
public:
    IVideoFrameCaptureIpc() = default;
    virtual ~IVideoFrameCaptureIpc() = default;

    IVideoFrameCaptureIpc(const IVideoFrameCaptureIpc &) = delete;
    IVideoFrameCaptureIpc &operator=(const IVideoFrameCaptureIpc &) = delete;
    IVideoFrameCaptureIpc(IVideoFrameCaptureIpc &&) = delete;
    IVideoFrameCaptureIpc &operator=(IVideoFrameCaptureIpc &&) = delete;

    /**
     * @brief Registers a capture with the server.
     *
     * @param[in]  api       : The graphics API of the capture.
     * @param[out] captureId : The server's capture ID, set on success.
     *
     * @retval false if the server does not support capture, rejects the request, or cannot be reached.
     */
    virtual bool createVideoFrameCapture(GraphicsAPIType api, std::uint32_t &captureId) = 0;

    /**
     * @brief Removes a capture registration.
     *
     * @retval false if the call failed.
     */
    virtual bool destroyVideoFrameCapture(std::uint32_t captureId) = 0;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_VIDEO_FRAME_CAPTURE_IPC_H_
