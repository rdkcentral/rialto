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

    /**
     * @brief Appends missing audio codecs from GStreamer capabilities to YAML capabilities.
     *
     * Iterates through each rank in GStreamer's audio capabilities and appends any missing
     * codecs to the corresponding rank in the YAML capabilities. If GStreamer has more ranks
     * than YAML, the additional ranks are appended as well.
     *
     * @param[in,out] yamlCaps  : The YAML audio capabilities to be enhanced (modified in-place)
     * @param[in]     gstCaps   : The GStreamer audio capabilities as reference
     */
    void appendMissingAudioCodecsToYaml(firebolt::rialto::common::AudioDecoderCapabilities &yamlCaps,
                                        const firebolt::rialto::common::AudioDecoderCapabilities &gstCaps);

    /**
     * @brief Appends missing audio codecs to a specific rank in the YAML capabilities.
     *
     * Compares each audio codec field in the GStreamer rank with the corresponding YAML rank
     * and appends any missing codecs (those present in GStreamer but not in YAML).
     * Handles multi-profile codecs by merging profiles: keeps all YAML profiles and adds
     * any new profiles from GStreamer.
     *
     * @param[in,out] yamlRank  : The YAML audio decoder capability rank to be enhanced (modified)
     * @param[in]     gstRank   : The GStreamer audio decoder capability rank as reference
     */
    void appendMissingCodecsToAudioRank(firebolt::rialto::common::AudioDecoderCapability &yamlRank,
                                        const firebolt::rialto::common::AudioDecoderCapability &gstRank);

    /**
     * @brief Appends missing video codecs from GStreamer capabilities to YAML capabilities.
     *
     * Iterates through each rank in GStreamer's video capabilities and appends any missing
     * codecs to the corresponding rank in the YAML capabilities. If GStreamer has more ranks
     * than YAML, the additional ranks are appended as well.
     *
     * @param[in,out] yamlCaps  : The YAML video capabilities to be enhanced (modified in-place)
     * @param[in]     gstCaps   : The GStreamer video capabilities as reference
     */
    void appendMissingVideoCodecsToYaml(firebolt::rialto::common::VideoDecoderCapabilities &yamlCaps,
                                        const firebolt::rialto::common::VideoDecoderCapabilities &gstCaps);

    /**
     * @brief Appends missing video codecs to a specific rank in the YAML capabilities.
     *
     * Compares each video codec field (mpeg2, h264, h265, vp8, vp9, av1) in the GStreamer rank
     * with the corresponding YAML rank and appends any missing codecs.
     * Handles profile merging by combining YAML and GStreamer profiles into a single vector.
     *
     * @param[in,out] yamlRank  : The YAML video decoder capability rank to be enhanced (modified)
     * @param[in]     gstRank   : The GStreamer video decoder capability rank as reference
     */
    void appendMissingCodecsToVideoRank(firebolt::rialto::common::VideoDecoderCapability &yamlRank,
                                        const firebolt::rialto::common::VideoDecoderCapability &gstRank);
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_SERVER_INTERNAL_H_
