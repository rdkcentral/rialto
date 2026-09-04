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

#include "MediaCapabilities.h"
#include "IGstCapabilities.h"
#include "RialtoServerLogging.h"
#include <stdexcept>

namespace firebolt::rialto::server
{
MediaCapabilities::MediaCapabilities(
    const std::shared_ptr<IGstCapabilities> &gstCapabilities,
    const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &preloadedAudio,
    const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &preloadedVideo)
    : m_gstCapabilities{gstCapabilities}, m_preloadedAudio{preloadedAudio}, m_preloadedVideo{preloadedVideo}
{
    RIALTO_SERVER_LOG_DEBUG("MediaCapabilities: constructor - uses ServerManager preload + GStreamer fallback");
    if (!m_gstCapabilities)
    {
        throw std::runtime_error("GstCapabilities is required");
    }
}

firebolt::rialto::common::AudioDecoderCapabilities MediaCapabilities::getSupportedAudioCapabilities()
{
    // Path 0: Use ServerManager's preloaded YAML (highest priority)
    if (m_preloadedAudio.has_value())
    {
        RIALTO_SERVER_LOG_DEBUG("MediaCapabilities: Audio from ServerManager preload (Path 0) - success");
        return *m_preloadedAudio;
    }

    RIALTO_SERVER_LOG_INFO("MediaCapabilities: ServerManager preload unavailable, falling back to GStreamer (Path B)");

    // Path B: GStreamer fallback (only if preload missing)
    return m_gstCapabilities->getSupportedAudioCapabilities();
}

firebolt::rialto::common::VideoDecoderCapabilities MediaCapabilities::getSupportedVideoCapabilities()
{
    // Path 0: Use ServerManager's preloaded YAML (highest priority)
    if (m_preloadedVideo.has_value())
    {
        RIALTO_SERVER_LOG_DEBUG("MediaCapabilities: Video from ServerManager preload (Path 0) - success");
        return *m_preloadedVideo;
    }

    RIALTO_SERVER_LOG_INFO("MediaCapabilities: ServerManager preload unavailable, falling back to GStreamer (Path B)");

    // Path B: GStreamer fallback (only if preload missing)
    return m_gstCapabilities->getSupportedVideoCapabilities();
}

} // namespace firebolt::rialto::server
