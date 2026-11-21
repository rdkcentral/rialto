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

#ifndef FIREBOLT_RIALTO_CLIENT_CAPTURE_CORE_H_
#define FIREBOLT_RIALTO_CLIENT_CAPTURE_CORE_H_

#include "FrameLease.h"
#include "IFrameImporter.h"
#include "IVideoFrameCapture.h"
#include "IVideoFrameCaptureIpc.h"
#include "ImportCache.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

namespace firebolt::rialto::client
{
/**
 * @brief The graphics-independent part of a capture: acquisition, the frame lease and the import cache.
 *
 * A GL or Vulkan capture object owns one CaptureCore and an IFrameImporter for its graphics API.
 */
/**
 * @brief A frame of any graphics API and whether its selection changed.
 */
struct CaptureFrameAcquisition
{
    bool changed{false};
    std::shared_ptr<IVideoFrame> frame;
};

class CaptureCore : public std::enable_shared_from_this<CaptureCore>
{
public:
    /**
     * @brief Makes the application's frame object from a description, its import and its lease.
     */
    using FrameMaker = std::function<std::shared_ptr<IVideoFrame>(const CaptureFrameDescription &description,
                                                                  const std::shared_ptr<ImportCacheEntry> &import,
                                                                  std::unique_ptr<FrameLease> lease)>;

    /**
     * @param[in] frameMaker : Makes the frames; null makes GL frames (VideoFrame).
     */
    CaptureCore(const std::shared_ptr<IVideoFrameCaptureIpc> &ipc, std::uint32_t captureId,
                const std::shared_ptr<IFrameImporter> &importer, const FrameMaker &frameMaker = nullptr);
    ~CaptureCore();

    CaptureCore(const CaptureCore &) = delete;
    CaptureCore &operator=(const CaptureCore &) = delete;

    /** The server's ID for this capture. */
    std::uint32_t captureId() const { return m_captureId; }

    /**
     * @brief Acquires the current frame (proposal 3.4).
     *
     * A new selection returns changed = true and becomes the current frame. NO_NEW_FRAME, NO_RESOURCES and an IPC
     * failure return the current frame unchanged. NO_FRAME, FATAL_ERROR, UNSUPPORTED and INVALID_ARGUMENT clear it.
     */
    GLFrameAcquisition acquireCurrentFrame();

    /**
     * @brief As acquireCurrentFrame(), for frames of any graphics API.
     */
    CaptureFrameAcquisition acquireFrame();

    /** Number of reusable imports kept, for tests and diagnostics. */
    std::size_t cacheSize();

private:
    /** The acquisition itself. Must be called with m_acquireMutex held. */
    CaptureFrameAcquisition acquireLocked();

    /** Imports a new frame. Must be called with m_acquireMutex held. @retval null if it could not be imported. */
    std::shared_ptr<IVideoFrame> importNewFrame(CaptureAcquireResult &result);

    std::shared_ptr<IVideoFrameCaptureIpc> m_ipc;
    const std::uint32_t m_captureId;
    std::shared_ptr<IFrameImporter> m_importer;

    std::mutex m_acquireMutex;                       /**< Serializes acquisitions, the current frame and the cache. */
    std::shared_ptr<CaptureLeaseState> m_leaseState; /**< Shared with frames, which may outlive the capture. */
    FrameMaker m_frameMaker;
    std::shared_ptr<IVideoFrame> m_currentFrame;
    ImportCache m_cache;
    std::uint64_t m_period{0}; /**< Invalidation period; part of every cache key. */
    bool m_poolRequested{false};
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_CAPTURE_CORE_H_
