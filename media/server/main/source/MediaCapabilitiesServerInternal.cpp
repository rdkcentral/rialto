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
#include "MediaCapabilitiesServerInternal.h"
#include "RialtoServerLogging.h"
#include <mutex>

namespace firebolt::rialto::server
{
std::shared_ptr<IMediaCapabilitiesServerInternal>
MediaCapabilitiesServerInternalFactory::createMediaCapabilitiesServerInternal() const
{
    std::shared_ptr<IMediaCapabilitiesServerInternal> mediaCapabilities;

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

        // Create MediaCapabilitiesServerInternal orchestrator for GStreamer queries (fallback path)
        // Pass ownership of gstCapabilities to MediaCapabilitiesServerInternal (unique_ptr)
        // Preloaded capabilities are applied later by the caller via setPreloadedCapabilities()
        mediaCapabilities = std::make_shared<MediaCapabilitiesServerInternal>(std::move(gstCapabilitiesUnique));

        RIALTO_SERVER_LOG_DEBUG("Created server-side MediaCapabilitiesServerInternal with GStreamer orchestration");
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create server-side media capabilities, reason: %s", e.what());
    }

    return mediaCapabilities;
}

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

MediaCapabilitiesServerInternal::MediaCapabilitiesServerInternal(std::unique_ptr<IGstCapabilities> gstCapabilities)
    : m_gstCapabilities{std::move(gstCapabilities)}
{
    RIALTO_SERVER_LOG_DEBUG("MediaCapabilitiesServerInternal: constructor - uses GStreamer for fallback queries");
    if (!m_gstCapabilities)
    {
        throw std::runtime_error("GstCapabilities is required");
    }
}

firebolt::rialto::common::AudioDecoderCapabilities MediaCapabilitiesServerInternal::getSupportedAudioCapabilities()
{
    // Path 0: Check preloaded capabilities first (highest priority)
    {
        std::lock_guard<std::mutex> lock{m_preloadedCapabilitiesMutex};
        if (m_preloadedAudioCapabilities.has_value())
        {
            RIALTO_SERVER_LOG_DEBUG("Returning preloaded audio capabilities (Path 0)");
            return *m_preloadedAudioCapabilities;
        }
    }

    // Path B: Fall back to GStreamer queries
    RIALTO_SERVER_LOG_DEBUG("Returning GStreamer audio capabilities (Path B fallback)");
    return m_gstCapabilities->getSupportedAudioCapabilities();
}

firebolt::rialto::common::VideoDecoderCapabilities MediaCapabilitiesServerInternal::getSupportedVideoCapabilities()
{
    // Path 0: Check preloaded capabilities first (highest priority)
    {
        std::lock_guard<std::mutex> lock{m_preloadedCapabilitiesMutex};
        if (m_preloadedVideoCapabilities.has_value())
        {
            RIALTO_SERVER_LOG_DEBUG("Returning preloaded video capabilities (Path 0)");
            return *m_preloadedVideoCapabilities;
        }
    }

    // Path B: Fall back to GStreamer queries
    RIALTO_SERVER_LOG_DEBUG("Returning GStreamer video capabilities (Path B fallback)");
    return m_gstCapabilities->getSupportedVideoCapabilities();
}

void MediaCapabilitiesServerInternal::setPreloadedCapabilities(
    const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &audioCapabilities,
    const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &videoCapabilities)
{
    std::lock_guard<std::mutex> lock{m_preloadedCapabilitiesMutex};
    m_preloadedAudioCapabilities = audioCapabilities;
    m_preloadedVideoCapabilities = videoCapabilities;
    RIALTO_SERVER_LOG_DEBUG("Preloaded capabilities set (audio: %s, video: %s)",
                            audioCapabilities.has_value() ? "yes" : "no", videoCapabilities.has_value() ? "yes" : "no");
}

} // namespace firebolt::rialto::server
