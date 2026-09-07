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
#include <memory>
#include <mutex>
#include <optional>

namespace firebolt::rialto::server
{
/**
 * @brief MediaCapabilities - Implements IMediaCapabilities interface
 *
 * This class is dedicated to implementing the IMediaCapabilities interface
 * for use within the session server. It handles decoder capabilities queries
 * by orchestrating between ServerManager preload and GStreamer fallback:
 *
 * 0. Preloaded capabilities: YAML already read by ServerManager at startup,
 *    forwarded via IPC setPreloadedCapabilities()
 * 1. GStreamer fallback: If ServerManager preload unavailable, query GStreamer
 *
 * Decision Strategy (Path 0 → Path B):
 * - Path 0: Use ServerManager's preloaded YAML data (highest priority)
 *   ServerManager reads YAML once at startup and provides it here
 * - Path B: Fall back to GStreamer queries only if ServerManager data missing
 *   (No local YAML re-reading - we're in a different process!)
 *
 * This ensures:
 * - Zero YAML re-parsing: ServerManager reads once, all sessions use cached preload
 * - Optimal performance: Preloaded data already available
 * - Robustness: GStreamer fallback if preload missing
 */
class MediaCapabilities : public firebolt::rialto::IMediaCapabilities
{
public:
    /**
     * @brief Constructor
     *
     * @param[in] gstCapabilities GStreamer capabilities handler (fallback for queries)
     *                           Ownership is transferred to MediaCapabilities
     */
    explicit MediaCapabilities(std::unique_ptr<IGstCapabilities> gstCapabilities);

    /**
     * @brief Virtual destructor.
     */
    virtual ~MediaCapabilities() = default;

    /**
     * @brief Gets the supported audio decoder capabilities.
     *
     * Attempts: Path A (ServerManager's YAML) → Path B (GStreamer)
     * If Path 0 preloaded data available, it's already used before calling orchestrator.
     *
     * @retval The supported audio decoder capabilities.
     */
    firebolt::rialto::common::AudioDecoderCapabilities getSupportedAudioCapabilities() override;

    /**
     * @brief Gets the supported video decoder capabilities.
     *
     * Attempts: Path 0 (Preloaded) → Path B (GStreamer)
     * Returns preloaded video if available, otherwise falls back to GStreamer.
     *
     * @retval The supported video decoder capabilities.
     */
    firebolt::rialto::common::VideoDecoderCapabilities getSupportedVideoCapabilities() override;

    /**
     * @brief Sets preloaded capabilities from ServerManager
     *
     * Called when ServerManager provides preloaded YAML capabilities.
     * These are stored here for Path 0 (priority) queries.
     *
     * @param[in] audioCapabilities Optional preloaded audio capabilities
     * @param[in] videoCapabilities Optional preloaded video capabilities
     */
    void
    setPreloadedCapabilities(const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &audioCapabilities,
                             const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &videoCapabilities);

private:
    /**
     * @brief The GStreamer capabilities handler (fallback for queries)
     *        Owned exclusively by MediaCapabilities
     */
    std::unique_ptr<IGstCapabilities> m_gstCapabilities;

    /**
     * @brief Preloaded capabilities from ServerManager (Path 0 - highest priority)
     */
    std::optional<firebolt::rialto::common::AudioDecoderCapabilities> m_preloadedAudioCapabilities;
    std::optional<firebolt::rialto::common::VideoDecoderCapabilities> m_preloadedVideoCapabilities;

    /**
     * @brief Protects access to preloaded capabilities
     */
    std::mutex m_preloadedCapabilitiesMutex;
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_H_
