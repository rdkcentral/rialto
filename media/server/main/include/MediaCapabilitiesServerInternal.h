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

#ifndef FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_SERVER_INTERNAL_H_
#define FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_SERVER_INTERNAL_H_

#include "IMediaCapabilitiesServerInternal.h"
#include <memory>
#include <mutex>
#include <optional>

namespace firebolt::rialto::server
{
class IGstCapabilities;
/**
 * @brief Server-internal factory implementation.
 *
 * Used by RialtoServer to create MediaCapabilitiesServerInternal instances.
 * Preloaded capabilities are not supplied here - the caller sets them later via
 * IMediaCapabilitiesServerInternal::setPreloadedCapabilities() once ServerManager's
 * configuration arrives.
 */
class MediaCapabilitiesServerInternalFactory : public IMediaCapabilitiesServerInternalFactory
{
public:
    MediaCapabilitiesServerInternalFactory() = default;
    ~MediaCapabilitiesServerInternalFactory() override = default;

    std::shared_ptr<IMediaCapabilitiesServerInternal> createMediaCapabilitiesServerInternal() const override;
};

/**
 * @brief The definition of the MediaCapabilitiesServerInternal.
 *
 * Orchestrates capability queries between preloaded YAML data (Path 0, highest priority,
 * set by ServerManager via setPreloadedCapabilities()) and GStreamer discovery (Path B,
 * fallback used when no preloaded data is available).
 */
class MediaCapabilitiesServerInternal : public IMediaCapabilitiesServerInternal
{
public:
    /**
     * @brief Constructor
     *
     * @param[in] gstCapabilities GStreamer capabilities handler (fallback for queries)
     *                           Ownership is transferred to MediaCapabilitiesServerInternal
     */
    explicit MediaCapabilitiesServerInternal(std::unique_ptr<IGstCapabilities> gstCapabilities);

    /**
     * @brief Virtual destructor.
     */
    ~MediaCapabilitiesServerInternal() override = default;

    firebolt::rialto::common::AudioDecoderCapabilities getSupportedAudioCapabilities() override;

    firebolt::rialto::common::VideoDecoderCapabilities getSupportedVideoCapabilities() override;

    void setPreloadedCapabilities(
        const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &audioCapabilities,
        const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &videoCapabilities) override;

private:
    /**
     * @brief The GStreamer capabilities handler (fallback for queries)
     *        Owned exclusively by MediaCapabilitiesServerInternal
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

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_SERVER_INTERNAL_H_
