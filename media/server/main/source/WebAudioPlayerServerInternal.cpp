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

namespace
{
constexpr uint32_t kPreferredFrames{640};
} // namespace

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
    : m_webAudioPlayerClient(client), m_shmBuffer{shmBuffer}, m_priority{priority}, m_shmId{handle}, m_dataPtr{nullptr},
      m_maxDataLength{0}, m_availableBuffer{}, m_expectWriteBuffer{false}
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
    const std::string &audioMimeType, const WebAudioConfig *config,
    const std::shared_ptr<IGstWebAudioPlayerFactory> &gstPlayerFactory)
{
    if (!m_shmBuffer->mapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_shmId))
    {
        RIALTO_SERVER_LOG_ERROR("Unable to map shm partition");
        return false;
    }

    if (!(m_dataPtr = m_shmBuffer->getDataPtr(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_shmId,
                                              MediaSourceType::AUDIO)))
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get the data pointer of the partition");
        return false;
    }

    if (!(m_maxDataLength = m_shmBuffer->getMaxDataLen(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_shmId,
                                                       MediaSourceType::AUDIO)))
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

    auto task = [&]()
    {
        if (!m_shmBuffer->unmapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_shmId))
        {
            RIALTO_SERVER_LOG_ERROR("Unable to unmap shm partition");
        }

        m_shmBuffer.reset();
        m_mainThread->unregisterClient(m_mainThreadClientId);
    };
    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
}

bool WebAudioPlayerServerInternal::play()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&]() { m_gstPlayer->play(); };

    m_mainThread->enqueueTask(m_mainThreadClientId, task);

    return true;
}

bool WebAudioPlayerServerInternal::pause()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&]() { m_gstPlayer->pause(); };

    m_mainThread->enqueueTask(m_mainThreadClientId, task);

    return true;
}

bool WebAudioPlayerServerInternal::setEos()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    // TODO(RIALTO-2): Don't set eos if we still have frames to write
    auto task = [&]() { m_gstPlayer->setEos(); };

    m_mainThread->enqueueTask(m_mainThreadClientId, task);

    return true;
}

bool WebAudioPlayerServerInternal::getBufferAvailable(uint32_t &availableFrames,
                                                      std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (!webAudioShmInfo)
    {
        RIALTO_SERVER_LOG_ERROR("WebAudioShmInfo is null");
        return false;
    }

    auto task = [&]()
    {
        *webAudioShmInfo = m_availableBuffer;
        // TODO(RIALTO-2): Calculate bytes per frame instead of using '4'.
        availableFrames = (m_availableBuffer.lengthMain + m_availableBuffer.lengthWrap) / 4;
        RIALTO_SERVER_LOG_ERROR("lukewill: getBufferAvailable %u, %u, %u", availableFrames, m_availableBuffer.lengthMain,  m_availableBuffer.lengthWrap);

        // A new getBufferAvailable shall overwrite the previous if writeBuffer is not called inbetween
        m_expectWriteBuffer = true;
    };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);

    return true;
}

bool WebAudioPlayerServerInternal::getBufferDelay(uint32_t &delayFrames)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    // TODO(RIALTO-2): Calculate the delayed frames based on bytes queued in shm and gst.
    return false;
}

bool WebAudioPlayerServerInternal::writeBuffer(const uint32_t numberOfFrames, void *data)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result = false;
    auto task = [&]()
    {
        // data can be ignored, the frames should be written to the shared memory
        result = writeBufferInternal(numberOfFrames);
    };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);

    return result;
}

