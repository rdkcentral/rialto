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

#ifndef FIREBOLT_RIALTO_SERVER_I_MEDIA_CAPABILITIES_SERVER_INTERNAL_H_
#define FIREBOLT_RIALTO_SERVER_I_MEDIA_CAPABILITIES_SERVER_INTERNAL_H_

#include "IMediaCapabilities.h"
#include <memory>
#include <optional>

namespace firebolt::rialto::server
{
/**
 * @brief The definition of the IMediaCapabilitiesServerInternal interface.
 *
 * Extends IMediaCapabilities with the ability to accept preloaded (YAML) capabilities
 * supplied by ServerManager at startup. This interface should be implemented by Rialto Server only.
 *
 * Following the pattern: IMediaPipeline vs IMediaPipelineServerInternal
 */
class IMediaCapabilitiesServerInternal : public firebolt::rialto::IMediaCapabilities
{
public:
    IMediaCapabilitiesServerInternal() = default;
    virtual ~IMediaCapabilitiesServerInternal() = default;

    /**
     * @brief Sets preloaded capabilities from ServerManager
     *
     * Called when ServerManager provides preloaded YAML capabilities.
     * These take priority over GStreamer discovery when present.
     *
     * @param[in] audioCapabilities Optional preloaded audio capabilities
     * @param[in] videoCapabilities Optional preloaded video capabilities
     */
    virtual void setPreloadedCapabilities(
        const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &audioCapabilities,
        const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &videoCapabilities) = 0;
};

/**
 * @brief Server-internal factory for MediaCapabilities
 *
 * This interface provides server-specific factory methods for creating MediaCapabilities.
 * It is separate from the public IMediaCapabilitiesFactory to maintain clean separation:
 *
 * - IMediaCapabilitiesFactory (public): Used by clients, has no preloaded parameters
 * - IMediaCapabilitiesServerInternalFactory (server): ONLY used by RialtoServer,
 *   provides method to create factories with preloaded capabilities
 *
 * Following the pattern: IMediaPipeline vs IMediaPipelineServerInternal
 */
class IMediaCapabilitiesServerInternalFactory
{
public:
    IMediaCapabilitiesServerInternalFactory() = default;
    virtual ~IMediaCapabilitiesServerInternalFactory() = default;

    /**
     * @brief Gets the IMediaCapabilitiesServerInternalFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IMediaCapabilitiesServerInternalFactory> createFactory();

    /**
     * @brief Creates an IMediaCapabilitiesServerInternal instance.
     *
     * This method is SERVER-INTERNAL ONLY. It creates the orchestrator that handles
     * preloaded capabilities and GStreamer fallback.
     *
     * @retval the orchestrator instance or null on error.
     */
    virtual std::shared_ptr<IMediaCapabilitiesServerInternal> createMediaCapabilitiesServerInternal() const = 0;
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_MEDIA_CAPABILITIES_SERVER_INTERNAL_H_
