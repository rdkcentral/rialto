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
      m_gstPlayerFactoryMock{std::make_shared<StrictMock<GstGenericPlayerFactoryMock>>()},
      m_gstPlayer{std::make_unique<StrictMock<GstGenericPlayerMock>>()},
      m_gstPlayerMock{static_cast<StrictMock<GstGenericPlayerMock> *>(m_gstPlayer.get())},
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
    EXPECT_CALL(*m_sharedMemoryBufferMock, mapPartition(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_kSessionId))
        .WillOnce(Return(true));
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
    EXPECT_CALL(*m_sharedMemoryBufferMock, unmapPartition(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_kSessionId))
        .WillOnce(Return(true));
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
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstGenericPlayer(_, _, _, _, _))
        .WillOnce(DoAll(SaveArg<0>(&m_gstPlayerCallback), Return(ByMove(std::move(m_gstPlayer)))));
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(NetworkState::BUFFERING));

    EXPECT_EQ(m_mediaPipeline->load(MediaType::MSE, "mime", "mse://1"), true);
    ASSERT_NE(m_gstPlayerCallback, nullptr);
}

int MediaPipelineTestBase::attachSource(MediaSourceType sourceType, const std::string &mimeType)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource;
    if (MediaSourceType::AUDIO == sourceType)
    {
        mediaSource = std::make_unique<IMediaPipeline::MediaSourceAudio>(mimeType);
    }
    else
    {
        mediaSource = std::make_unique<IMediaPipeline::MediaSourceVideo>(mimeType);
    }

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));

    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);

    return mediaSource->getId();
}

void MediaPipelineTestBase::setEos(MediaSourceType sourceType)
{
    const uint32_t kNeedDataRequestId{0};
    auto status = firebolt::rialto::MediaSourceStatus::EOS;
    IMediaPipeline::MediaSegmentVector dataVec;
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(kNeedDataRequestId)).WillOnce(Return(sourceType));
    EXPECT_CALL(*m_activeRequestsMock, getSegments(kNeedDataRequestId)).WillOnce(ReturnRef(dataVec));
    EXPECT_CALL(*m_activeRequestsMock, erase(kNeedDataRequestId));
    EXPECT_CALL(*m_gstPlayerMock, attachSamples(A<const IMediaPipeline::MediaSegmentVector &>()));
    EXPECT_CALL(*m_gstPlayerMock, setEos(sourceType));
    EXPECT_TRUE(m_mediaPipeline->haveData(status, kNeedDataRequestId));
}

void MediaPipelineTestBase::expectNotifyNeedData(MediaSourceType sourceType, int sourceId, int numFrames)
{
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_sharedMemoryBufferMock);
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                clearData(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_kSessionId, sourceType))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                getMaxDataLen(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_kSessionId, sourceType))
        .WillOnce(Return(7 * 1024 * 1024));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                getDataOffset(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_kSessionId, sourceType))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_activeRequestsMock, insert(sourceType, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mediaPipelineClientMock,
                notifyNeedMediaData(sourceId, numFrames, 0, _)); // params tested in NeedMediaDataTests
}

void MediaPipelineTestBase::expectNotifyNeedDataEos(MediaSourceType sourceType)
{
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_sharedMemoryBufferMock);
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                clearData(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_kSessionId, sourceType))
        .WillOnce(Return(true));
}