bool WebAudioPlayerServerInternal::writeBufferInternal(const uint32_t numberOfFrames)
{
    if (!m_expectWriteBuffer)
    {
        RIALTO_SERVER_LOG_ERROR("No getBufferAvailable to match with this writeBuffer call");
        return false;
    }
    m_expectWriteBuffer = false;

    // Cancel timer

    // Write stored frames first, only write new frames if stored frames successful
    if (!writeStoredBuffers())
    {
        // Update the available buffer with the new data that will not be written to gst
        updateAvailableBuffer(numberOfFrames * 4, 0);
        return true;
    }

    if (0 == numberOfFrames)
    {
        // No frames to write to gst
        return true;
    }

    uint8_t *mainPtr = m_dataPtr + m_availableBuffer.offsetMain;
    uint8_t *wrapPtr = m_dataPtr + m_availableBuffer.offsetWrap;
    uint32_t mainLength = 0;
    uint32_t wrapLength = 0;
    if (numberOfFrames * 4 <= m_availableBuffer.lengthMain)
    {
        mainLength = numberOfFrames * 4;
    }
    else
    {
        mainLength = m_availableBuffer.lengthMain;
        wrapLength = numberOfFrames * 4 - mainLength;
    }

    RIALTO_SERVER_LOG_ERROR("lukewill: %p, %u, %p, %u", mainPtr, mainLength, wrapPtr, wrapLength);
    uint32_t newBytesWritten = m_gstPlayer->writeBuffer(mainPtr, mainLength, wrapPtr, wrapLength);
    updateAvailableBuffer(numberOfFrames * 4, newBytesWritten);
    if (newBytesWritten != numberOfFrames * 4)
    {
        // Start timer
    }
    RIALTO_SERVER_LOG_ERROR("lukewill: %u", newBytesWritten);

    return true;
}

bool WebAudioPlayerServerInternal::writeStoredBuffers()
{
    uint8_t *mainPtr = nullptr;
    uint32_t mainLength = 0;
    uint8_t *wrapPtr = nullptr;
    uint32_t wrapLength = 0;

    if (m_availableBuffer.lengthWrap + m_availableBuffer.lengthMain == m_maxDataLength)
    {
        // No data stored
        return true;
    }
    else if (m_availableBuffer.lengthWrap != 0)
    {
        // Data written to the shared memory has not wrapped, data only written in the middle of the shared memory region
        uint32_t startOfBufferOffset = m_availableBuffer.offsetWrap + m_availableBuffer.lengthWrap;
        mainPtr = m_dataPtr + startOfBufferOffset;
        mainLength = m_availableBuffer.offsetMain - startOfBufferOffset;
    }
    else
    {
        // Data written to the shared memory has wrapped, data at the end and start of the shared memory region
        mainPtr = m_dataPtr + m_availableBuffer.offsetMain + m_availableBuffer.lengthMain;
        mainLength = m_maxDataLength - (m_availableBuffer.offsetMain + m_availableBuffer.lengthMain);
        wrapPtr = m_dataPtr;
        wrapLength = m_availableBuffer.offsetMain - m_availableBuffer.offsetWrap;
    }

    RIALTO_SERVER_LOG_ERROR("lukewill: %p, %u, %p, %u", mainPtr, mainLength, wrapPtr, wrapLength);
    uint32_t bytesWritten = m_gstPlayer->writeBuffer(mainPtr, mainLength, wrapPtr, wrapLength);
    RIALTO_SERVER_LOG_ERROR("lukewill: %u", bytesWritten);

    // Update available buffer with the bytes written to gst.
    // Bytes written to shm is 0 becuase they have already been handled.
    updateAvailableBuffer(0, bytesWritten);

    if (bytesWritten == mainLength + wrapLength)
    {
        // All data written to gstreamer
        return true;
    }

    return false;
}

