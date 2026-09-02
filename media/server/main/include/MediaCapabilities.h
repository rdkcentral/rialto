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

#ifndef FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_H_
#define FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_H_

#include "IGstCapabilities.h"
#include "IMediaCapabilities.h"
#include "YamlCapabilities.h"
#include <memory>
#include <optional>

namespace firebolt::rialto::server
{
/**
 * @brief MediaCapabilities - Implements IMediaCapabilities interface
 *
 * This class is dedicated to implementing the IMediaCapabilities interface
 * for use within the session server. It handles decoder capabilities queries
 * by orchestrating between three sources:
 *
 * 0. Preloaded capabilities: Already-parsed data forwarded by ServerManager over IPC
 * 1. YamlCapabilities: Reads capabilities from YAML configuration files
 *    (provided by ServerManager during boot)
 * 2. GstCapabilities: Queries GStreamer plugins at runtime
 *
 * Decision Strategy (Path 0/A/B):
 * - Path 0: If ServerManager already forwarded parsed capabilities, use them directly
 * - Path A: Otherwise try to get capabilities from YAML
 * - Path B: If YAML fails or is unavailable, fall back to GStreamer queries
 *
 * This ensures:
 * - No redundant YAML re-parsing in each session server process when ServerManager already did it
 * - Optimal performance when ServerManager provides YAML (no runtime queries)
 * - Robustness when YAML is missing (GStreamer fallback)
 */
class MediaCapabilities : public firebolt::rialto::IMediaCapabilities
{
public:
    /**
     * @brief Constructor
     *
     * @param[in] yamlCapabilities The YAML capabilities reader (Path A)
     * @param[in] gstCapabilities The GStreamer capabilities handler (Path B fallback)
     * @param[in] preloadedAudio Optional audio capabilities already forwarded by ServerManager (Path 0)
     * @param[in] preloadedVideo Optional video capabilities already forwarded by ServerManager (Path 0)
     */
    explicit MediaCapabilities(
        const std::shared_ptr<YamlCapabilities> &yamlCapabilities,
        const std::shared_ptr<IGstCapabilities> &gstCapabilities,
        const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &preloadedAudio = std::nullopt,
        const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &preloadedVideo = std::nullopt);

    /**
     * @brief Virtual destructor.
     */
    virtual ~MediaCapabilities() = default;

    /**
     * @brief Gets the supported audio decoder capabilities.
     *
     * Attempts Path A (YAML) first; falls back to Path B (GStreamer) if YAML is unavailable.
     *
     * @retval The supported audio decoder capabilities.
     */
    firebolt::rialto::common::AudioDecoderCapabilities getSupportedAudioCapabilities() override;

    /**
     * @brief Gets the supported video decoder capabilities.
     *
     * Attempts Path A (YAML) first; falls back to Path B (GStreamer) if YAML is unavailable.
     *
     * @retval The supported video decoder capabilities.
     */
    firebolt::rialto::common::VideoDecoderCapabilities getSupportedVideoCapabilities() override;

private:
    /**
     * @brief The YAML capabilities handler (Path A - primary source)
     */
    std::shared_ptr<YamlCapabilities> m_yamlCapabilities;

    /**
     * @brief The GStreamer capabilities handler (Path B - fallback)
     */
    std::shared_ptr<IGstCapabilities> m_gstCapabilities;

    /**
     * @brief Audio capabilities already forwarded by ServerManager (Path 0 - highest priority)
     */
    std::optional<firebolt::rialto::common::AudioDecoderCapabilities> m_preloadedAudio;

    /**
     * @brief Video capabilities already forwarded by ServerManager (Path 0 - highest priority)
     */
    std::optional<firebolt::rialto::common::VideoDecoderCapabilities> m_preloadedVideo;
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_H_
