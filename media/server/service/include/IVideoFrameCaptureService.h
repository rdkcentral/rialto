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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_I_VIDEO_FRAME_CAPTURE_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_I_VIDEO_FRAME_CAPTURE_SERVICE_H_

#include "IVideoFrameCapture.h"

#include <cstdint>
#include <functional>
#include <optional>

namespace firebolt::rialto::server::service
{
/**
 * @brief Identifies one client IPC connection. Assigned by the IPC module; opaque to the service.
 */
using VideoFrameCaptureClientId = std::uint64_t;

/**
 * @brief Result of a capture registration.
 */
enum class VideoFrameCaptureServiceStatus
{
    OK,
    UNSUPPORTED,     /**< The capture HAL library is not available. */
    INVALID_ARGUMENT /**< Unknown graphics API. */
};

/**
 * @brief Registers Video Frame Capture objects per client connection.
 *
 * Each connection has its own registry. Capture IDs are non-zero, unique on their connection and never reused
 * on it; an ID from another connection, or one that was destroyed, is not found.
 */
class IVideoFrameCaptureService
{
public:
    IVideoFrameCaptureService() = default;
    virtual ~IVideoFrameCaptureService() = default;

    IVideoFrameCaptureService(const IVideoFrameCaptureService &) = delete;
    IVideoFrameCaptureService(IVideoFrameCaptureService &&) = delete;
    IVideoFrameCaptureService &operator=(const IVideoFrameCaptureService &) = delete;
    IVideoFrameCaptureService &operator=(IVideoFrameCaptureService &&) = delete;

    /**
     * @brief Registers a capture for a client. Allocates no session or buffers.
     *
     * @param[in]  clientId  : The client connection.
     * @param[in]  api       : The graphics API, or nullopt if the client sent an unknown one.
     * @param[out] captureId : The new capture ID, set when OK is returned.
     */
    virtual VideoFrameCaptureServiceStatus
    createCapture(VideoFrameCaptureClientId clientId, std::optional<GraphicsAPIType> api, std::uint32_t &captureId) = 0;

    /**
     * @brief Removes a capture registration of a client.
     *
     * @retval false if the client has no capture with this ID.
     */
    virtual bool destroyCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId) = 0;

    /**
     * @brief Checks whether a client has a capture with this ID.
     */
    virtual bool hasCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId) const = 0;

    /**
     * @brief Removes every capture registration of a client, when its connection closes.
     */
    virtual void removeClient(VideoFrameCaptureClientId clientId) = 0;

    /**
     * @brief Binds a client's capture to a pipeline session.
     *
     * @retval false if the client has no such capture or it is already bound to any session.
     */
    virtual bool bindCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId, int sessionId) = 0;

    /**
     * @brief Unbinds a client's capture from its session. Does nothing if it is not bound.
     */
    virtual void unbindCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId) = 0;

    /**
     * @brief Called with the session and capture ID when a bound capture is destroyed or its client disconnects.
     *
     * Runs on the calling thread with no lock of this service held.
     */
    using BoundCaptureRemovedHandler = std::function<void(int sessionId, std::uint32_t captureId)>;

    /**
     * @brief Sets the handler for bound captures that are removed (implicit detach).
     */
    virtual void setBoundCaptureRemovedHandler(const BoundCaptureRemovedHandler &handler) = 0;
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_I_VIDEO_FRAME_CAPTURE_SERVICE_H_