void WebAudioPlayerServerInternal::updateAvailableBuffer(uint32_t bytesWrittenToShm, uint32_t bytesWrittenToGst)
{
    RIALTO_SERVER_LOG_ERROR("lukewill: updateAvailableBuffer bytesWrittenToShm %u, bytesWrittenToGst %u", bytesWrittenToShm, bytesWrittenToGst);
    RIALTO_SERVER_LOG_ERROR("lukewill: updateAvailableBuffer lengthMain %u, lengthWrap %u", m_availableBuffer.lengthMain, m_availableBuffer.lengthWrap);
    if (bytesWrittenToShm < m_availableBuffer.lengthMain)
    {
        uint32_t dataLengthAtEndOfShm = m_maxDataLength - (m_availableBuffer.offsetMain + m_availableBuffer.lengthMain);

        // Data written to the shared memory has not wrapped
        m_availableBuffer.offsetMain = m_availableBuffer.offsetMain + bytesWrittenToShm;
        if (bytesWrittenToGst <= dataLengthAtEndOfShm)
        {
            // Data written to gstreamer has not wrapped, data written is taken from the main buffer only
            m_availableBuffer.lengthMain = m_availableBuffer.lengthMain - bytesWrittenToShm + bytesWrittenToGst;
        }
        else
        {
            // Data written to gstreamer has wrapped, data written is taken from the main buffer and wrapped buffer
            m_availableBuffer.lengthMain = m_availableBuffer.lengthMain - bytesWrittenToShm + dataLengthAtEndOfShm;
            m_availableBuffer.lengthWrap = m_availableBuffer.lengthWrap + (bytesWrittenToGst - dataLengthAtEndOfShm);
        }
    }
    else
    {
        uint32_t dataLengthAtEndOfShm = m_maxDataLength - (m_availableBuffer.offsetMain + m_availableBuffer.lengthMain);

        // Data written to the shared memory has wrapped, available buffer should now point to where the wrapped buffer was
        m_availableBuffer.offsetMain = m_availableBuffer.offsetWrap + (bytesWrittenToShm - m_availableBuffer.lengthMain);

        if (bytesWrittenToGst <= dataLengthAtEndOfShm)
        {
            // Data written to gstreamer has not wrapped, data written is taken from the main buffer only
            m_availableBuffer.lengthMain = m_availableBuffer.lengthWrap + bytesWrittenToGst;
            m_availableBuffer.lengthWrap = 0;
        }
        else
        {
            // Data written to gstreamer has wrapped, data written is taken from the main buffer and wrapped buffer
            m_availableBuffer.lengthMain = m_availableBuffer.lengthWrap + m_availableBuffer.lengthMain + dataLengthAtEndOfShm;
            m_availableBuffer.lengthWrap = bytesWrittenToGst - dataLengthAtEndOfShm;
        }
    }
    RIALTO_SERVER_LOG_ERROR("lukewill: updateAvailableBuffer lengthMain %u, lengthWrap %u", m_availableBuffer.lengthMain, m_availableBuffer.lengthWrap);
}

bool WebAudioPlayerServerInternal::getDeviceInfo(uint32_t &preferredFrames, uint32_t &maximumFrames,
                                                 bool &supportDeferredPlay)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    // Can be called from any thread
    preferredFrames = kPreferredFrames;
    maximumFrames = m_maxDataLength / 4;
    supportDeferredPlay = true;

    return true;
}

bool WebAudioPlayerServerInternal::setVolume(double volume)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&]() { m_gstPlayer->setVolume(volume); };

    m_mainThread->enqueueTask(m_mainThreadClientId, task);

    return true;
}

bool WebAudioPlayerServerInternal::getVolume(double &volume)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    bool result = false;
    auto task = [&]() { result = m_gstPlayer->getVolume(volume); };

    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);

    return result;
}

std::weak_ptr<IWebAudioPlayerClient> WebAudioPlayerServerInternal::getClient()
{
    return m_webAudioPlayerClient;
}

void WebAudioPlayerServerInternal::notifyState(WebAudioPlayerState state)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto task = [&, state]()
    {
        if (m_webAudioPlayerClient)
        {
            m_webAudioPlayerClient->notifyState(state);
        }
    };

    m_mainThread->enqueueTask(m_mainThreadClientId, task);
}

bool WebAudioPlayerServerInternal::initGstWebAudioPlayer(const std::string &audioMimeType, const WebAudioConfig *config,
                                                         const std::shared_ptr<IGstWebAudioPlayerFactory> &gstPlayerFactory)
{
    m_gstPlayer = gstPlayerFactory->createGstWebAudioPlayer(this);
    if (!m_gstPlayer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to load gstreamer player");
        return false;
    }

    m_gstPlayer->setCaps(audioMimeType, config);

    return true;
}

}; // namespace firebolt::rialto::server
