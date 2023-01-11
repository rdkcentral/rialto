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
    const WebAudioConfig *config, const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer, int handle) const
{
    std::unique_ptr<IWebAudioPlayer> webAudioPlayer;
    try
    {
        webAudioPlayer = std::make_unique<server::WebAudioPlayerServerInternal>(client, audioMimeType, priority, config,
                                                                                shmBuffer, handle,
                                                                                IMainThreadFactory::createFactory(),
                                                                                IGstWebAudioPlayerFactory::getFactory());
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the web audio player, reason: %s", e.what());
    }

    return webAudioPlayer;
}

WebAudioPlayerServerInternal::WebAudioPlayerServerInternal(
    std::weak_ptr<IWebAudioPlayerClient> client, const std::string &audioMimeType, const uint32_t priority,
    const WebAudioConfig *config, const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer, int handle,
    const std::shared_ptr<IMainThreadFactory> &mainThreadFactory,
    const std::shared_ptr<IGstWebAudioPlayerFactory> &gstPlayerFactory)
    : m_webAudioPlayerClient(client), m_shmBuffer{shmBuffer}, m_priority{priority}, m_shmId{handle}, m_dataPtr{nullptr}, m_maxDataLength{0}, m_availableBuffer{}
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (audioMimeType == "audio/x-raw")
    {
        if (config == nullptr)
        {
            throw std::runtime_error("Config is null for 'audio/x-raw'");
        }
    }
    else
    {
        throw std::runtime_error("Mimetype '" + audioMimeType + "' not supported");
    }

    m_mainThread = mainThreadFactory->getMainThread();
    if (!m_mainThread)
    {
        throw std::runtime_error("Failed to get the main thread");
    }
    m_mainThreadClientId = m_mainThread->registerClient();

    bool result = false;
    auto task = [&]() { result = initWebAudioPlayerInternal(audioMimeType, config, gstPlayerFactory); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    if (!result)
    {
        throw std::runtime_error("WebAudioPlayerServerInternal construction failed");
    }
}

bool WebAudioPlayerServerInternal::initWebAudioPlayerInternal(
    const std::string &audioMimeType, const WebAudioConfig *config, const std::shared_ptr<IGstWebAudioPlayerFactory> &gstPlayerFactory)
{
    if (!m_shmBuffer->mapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_shmId))
    {
        RIALTO_SERVER_LOG_ERROR("Unable to map shm partition");
        return false;
    }

    if (!(m_dataPtr = m_shmBuffer->getDataPtr(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_shmId, MediaSourceType::AUDIO)))
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get the data pointer of the partition");
        return false;
    }

    if (!(m_maxDataLength = m_shmBuffer->getMaxDataLen(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_shmId, MediaSourceType::AUDIO)))
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get the length of the partition");
        return false;
    }

    if (!initGstWebAudioPlayer(audioMimeType, config, gstPlayerFactory))
    {
        RIALTO_SERVER_LOG_ERROR("Failed to initalise the GstPlayer");
        return false;
    }

    // Set the available bytes
    m_availableBuffer.lengthMain = m_maxDataLength;

    return true;
}

WebAudioPlayerServerInternal::~WebAudioPlayerServerInternal()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
}

bool WebAudioPlayerServerInternal::play()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (!m_gstPlayer->play())
    {
        RIALTO_SERVER_LOG_ERROR("Play failed");
        return false;
    }

    return true;
}

bool WebAudioPlayerServerInternal::pause()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (!m_gstPlayer->pause())
    {
        RIALTO_SERVER_LOG_ERROR("Pause failed");
        return false;
    }

    return true;
}

bool WebAudioPlayerServerInternal::setEos()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (pause())
    {
        return false;
    }

    if (!m_gstPlayer->flush())
    {
        RIALTO_SERVER_LOG_ERROR("Flushing of the gst player failed");
        return false;
    }

    return true;
}

bool WebAudioPlayerServerInternal::getBufferAvailable(uint32_t &availableFrames,
                                                      std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    *webAudioShmInfo = m_availableBuffer;
    availableFrames = (m_availableBuffer.lengthMain + m_availableBuffer.lengthMain) /  4;

    return true;
}

bool WebAudioPlayerServerInternal::getBufferDelay(uint32_t &delayFrames)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    return false;
}

bool WebAudioPlayerServerInternal::writeBuffer(const uint32_t numberOfFrames, void *data)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    //data can be ignored in the server as the data should be written to the shared memory
    if (0 == numberOfFrames)
    {
        return true;
    }

    if (numberOfFrames * 4 > m_availableBuffer.lengthMain + m_availableBuffer.lengthWrap)
    {
        RIALTO_SERVER_LOG_ERROR("Number of frames written is too large!");
        return false;
    }

    uint8_t* mainPtr = m_dataPtr + m_availableBuffer.offsetMain;
    uint32_t mainLength = 0;
    uint8_t* wrapPtr = nullptr;
    uint32_t wrapLength = 0;
    if (numberOfFrames * 4 < m_availableBuffer.lengthMain)
    {
        mainLength = numberOfFrames * 4;
    }
    else
    {
        mainLength = m_availableBuffer.lengthMain;
        wrapPtr = m_dataPtr + m_availableBuffer.offsetWrap;
        wrapLength = numberOfFrames * 4 - mainLength;
    }

    uint32_t bytesWritten = m_gstPlayer->writeBuffer(mainPtr, mainLength, wrapPtr, wrapLength);

    if (bytesWritten < numberOfFrames * 4)
    {
        if (bytesWritten < m_availableBuffer.lengthMain)
        {
            //m_availableBuffer.lengthMain =
        }
    }

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

    if (!m_gstPlayer->setVolume(volume))
    {
        RIALTO_SERVER_LOG_ERROR("SetVolume failed");
        return false;
    }

    return true;
}

bool WebAudioPlayerServerInternal::getVolume(double &volume)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (!m_gstPlayer->getVolume(volume))
    {
        RIALTO_SERVER_LOG_ERROR("GetVolume failed");
        return false;
    }

    return true;
}

std::weak_ptr<IWebAudioPlayerClient> WebAudioPlayerServerInternal::getClient()
{
    return m_webAudioPlayerClient;
}

void WebAudioPlayerServerInternal::notifyState(WebAudioPlayerState state) {}

bool WebAudioPlayerServerInternal::initGstWebAudioPlayer(const std::string &audioMimeType, const WebAudioConfig *config, const std::shared_ptr<IGstWebAudioPlayerFactory> &gstPlayerFactory)
{
    m_gstPlayer = gstPlayerFactory->createGstWebAudioPlayer(this);
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to load gstreamer player");
        return false;
    }

    m_gstPlayer->attachSource(audioMimeType, config);

    return true;
}

}; // namespace firebolt::rialto::server
