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
#include "IMediaCapabilitiesServerInternal.h"
#include "MediaCapabilities.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
/**
 * @brief Server-side factory implementation that knows about preloaded capabilities
 *
 * This factory stores preloaded capabilities and uses them when creating
 * MediaCapabilities instances. This is returned by IMediaCapabilitiesServerInternalFactory.
 *
 * Different from client-side factory (no parameters, creates IPC stubs).
 */
class MediaCapabilitiesServerFactory : public firebolt::rialto::IMediaCapabilitiesFactory
{
private:
    std::optional<firebolt::rialto::common::AudioDecoderCapabilities> m_preloadedAudio;
    std::optional<firebolt::rialto::common::VideoDecoderCapabilities> m_preloadedVideo;

public:
    /**
     * @brief Constructor that stores preloaded capabilities
     *
     * @param[in] preloadedAudio   Preloaded audio from ServerManager
     * @param[in] preloadedVideo   Preloaded video from ServerManager
     */
    MediaCapabilitiesServerFactory(const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &preloadedAudio,
                                   const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &preloadedVideo)
        : m_preloadedAudio(preloadedAudio), m_preloadedVideo(preloadedVideo)
    {
    }

    ~MediaCapabilitiesServerFactory() override = default;

    /**
     * @brief Creates MediaCapabilities with stored preloaded data
     *
     * Uses the preloaded capabilities stored in this factory instance
     * (provided during construction from ServerManager)
     */
    std::unique_ptr<firebolt::rialto::IMediaCapabilities> createMediaCapabilities() const override
    {
        std::unique_ptr<firebolt::rialto::IMediaCapabilities> mediaCapabilities;

        try
        {
            // Create GstCapabilities for runtime GStreamer queries (fallback path)
            std::shared_ptr<IGstCapabilitiesFactory> gstCapabilitiesFactory = IGstCapabilitiesFactory::getFactory();
            if (!gstCapabilitiesFactory)
            {
                throw std::runtime_error("Failed to get the gstreamer capabilities factory");
            }

            auto gstCapabilitiesUnique = gstCapabilitiesFactory->createGstCapabilities();
            if (!gstCapabilitiesUnique)
            {
                throw std::runtime_error("Failed to create GstCapabilities");
            }

            // Create MediaCapabilities orchestrator for GStreamer queries (fallback path)
            // Pass ownership of gstCapabilities to MediaCapabilities (unique_ptr)
            auto mediaCapabilitiesPtr = std::make_unique<MediaCapabilities>(std::move(gstCapabilitiesUnique));

            // Apply preloaded capabilities (Path 0 priority)
            mediaCapabilitiesPtr->setPreloadedCapabilities(m_preloadedAudio, m_preloadedVideo);

            mediaCapabilities = std::move(mediaCapabilitiesPtr);

            RIALTO_SERVER_LOG_DEBUG("Created server-side MediaCapabilities with GStreamer orchestration");
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create server-side media capabilities, reason: %s", e.what());
        }

        return mediaCapabilities;
    }
};

/**
 * @brief Server-internal factory implementation
 *
 * This is the factory used by RialtoServer to create server-side factories.
 * It takes preloaded capabilities and wraps them in a server factory.
 *
 * Pattern: Follows IMediaPipelineServerInternalFactory model
 */
class MediaCapabilitiesServerInternalFactory : public IMediaCapabilitiesServerInternalFactory
{
public:
    MediaCapabilitiesServerInternalFactory() = default;
    ~MediaCapabilitiesServerInternalFactory() override = default;

    /**
     * @brief Creates a server-side factory with preloaded capabilities
     *
     * @param[in] preloadedAudio   Audio capabilities from ServerManager
     * @param[in] preloadedVideo   Video capabilities from ServerManager
     *
     * @retval Factory instance or null on error
     */
    std::shared_ptr<firebolt::rialto::IMediaCapabilitiesFactory> createMediaCapabilitiesFactory(
        const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &preloadedAudio,
        const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &preloadedVideo) const override
    {
        std::shared_ptr<firebolt::rialto::IMediaCapabilitiesFactory> factory;

        try
        {
            factory = std::make_shared<MediaCapabilitiesServerFactory>(preloadedAudio, preloadedVideo);
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create server-side media capabilities factory, reason: %s", e.what());
        }

        return factory;
    }
};

std::shared_ptr<IMediaCapabilitiesServerInternalFactory> IMediaCapabilitiesServerInternalFactory::createFactory()
{
    std::shared_ptr<IMediaCapabilitiesServerInternalFactory> factory;

    try
    {
        factory = std::make_shared<MediaCapabilitiesServerInternalFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the server-internal media capabilities factory, reason: %s", e.what());
    }

    return factory;
}

} // namespace firebolt::rialto::server
