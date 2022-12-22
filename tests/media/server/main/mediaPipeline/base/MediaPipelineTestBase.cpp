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

#include "MediaPipelineTestBase.h"
#include <memory>
#include <string>
#include <utility>

MediaPipelineTestBase::MediaPipelineTestBase()
    : m_mediaPipelineClientMock{std::make_shared<StrictMock<MediaPipelineClientMock>>()},
      m_gstPlayerFactoryMock{std::make_shared<StrictMock<GstPlayerFactoryMock>>()},
      m_gstPlayer{std::make_unique<StrictMock<GstPlayerMock>>()},
      m_gstPlayerMock{static_cast<StrictMock<GstPlayerMock> *>(m_gstPlayer.get())},
      m_sharedMemoryBufferMock{std::make_shared<StrictMock<SharedMemoryBufferMock>>()},
      m_dataReaderFactory{std::make_unique<StrictMock<DataReaderFactoryMock>>()},
      m_dataReaderFactoryMock{static_cast<StrictMock<DataReaderFactoryMock> *>(m_dataReaderFactory.get())},
      m_activeRequests{std::make_unique<StrictMock<ActiveRequestsMock>>()},
      m_activeRequestsMock{static_cast<StrictMock<ActiveRequestsMock> *>(m_activeRequests.get())},
      m_mainThreadFactoryMock{std::make_shared<StrictMock<MainThreadFactoryMock>>()},
      m_mainThreadMock{std::make_shared<StrictMock<MainThreadMock>>()},
      m_timerFactoryMock{std::make_shared<StrictMock<TimerFactoryMock>>()}, m_timerMock{
                                                                                std::make_unique<StrictMock<TimerMock>>()}
{
}

MediaPipelineTestBase::~MediaPipelineTestBase() {}

void MediaPipelineTestBase::createMediaPipeline()
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, mapPartition(m_kSessionId)).WillOnce(Return(true));
    EXPECT_NO_THROW(
        m_mediaPipeline =
            std::make_unique<MediaPipelineServerInternal>(m_mediaPipelineClientMock, m_videoReq, m_gstPlayerFactoryMock,
                                                          m_kSessionId, m_sharedMemoryBufferMock, m_mainThreadFactoryMock,
                                                          m_timerFactoryMock, std::move(m_dataReaderFactory),
                                                          std::move(m_activeRequests), m_decryptionServiceMock););
    EXPECT_NE(m_mediaPipeline, nullptr);
}

void MediaPipelineTestBase::destroyMediaPipeline()
{
    EXPECT_CALL(*m_sharedMemoryBufferMock, unmapPartition(m_kSessionId)).WillOnce(Return(true));
    EXPECT_CALL(*m_mainThreadMock, unregisterClient(m_kMainThreadClientId));
    // Objects are destroyed on the main thread
    mainThreadWillEnqueueTaskAndWait();

    m_mediaPipeline.reset();
}

void MediaPipelineTestBase::mainThreadWillEnqueueTask()
{
    EXPECT_CALL(*m_mainThreadMock, enqueueTask(m_kMainThreadClientId, _))
        .WillOnce(Invoke([](uint32_t clientId, firebolt::rialto::server::IMainThread::Task task) { task(); }))
        .RetiresOnSaturation();
}

void MediaPipelineTestBase::mainThreadWillEnqueueTaskAndWait()
{
    EXPECT_CALL(*m_mainThreadMock, enqueueTaskAndWait(m_kMainThreadClientId, _))
        .WillOnce(Invoke([](uint32_t clientId, firebolt::rialto::server::IMainThread::Task task) { task(); }))
        .RetiresOnSaturation();
}

void MediaPipelineTestBase::loadGstPlayer()
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstPlayer(_, _, _, _)).WillOnce(Return(ByMove(std::move(m_gstPlayer))));
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(NetworkState::BUFFERING));

    EXPECT_EQ(m_mediaPipeline->load(MediaType::MSE, "mime", "mse://1"), true);
}
