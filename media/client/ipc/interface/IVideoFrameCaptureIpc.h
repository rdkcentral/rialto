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
#include <optional>
#include <vector>

namespace firebolt::rialto::client
{
/**
 * @brief The server's answer to a capabilities query, before graphics-specific limits are added.
 */
struct CaptureCapabilitiesReply
{
    std::uint32_t maxConcurrentFrames{0};
    std::uint32_t maxResourcesPerFrame{0};
    std::uint32_t supportsProtectedContent{0};
    std::uint32_t supportsHdr{0};
    std::vector<std::uint32_t> drmFourccs;
    std::vector<std::uint64_t> drmModifiers;
};

/**
 * @brief One DMA-BUF object of an acquired frame.
 */
struct CaptureObject
{
    std::uint64_t objectId{0};
    std::uint64_t sizeBytes{0};
};

/**
 * @brief One DRM plane of an acquired frame and where it is stored.
 */
struct CapturePlane
{
    std::uint32_t planeIndex{0};
    std::uint64_t objectId{0};
    std::uint64_t offsetBytes{0};
    std::uint64_t lengthBytes{0};
    std::uint32_t strideBytes{0};
};

/**
 * @brief Effective colour interpretation of a frame, as the wire enum values (matrix, range, siting).
 */
struct CaptureColour
{
    std::uint32_t matrix{0};
    std::uint32_t range{0};
    std::uint32_t horizontalSiting{0};
    std::uint32_t verticalSiting{0};

    bool operator==(const CaptureColour &other) const
    {
        return matrix == other.matrix && range == other.range && horizontalSiting == other.horizontalSiting &&
               verticalSiting == other.verticalSiting;
    }
};

/**
 * @brief Everything the server sends about one acquired frame, apart from its descriptors.
 */
struct CaptureFrameDescription
{
    std::uint32_t poolId{0};
    std::uint32_t slotIndex{0};
    std::uint64_t acquisitionToken{0};
    std::int64_t presentationTimeNs{0};
    std::uint32_t visibleWidth{0};
    std::uint32_t visibleHeight{0};
    std::uint32_t drmFormat{0};
    std::uint64_t drmModifier{0};
    std::uint32_t backingWidth{0};
    std::uint32_t backingHeight{0};
    std::vector<CaptureObject> objects;
    std::vector<CapturePlane> planes;
    CaptureColour colour;
    bool dmaBufObjectsFlushed{false};
};

/**
 * @brief A received descriptor for one DMA-BUF object. The receiver owns fd until it is closed or consumed.
 */
struct CaptureObjectFd
{
    std::uint64_t objectId{0};
    int fd{-1};
};

/**
 * @brief The structural pool description: identity and admitted slot count.
 */
struct CapturePoolDescription
{
    std::uint32_t poolId{0};
    std::uint32_t slotCount{0};
};

/**
 * @brief Result of an acquisition call.
 */
enum class CaptureAcquireStatus
{
    OK,
    NO_NEW_FRAME,
    NO_FRAME,
    UNSUPPORTED,
    INVALID_ARGUMENT,
    NO_RESOURCES,
    FATAL_ERROR,
    IPC_FAILURE /**< The call itself failed (server gone, channel down). */
};

/**
 * @brief An acquisition: status, and for OK the frame description, its descriptors and any updated pool.
 */
struct CaptureAcquireResult
{
    CaptureAcquireStatus status{CaptureAcquireStatus::IPC_FAILURE};
    CaptureFrameDescription description;
    std::vector<CaptureObjectFd> fds;
    std::optional<CapturePoolDescription> pool;
};

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

    /**
     * @brief Acquires the current frame of a capture.
     *
     * For OK the result carries the frame description, one received descriptor per distinct object (owned by the
     * caller), and the updated pool description if the server sent one. IPC_FAILURE means the call itself failed.
     */
    virtual CaptureAcquireResult acquireCurrentFrame(std::uint32_t captureId) = 0;

    /**
     * @brief Releases the frame lock identified by pool, slot and acquisition token.
     *
     * @retval true if the server released it.
     */
    virtual bool releaseFrame(std::uint32_t captureId, std::uint32_t poolId, std::uint32_t slotIndex,
                              std::uint64_t acquisitionToken) = 0;

    /**
     * @brief Gets the capture's admitted pool description.
     *
     * @retval false if the call failed or the capture has no pool.
     */
    virtual bool getPool(std::uint32_t captureId, CapturePoolDescription &pool) = 0;

    /**
     * @brief Queries the capabilities of the capture's admitted session.
     *
     * @param[out] reply : Set only on OK.
     * @retval The server's status, mapped one to one; FATAL_ERROR if the call itself failed.
     */
    virtual VideoFrameCaptureStatus getCapabilities(std::uint32_t captureId, GraphicsAPIType api,
                                                    CaptureCapabilitiesReply &reply) = 0;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_VIDEO_FRAME_CAPTURE_IPC_H_
