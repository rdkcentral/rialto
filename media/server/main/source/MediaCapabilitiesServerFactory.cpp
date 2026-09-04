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

#include <stdexcept>

#include "IGstCapabilities.h"
#include "MediaCapabilities.h"
#include "MediaCapabilitiesServerFactory.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
std::unique_ptr<firebolt::rialto::IMediaCapabilities> MediaCapabilitiesServerFactory::createMediaCapabilities(
    const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &preloadedAudio,
    const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &preloadedVideo) const
{
    std::unique_ptr<firebolt::rialto::IMediaCapabilities> mediaCapabilities;

    try
    {
        // Create GstCapabilities for runtime GStreamer queries (fallback path only)
        std::shared_ptr<IGstCapabilitiesFactory> gstCapabilitiesFactory = IGstCapabilitiesFactory::getFactory();
        if (!gstCapabilitiesFactory)
        {
            throw std::runtime_error("Failed to get the gstreamer capabilities factory");
        }
        std::shared_ptr<IGstCapabilities> gstCapabilities = gstCapabilitiesFactory->createGstCapabilities();

        // Create MediaCapabilities orchestrator with 2-path strategy:
        // - Path 0 (Highest priority): Preloaded capabilities from ServerManager
        // - Path B (Fallback): GStreamer element queries
        mediaCapabilities = std::make_unique<MediaCapabilities>(gstCapabilities, preloadedAudio, preloadedVideo);

        RIALTO_SERVER_LOG_DEBUG("Created server-side MediaCapabilities with YAML + GStreamer orchestration");
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the server-side media capabilities, reason: %s", e.what());
    }

    return mediaCapabilities;
}

class MediaCapabilitiesServerInternalFactoryImpl : public IMediaCapabilitiesServerInternalFactory
{
public:
    MediaCapabilitiesServerInternalFactoryImpl() = default;
    ~MediaCapabilitiesServerInternalFactoryImpl() override = default;

    std::shared_ptr<firebolt::rialto::IMediaCapabilitiesFactory> createMediaCapabilitiesFactory() const override
    {
        return std::make_shared<MediaCapabilitiesServerFactory>();
    }
};

std::shared_ptr<IMediaCapabilitiesServerInternalFactory> IMediaCapabilitiesServerInternalFactory::createFactory()
{
    std::shared_ptr<IMediaCapabilitiesServerInternalFactory> factory;

    try
    {
        factory = std::make_shared<MediaCapabilitiesServerInternalFactoryImpl>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the server-side media capabilities factory, reason: %s", e.what());
    }

    return factory;
}

} // namespace firebolt::rialto::server
