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

#ifndef FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_SERVER_FACTORY_H_
#define FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_SERVER_FACTORY_H_

#include "IMediaCapabilities.h"
#include <memory>

namespace firebolt::rialto::server
{
/**
 * @brief Server-side MediaCapabilities factory implementation.
 *
 * This factory creates server-side MediaCapabilities instances that orchestrate
 * between YamlCapabilities (preloaded configuration) and GstCapabilities (runtime queries).
 *
 * This is different from the client-side IMediaCapabilitiesFactory which creates
 * IPC-backed clients. The session server should ALWAYS use this server-side factory
 * to avoid recursive IPC calls and deadlocks.
 */
class MediaCapabilitiesServerFactory : public firebolt::rialto::IMediaCapabilitiesFactory
{
public:
    MediaCapabilitiesServerFactory() = default;
    ~MediaCapabilitiesServerFactory() override = default;

    /**
     * @brief Creates a server-side MediaCapabilities instance.
     *
     * @param[in] preloadedAudio   Optional preloaded audio capabilities from ServerManager
     * @param[in] preloadedVideo   Optional preloaded video capabilities from ServerManager
     *
     * @retval the IMediaCapabilities instance or null on error.
     */
    std::unique_ptr<firebolt::rialto::IMediaCapabilities> createMediaCapabilities(
        const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &preloadedAudio = std::nullopt,
        const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &preloadedVideo = std::nullopt) const override;
};

/**
 * @brief Server-internal factory for creating MediaCapabilitiesServerFactory instances.
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
     * @brief Creates a server-side IMediaCapabilitiesFactory.
     *
     * @retval the factory instance or null on error.
     */
    virtual std::shared_ptr<firebolt::rialto::IMediaCapabilitiesFactory> createMediaCapabilitiesFactory() const = 0;
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_SERVER_FACTORY_H_
