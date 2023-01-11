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
      m_gstPlayerMock{static_cast<StrictMock<GstWebAudioPlayerMock> *>(m_gstPlayer.get())}
{
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
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataPtr(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                                      m_webAudioPlayerHandle, MediaSourceType::AUDIO))
        .WillOnce(Return(&m_dataPtr));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getMaxDataLen(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                                         m_webAudioPlayerHandle, MediaSourceType::AUDIO))
        .WillOnce(Return(m_dataLen));
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstWebAudioPlayer(_)).WillOnce(Return(ByMove(std::move(m_gstPlayer))));
    EXPECT_CALL(*m_gstPlayerMock, attachSource(m_audioMimeType, &m_config));

    EXPECT_NO_THROW(m_webAudioPlayer =
                        std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                       m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                       m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                       m_gstPlayerFactoryMock));
    EXPECT_NE(m_webAudioPlayer, nullptr);
}

void WebAudioPlayerTestBase::destroyWebAudioPlayer()
{
    EXPECT_CALL(*m_sharedMemoryBufferMock, unmapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_webAudioPlayerHandle))
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
