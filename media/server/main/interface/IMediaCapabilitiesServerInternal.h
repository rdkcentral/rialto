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
     * @brief Creates a public IMediaCapabilitiesFactory with server-side context.
     *
     * This method is SERVER-INTERNAL ONLY. It creates a factory that knows about
     * preloaded capabilities provided by ServerManager.
     *
     * @param[in] preloadedAudio   Optional audio capabilities preloaded by ServerManager
     * @param[in] preloadedVideo   Optional video capabilities preloaded by ServerManager
     *
     * @retval the factory instance (implements IMediaCapabilitiesFactory) or null on error.
     */
    virtual std::shared_ptr<firebolt::rialto::IMediaCapabilitiesFactory> createMediaCapabilitiesFactory(
        const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &preloadedAudio,
        const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &preloadedVideo) const = 0;
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_MEDIA_CAPABILITIES_SERVER_INTERNAL_H_
