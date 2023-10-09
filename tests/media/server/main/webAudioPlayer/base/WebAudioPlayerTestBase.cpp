/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "WebAudioPlayerTestBase.h"
#include <memory>
#include <string>
#include <utility>

WebAudioPlayerTestBase::WebAudioPlayerTestBase()
    : m_webAudioPlayerClientMock{std::make_shared<StrictMock<WebAudioPlayerClientMock>>()},
      m_sharedMemoryBufferMock{std::make_shared<StrictMock<SharedMemoryBufferMock>>()},
      m_mainThreadFactoryMock{std::make_shared<StrictMock<MainThreadFactoryMock>>()},
      m_mainThreadMock{std::make_shared<StrictMock<MainThreadMock>>()},
      m_gstPlayerFactoryMock{std::make_shared<StrictMock<GstWebAudioPlayerFactoryMock>>()},
      m_gstPlayer{std::make_unique<StrictMock<GstWebAudioPlayerMock>>()},
      m_gstPlayerMock{static_cast<StrictMock<GstWebAudioPlayerMock> *>(m_gstPlayer.get())},
      m_timerFactoryMock{std::make_shared<StrictMock<TimerFactoryMock>>()}, m_timerMock{nullptr}
{
    m_config.pcm.rate = 1;
    m_config.pcm.channels = 2;
    m_config.pcm.sampleSize = 16;
    m_config.pcm.isBigEndian = false;
    m_config.pcm.isSigned = false;
    m_config.pcm.isFloat = false;
    m_bytesPerFrame = m_config.pcm.channels * (m_config.pcm.sampleSize / 8);
    m_maxFrame = m_dataLen / m_bytesPerFrame;
}

WebAudioPlayerTestBase::~WebAudioPlayerTestBase() {}

void WebAudioPlayerTestBase::createWebAudioPlayer()
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                mapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_webAudioPlayerHandle))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(&m_dataPtr));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataOffset(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                                         m_webAudioPlayerHandle, MediaSourceType::AUDIO))
        .WillOnce(Return(m_dataOffset));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getMaxDataLen(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                                         m_webAudioPlayerHandle, MediaSourceType::AUDIO))
        .WillOnce(Return(m_dataLen));
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstWebAudioPlayer(_, m_priority))
        .WillOnce(Return(ByMove(std::move(m_gstPlayer))));
    EXPECT_CALL(*m_gstPlayerMock, setCaps(m_audioMimeType, &m_config));

    EXPECT_NO_THROW(m_webAudioPlayer =
                        std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                       m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                       m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                       m_gstPlayerFactoryMock, m_timerFactoryMock));
    EXPECT_NE(m_webAudioPlayer, nullptr);
}

void WebAudioPlayerTestBase::destroyWebAudioPlayer()
{
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                unmapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_webAudioPlayerHandle))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mainThreadMock, unregisterClient(m_kMainThreadClientId));
    // Objects are destroyed on the main thread
    mainThreadWillEnqueueTaskAndWait();

    m_webAudioPlayer.reset();
}

void WebAudioPlayerTestBase::mainThreadWillEnqueueTask()
{
    EXPECT_CALL(*m_mainThreadMock, enqueueTask(m_kMainThreadClientId, _))
        .WillOnce(Invoke([](uint32_t clientId, firebolt::rialto::server::IMainThread::Task task) { task(); }))
        .RetiresOnSaturation();
}

void WebAudioPlayerTestBase::mainThreadWillEnqueueTaskAndWait()
{
    EXPECT_CALL(*m_mainThreadMock, enqueueTaskAndWait(m_kMainThreadClientId, _))
        .WillOnce(Invoke([](uint32_t clientId, firebolt::rialto::server::IMainThread::Task task) { task(); }))
        .RetiresOnSaturation();
}

void WebAudioPlayerTestBase::getBufferAvailableSuccess(uint32_t expectedAvailableFrames,
                                                       const std::shared_ptr<WebAudioShmInfo> &expectedShmInfo)
{
    m_webAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    mainThreadWillEnqueueTaskAndWait();
    bool status = m_webAudioPlayer->getBufferAvailable(m_availableFrames, m_webAudioShmInfo);
    EXPECT_EQ(status, true);
    EXPECT_EQ(m_availableFrames, expectedAvailableFrames);
    if (expectedShmInfo)
    {
        EXPECT_EQ(m_webAudioShmInfo->lengthMain, expectedShmInfo->lengthMain);
        EXPECT_EQ(m_webAudioShmInfo->offsetMain, expectedShmInfo->offsetMain);
        EXPECT_EQ(m_webAudioShmInfo->lengthWrap, expectedShmInfo->lengthWrap);
        EXPECT_EQ(m_webAudioShmInfo->offsetWrap, expectedShmInfo->offsetWrap);
    }
}

