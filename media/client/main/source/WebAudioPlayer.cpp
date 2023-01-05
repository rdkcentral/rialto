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

#include "WebAudioPlayer.h"
#include "RialtoClientLogging.h"

namespace firebolt::rialto
{
std::shared_ptr<IWebAudioPlayerFactory> IWebAudioPlayerFactory::createFactory()
{
    std::shared_ptr<IWebAudioPlayerFactory> factory;

    try
    {
        factory = std::make_shared<WebAudioPlayerFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the web audio player factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IWebAudioPlayer> WebAudioPlayerFactory::createWebAudioPlayer(std::weak_ptr<IWebAudioPlayerClient> client,
                                                                             const std::string &audioMimeType,
                                                                             const uint32_t priority,
                                                                             const WebAudioConfig *config) const
{
    std::unique_ptr<IWebAudioPlayer> webAudioPlayer;
    try
    {
        webAudioPlayer = std::make_unique<client::WebAudioPlayer>(client, audioMimeType, priority, config);
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the web audio player, reason: %s", e.what());
    }

    return webAudioPlayer;
}
}; // namespace firebolt::rialto

namespace firebolt::rialto::client
{
WebAudioPlayer::WebAudioPlayer(std::weak_ptr<IWebAudioPlayerClient> client, const std::string &audioMimeType,
                               const uint32_t priority, const WebAudioConfig *config)
    : m_webAudioPlayerClient(client)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
}

WebAudioPlayer::~WebAudioPlayer()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
}

bool WebAudioPlayer::play()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayer::pause()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayer::setEos()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayer::getBufferAvailable(uint32_t &availableFrames, std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayer::getBufferDelay(uint32_t &delayFrames)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayer::writeBuffer(const uint32_t numberOfFrames, void *data)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayer::getDeviceInfo(uint32_t &preferredFrames, uint32_t &maximumFrames, bool &supportDeferredPlay)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayer::setVolume(double volume)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayer::getVolume(double &volume)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return false;
}

std::weak_ptr<IWebAudioPlayerClient> WebAudioPlayer::getClient()
{
    return m_webAudioPlayerClient;
}

}; // namespace firebolt::rialto::client
