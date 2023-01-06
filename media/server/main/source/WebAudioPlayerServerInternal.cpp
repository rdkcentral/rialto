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

#include "WebAudioPlayerServerInternal.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto
{
std::shared_ptr<IWebAudioPlayerFactory> IWebAudioPlayerFactory::createFactory()
{
    return server::IWebAudioPlayerServerInternalFactory::createFactory();
}
}; // namespace firebolt::rialto

namespace firebolt::rialto::server
{
std::shared_ptr<IWebAudioPlayerServerInternalFactory> IWebAudioPlayerServerInternalFactory::createFactory()
{
    std::shared_ptr<IWebAudioPlayerServerInternalFactory> factory;

    try
    {
        factory = std::make_shared<server::WebAudioPlayerServerInternalFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the web audio player factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IWebAudioPlayer>
WebAudioPlayerServerInternalFactory::createWebAudioPlayer(std::weak_ptr<IWebAudioPlayerClient> client,
                                                          const std::string &audioMimeType, const uint32_t priority,
                                                          const WebAudioConfig *config) const
{
    RIALTO_SERVER_LOG_ERROR(
        "This function can't be used by rialto server. Please use createWebAudioPlayerServerInternal");
    return nullptr;
}

std::unique_ptr<IWebAudioPlayer> WebAudioPlayerServerInternalFactory::createWebAudioPlayerServerInternal(
    std::weak_ptr<IWebAudioPlayerClient> client, const std::string &audioMimeType, const uint32_t priority,
    const WebAudioConfig *config, const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer) const
{
    std::unique_ptr<IWebAudioPlayer> webAudioPlayer;
    try
    {
        webAudioPlayer =
            std::make_unique<server::WebAudioPlayerServerInternal>(client, audioMimeType, priority, config, shmBuffer);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the web audio player, reason: %s", e.what());
    }

    return webAudioPlayer;
}

WebAudioPlayerServerInternal::WebAudioPlayerServerInternal(std::weak_ptr<IWebAudioPlayerClient> client,
                                                           const std::string &audioMimeType, const uint32_t priority,
                                                           const WebAudioConfig *config,
                                                           const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer)
    : m_webAudioPlayerClient(client), m_shmBuffer{shmBuffer}
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
}

WebAudioPlayerServerInternal::~WebAudioPlayerServerInternal()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
}

bool WebAudioPlayerServerInternal::play()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayerServerInternal::pause()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayerServerInternal::setEos()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayerServerInternal::getBufferAvailable(uint32_t &availableFrames,
                                                      std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayerServerInternal::getBufferDelay(uint32_t &delayFrames)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayerServerInternal::writeBuffer(const uint32_t numberOfFrames, void *data)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayerServerInternal::getDeviceInfo(uint32_t &preferredFrames, uint32_t &maximumFrames,
                                                 bool &supportDeferredPlay)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayerServerInternal::setVolume(double volume)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayerServerInternal::getVolume(double &volume)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    return false;
}

std::weak_ptr<IWebAudioPlayerClient> WebAudioPlayerServerInternal::getClient()
{
    return m_webAudioPlayerClient;
}

}; // namespace firebolt::rialto::server