void WebAudioPlayerTestBase::writeBufferSuccess(uint32_t newFramesToWrite)
{
    mainThreadWillEnqueueTaskAndWait();
    bool status = m_webAudioPlayer->writeBuffer(newFramesToWrite, nullptr);
    EXPECT_EQ(status, true);
}

void WebAudioPlayerTestBase::expectWriteStoredFrames(uint32_t storedFramesToWrite, uint32_t storedFramesWritten)
{
    uint8_t *expectedStoredMainPtr = nullptr;
    uint8_t *expectedStoredWrapPtr = nullptr;
    uint32_t expectedStoredMainLength = 0;
    uint32_t expectedStoredWrapLength = 0;

    if (m_webAudioShmInfo->lengthWrap != 0)
    {
        // Should start reading data from the end of the wrap free buffer
        expectedStoredMainPtr = &m_dataPtr + m_webAudioShmInfo->offsetWrap + m_webAudioShmInfo->lengthWrap;
        expectedStoredMainLength = storedFramesToWrite * 4;
    }
    else
    {
        // Should start reading data from the end of the main free buffer
        expectedStoredMainPtr = &m_dataPtr + m_webAudioShmInfo->offsetMain + m_webAudioShmInfo->lengthMain;
        std::cout << m_dataLen << ", " << m_webAudioShmInfo->offsetMain << ", " << m_webAudioShmInfo->lengthMain
                  << std::endl;
        expectedStoredMainLength = m_dataLen -
                                   ((m_webAudioShmInfo->offsetMain - m_dataOffset) + m_webAudioShmInfo->lengthMain);
        // Continue reading data from the start of the buffer
        expectedStoredWrapPtr = &m_dataPtr + m_webAudioShmInfo->offsetWrap;
        expectedStoredWrapLength = m_webAudioShmInfo->offsetMain - m_webAudioShmInfo->offsetWrap;
    }

    std::cout << expectedStoredMainLength << ", " << expectedStoredWrapLength << std::endl;
    EXPECT_CALL(*m_gstPlayerMock, writeBuffer(expectedStoredMainPtr, expectedStoredMainLength, expectedStoredWrapPtr,
                                              expectedStoredWrapLength))
        .WillOnce(Return(storedFramesWritten * 4))
        .RetiresOnSaturation();

    m_framesStored += -storedFramesWritten;
}

void WebAudioPlayerTestBase::expectWriteNewFrames(uint32_t newFramesToWrite, uint32_t newFramesWritten)
{
    uint8_t *expectedNewMainPtr = &m_dataPtr + m_webAudioShmInfo->offsetMain;
    uint8_t *expectedNewWrapPtr = &m_dataPtr + m_webAudioShmInfo->offsetWrap;
    uint32_t expectedNewMainLength = 0;
    uint32_t expectedNewWrapLength = 0;

    if (newFramesToWrite * 4 <= m_webAudioShmInfo->lengthMain)
    {
        expectedNewMainLength = newFramesToWrite * 4;
    }
    else
    {
        expectedNewMainLength = m_webAudioShmInfo->lengthMain;
        expectedNewWrapLength = newFramesToWrite * 4 - m_webAudioShmInfo->lengthMain;
    }

    EXPECT_CALL(*m_gstPlayerMock,
                writeBuffer(expectedNewMainPtr, expectedNewMainLength, expectedNewWrapPtr, expectedNewWrapLength))
        .WillOnce(Return(newFramesWritten * 4))
        .RetiresOnSaturation();

    m_framesStored += newFramesToWrite - newFramesWritten;
}

void WebAudioPlayerTestBase::expectStartTimer()
{
    m_timer = std::make_unique<StrictMock<TimerMock>>();
    m_timerMock = static_cast<StrictMock<TimerMock> *>(m_timer.get());
    EXPECT_CALL(*m_timerFactoryMock, createTimer(m_kExpectedWriteDataTimeout, _, _))
        .WillOnce(DoAll(SaveArg<1>(&m_writeBufferTimerCallback), Return(ByMove(std::move(m_timer)))))
        .RetiresOnSaturation();
}

void WebAudioPlayerTestBase::expectCancelTimer()
{
    EXPECT_CALL(*m_timerMock, isActive()).WillOnce(Return(true)).RetiresOnSaturation();
    EXPECT_CALL(*m_timerMock, cancel()).RetiresOnSaturation();
}

void WebAudioPlayerTestBase::expectConstructionOfWebAudioPlayerServerInternal()
{
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                mapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_webAudioPlayerHandle))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(&m_dataPtr));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataOffset(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                                         m_webAudioPlayerHandle, MediaSourceType::AUDIO))
        .WillOnce(Return(m_dataOffset));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getMaxDataLen(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                                         m_webAudioPlayerHandle, MediaSourceType::AUDIO))
        .WillOnce(Return(m_dataLen));
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstWebAudioPlayer(_, m_priority))
        .WillOnce(Return(ByMove(std::move(m_gstPlayer))));
    EXPECT_CALL(*m_gstPlayerMock, setCaps(m_audioMimeType, &m_config));
}
