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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_VIDEO_FRAME_CAPTURE_REGISTRY_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_VIDEO_FRAME_CAPTURE_REGISTRY_H_

#include "IVideoFrameCapture.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>

namespace firebolt::rialto::server::service
{
/**
 * @brief What the server knows about one registered capture. Later changes add its capture sessions.
 */
struct VideoFrameCaptureEntry
{
    GraphicsAPIType api;
    std::optional<int> attachedSessionId; /**< The pipeline session the capture is attached to, if any. */
};

/**
 * @brief The captures registered on one client connection.
 *
 * IDs come from a counter starting at 1, so they are non-zero and never reused on the connection. Not thread
 * safe; the owning service serializes access.
 */
class VideoFrameCaptureRegistry
{
public:
    /** Registers a capture and returns its new ID. */
    std::uint32_t add(GraphicsAPIType api);

    /** Removes a capture. @retval false if there is no capture with this ID. */
    bool remove(std::uint32_t captureId);

    /** @retval the entry, or nullptr if there is no capture with this ID. */
    const VideoFrameCaptureEntry *find(std::uint32_t captureId) const;

    /** Binds a capture to a pipeline session. @retval false if unknown or already bound to any session. */
    bool bind(std::uint32_t captureId, int sessionId);

    /** Unbinds a capture. @retval false if unknown or not bound. */
    bool unbind(std::uint32_t captureId);

    /** @retval the IDs and sessions of every bound capture. */
    std::map<std::uint32_t, int> boundCaptures() const;

    /** Number of registered captures. */
    std::size_t size() const { return m_entries.size(); }

private:
    std::uint32_t m_nextId{1};
    std::map<std::uint32_t, VideoFrameCaptureEntry> m_entries;
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_VIDEO_FRAME_CAPTURE_REGISTRY_H_
