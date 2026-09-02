/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "IMediaCapabilitiesIpcFactory.h"
#include "MediaCapabilities.h"
#include "RialtoClientLogging.h"

namespace firebolt::rialto
{
std::shared_ptr<IMediaCapabilitiesFactory> IMediaCapabilitiesFactory::createFactory()
{
    std::shared_ptr<IMediaCapabilitiesFactory> factory;

    try
    {
        factory = std::make_shared<MediaCapabilitiesFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the media capabilities factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IMediaCapabilities> MediaCapabilitiesFactory::createMediaCapabilities(
    [[maybe_unused]] const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &preloadedAudio,
    [[maybe_unused]] const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &preloadedVideo) const
{
    std::unique_ptr<IMediaCapabilities> mediaCapabilities;
    try
    {
        mediaCapabilities =
            std::make_unique<client::MediaCapabilities>(client::IMediaCapabilitiesIpcFactory::createFactory());
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the media capabilities, reason: %s", e.what());
    }

    return mediaCapabilities;
}

}; // namespace firebolt::rialto

namespace firebolt::rialto::client
{
MediaCapabilities::MediaCapabilities(const std::shared_ptr<IMediaCapabilitiesIpcFactory> &mediaCapabilitiesIpcFactory)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    m_mediaCapabilitiesIpc = mediaCapabilitiesIpcFactory->createMediaCapabilitiesIpc();
    if (!m_mediaCapabilitiesIpc)
    {
        throw std::runtime_error("Media capabilities ipc could not be created");
    }
}

MediaCapabilities::~MediaCapabilities()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    m_mediaCapabilitiesIpc.reset();
}

common::AudioDecoderCapabilities MediaCapabilities::getSupportedAudioCapabilities()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaCapabilitiesIpc->getSupportedAudioCapabilities();
}

common::VideoDecoderCapabilities MediaCapabilities::getSupportedVideoCapabilities()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_mediaCapabilitiesIpc->getSupportedVideoCapabilities();
}

}; // namespace firebolt::rialto::client
