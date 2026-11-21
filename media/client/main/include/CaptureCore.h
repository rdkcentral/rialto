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

#include "IFrameImporter.h"
#include "IVideoFrameCapture.h"
#include "IVideoFrameCaptureIpc.h"

#include <cstdint>
#include <memory>
#include <mutex>

namespace firebolt::rialto::client
{
/**
 * @brief The graphics-independent part of a capture: acquisition, the frame lease and the import cache.
 *
 * A GL or Vulkan capture object owns one CaptureCore and an IFrameImporter for its graphics API.
 */
class CaptureCore : public std::enable_shared_from_this<CaptureCore>
{
public:
    CaptureCore(const std::shared_ptr<IVideoFrameCaptureIpc> &ipc, std::uint32_t captureId,
                const std::shared_ptr<IFrameImporter> &importer);
    ~CaptureCore();

    CaptureCore(const CaptureCore &) = delete;
    CaptureCore &operator=(const CaptureCore &) = delete;

    /** The server's ID for this capture. */
    std::uint32_t captureId() const { return m_captureId; }

    /** Acquires the current frame (proposal 3.4). */
    GLFrameAcquisition acquireCurrentFrame();

private:
    std::shared_ptr<IVideoFrameCaptureIpc> m_ipc;
    const std::uint32_t m_captureId;
    std::shared_ptr<IFrameImporter> m_importer;

    std::mutex m_acquireMutex; /**< Serializes acquisitions, the current frame and the cache. */
    std::mutex m_releaseMutex; /**< Serializes slot releases, which may come from any thread. */
    std::shared_ptr<IGLVideoFrame> m_currentFrame;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_CAPTURE_CORE_H_
