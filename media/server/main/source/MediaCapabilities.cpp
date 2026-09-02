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
#include "YamlCapabilities.h"
#include <stdexcept>

namespace firebolt::rialto::server
{
MediaCapabilities::MediaCapabilities(
    const std::shared_ptr<YamlCapabilities> &yamlCapabilities, const std::shared_ptr<IGstCapabilities> &gstCapabilities,
    const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &preloadedAudio,
    const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &preloadedVideo)
    : m_yamlCapabilities{yamlCapabilities}, m_gstCapabilities{gstCapabilities}, m_preloadedAudio{preloadedAudio},
      m_preloadedVideo{preloadedVideo}
{
    RIALTO_SERVER_LOG_DEBUG("MediaCapabilities: constructor - orchestrates between YAML and GStreamer sources");
    if (!m_yamlCapabilities)
    {
        throw std::runtime_error("YamlCapabilities is required");
    }
    if (!m_gstCapabilities)
    {
        throw std::runtime_error("GstCapabilities is required");
    }
}

firebolt::rialto::common::AudioDecoderCapabilities MediaCapabilities::getSupportedAudioCapabilities()
{
    if (m_preloadedAudio.has_value())
    {
        RIALTO_SERVER_LOG_DEBUG("MediaCapabilities: Audio capabilities from ServerManager preload (Path 0) - success");
        return *m_preloadedAudio;
    }

    RIALTO_SERVER_LOG_DEBUG("MediaCapabilities: getSupportedAudioCapabilities - trying YAML first (Path A)");

    // Path A: Try to get audio capabilities from YAML
    firebolt::rialto::common::AudioDecoderCapabilities audioCapabilities;
    auto status = m_yamlCapabilities->getAudioDecoderCapabilities(audioCapabilities);

    if (status == firebolt::rialto::common::DecoderCapabilitiesStatus::OK)
    {
        RIALTO_SERVER_LOG_DEBUG("MediaCapabilities: Audio capabilities from YAML (Path A) - success");
        return audioCapabilities;
    }

    RIALTO_SERVER_LOG_INFO("MediaCapabilities: YAML audio capabilities unavailable (status: %d), "
                           "falling back to GStreamer (Path B)",
                           static_cast<int>(status));

    // Path B: Fall back to GStreamer queries (returns empty if not available)
    return m_gstCapabilities->getSupportedAudioCapabilities();
}

firebolt::rialto::common::VideoDecoderCapabilities MediaCapabilities::getSupportedVideoCapabilities()
{
    if (m_preloadedVideo.has_value())
    {
        RIALTO_SERVER_LOG_DEBUG("MediaCapabilities: Video capabilities from ServerManager preload (Path 0) - success");
        return *m_preloadedVideo;
    }

    RIALTO_SERVER_LOG_DEBUG("MediaCapabilities: getSupportedVideoCapabilities - trying YAML first (Path A)");

    // Path A: Try to get video capabilities from YAML
    firebolt::rialto::common::VideoDecoderCapabilities videoCapabilities;
    auto status = m_yamlCapabilities->getVideoDecoderCapabilities(videoCapabilities);

    if (status == firebolt::rialto::common::DecoderCapabilitiesStatus::OK)
    {
        RIALTO_SERVER_LOG_DEBUG("MediaCapabilities: Video capabilities from YAML (Path A) - success");
        return videoCapabilities;
    }

    RIALTO_SERVER_LOG_INFO("MediaCapabilities: YAML video capabilities unavailable (status: %d), "
                           "falling back to GStreamer (Path B)",
                           static_cast<int>(status));

    // Path B: Fall back to GStreamer queries (returns empty if not available)
    return m_gstCapabilities->getSupportedVideoCapabilities();
}

} // namespace firebolt::rialto::server
