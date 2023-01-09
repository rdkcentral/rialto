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
#include "ISharedMemoryManager.h"
#include "IWebAudioPlayerIpc.h"
#include "IWebAudioPlayerIpcClient.h"
#include "RialtoClientLogging.h"
#include <cstring>
#include <mutex>

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
        webAudioPlayer = std::make_unique<client::WebAudioPlayer>(client, audioMimeType, priority, config,
                                                                  client::IWebAudioPlayerIpcFactory::getFactory(),
                                                                  client::ISharedMemoryManagerFactory::createFactory());
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
                               const uint32_t priority, const WebAudioConfig *config,
                               const std::shared_ptr<IWebAudioPlayerIpcFactory> &webAudioPlayerIpcFactory,
                               const std::shared_ptr<ISharedMemoryManagerFactory> &sharedMemoryManagerFactory)
    : m_webAudioPlayerClient(client)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    m_sharedMemoryManager = sharedMemoryManagerFactory->getSharedMemoryManager();
    if (!m_sharedMemoryManager)
    {
        throw std::runtime_error("Shared memory manager could not be created");
    }

    if (!m_sharedMemoryManager->registerClient(this))
    {
        throw std::runtime_error("Failed to register client with SharedMemoryManager");
    }

    if (!webAudioPlayerIpcFactory)
    {
        throw std::runtime_error("Web audio player ipc factory could not be null");
    }

    m_webAudioPlayerIpc = webAudioPlayerIpcFactory->createWebAudioPlayerIpc(this, audioMimeType, priority, config);
    if (!m_webAudioPlayerIpc)
    {
        (void)m_sharedMemoryManager->unregisterClient(this);
        throw std::runtime_error("Web audio player ipc could not be created");
    }
}

WebAudioPlayer::~WebAudioPlayer()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    m_webAudioPlayerIpc.reset();
    if (!m_sharedMemoryManager->unregisterClient(this))
    {
        RIALTO_CLIENT_LOG_WARN("Failed to unregister client with SharedMemoryManager");
    }

    m_sharedMemoryManager.reset();
}

bool WebAudioPlayer::play()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_webAudioPlayerIpc->play();
}

bool WebAudioPlayer::pause()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_webAudioPlayerIpc->pause();
}

bool WebAudioPlayer::setEos()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_webAudioPlayerIpc->setEos();
}

bool WebAudioPlayer::getBufferAvailable(uint32_t &availableFrames, std::shared_ptr<WebAudioShmInfo> &)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    if (!m_webAudioShmInfo)
    {
        m_webAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    }
    std::lock_guard<std::mutex> bufLocker(m_bufLock);
    return m_webAudioPlayerIpc->getBufferAvailable(availableFrames, m_webAudioShmInfo);
}

bool WebAudioPlayer::getBufferDelay(uint32_t &delayFrames)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_webAudioPlayerIpc->getBufferDelay(delayFrames);
}

bool WebAudioPlayer::writeBuffer(const uint32_t numberOfFrames, void *data)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    if (!m_webAudioShmInfo)
    {
        RIALTO_CLIENT_LOG_ERROR("Web audo shared info is null!");
        return false;
    }

    uint32_t dataLength = numberOfFrames * 4;
    uint32_t availableDataLength = m_webAudioShmInfo->lengthMain + m_webAudioShmInfo->lengthWrap;
    if (dataLength > availableDataLength)
    {
        RIALTO_CLIENT_LOG_ERROR("The number of frames to write exceeds the available space!");
        return false;
    }

    uint8_t *shmBuffer = m_sharedMemoryManager->getSharedMemoryBuffer();
    if (nullptr == shmBuffer)
    {
        RIALTO_CLIENT_LOG_ERROR("Shared buffer no longer valid");
        return false;
    }

    if (dataLength > m_webAudioShmInfo->lengthMain)
    {
        std::memcpy(shmBuffer + m_webAudioShmInfo->offsetMain, data, m_webAudioShmInfo->lengthMain);
        std::memcpy(shmBuffer + m_webAudioShmInfo->offsetWrap,
                    reinterpret_cast<uint8_t *>(data) + m_webAudioShmInfo->lengthMain,
                    dataLength - m_webAudioShmInfo->lengthMain);
    }
    else
    {
        std::memcpy(shmBuffer + m_webAudioShmInfo->offsetMain, data, m_webAudioShmInfo->lengthMain);
    }
    std::lock_guard<std::mutex> bufLocker(m_bufLock);
    return m_webAudioPlayerIpc->writeBuffer(numberOfFrames);
}

bool WebAudioPlayer::getDeviceInfo(uint32_t &preferredFrames, uint32_t &maximumFrames, bool &supportDeferredPlay)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_webAudioPlayerIpc->getDeviceInfo(preferredFrames, maximumFrames, supportDeferredPlay);
}

bool WebAudioPlayer::setVolume(double volume)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_webAudioPlayerIpc->setVolume(volume);
}

bool WebAudioPlayer::getVolume(double &volume)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    return m_webAudioPlayerIpc->getVolume(volume);
}

std::weak_ptr<IWebAudioPlayerClient> WebAudioPlayer::getClient()
{
    return m_webAudioPlayerClient;
}

void WebAudioPlayer::notifyState(WebAudioPlayerState state)
{
    std::shared_ptr<IWebAudioPlayerClient> client = m_webAudioPlayerClient.lock();
    if (client)
    {
        client->notifyState(state);
    }
}

void WebAudioPlayer::notifyBufferTerm() {}

}; // namespace firebolt::rialto::client
