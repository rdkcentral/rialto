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

#include "ActiveRequestsMock.h"
#include "DataReaderFactoryMock.h"
#include "DataReaderMock.h"
#include "DecryptionServiceMock.h"
#include "GstPlayerFactoryMock.h"
#include "GstPlayerMock.h"
#include "MediaPipelineClientMock.h"
#include "MediaPipelineServerInternal.h"
#include "SharedMemoryBufferMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::server::mock;

using ::testing::_;
using ::testing::A;
using ::testing::ByMove;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;
using ::testing::Throw;

class RialtoServerMediaPipelineHaveDataTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<MediaPipelineClientMock>> m_mediaPipelineClient;
    std::unique_ptr<IMediaPipelineServerInternal> m_mediaPipeline;
    std::shared_ptr<StrictMock<GstPlayerFactoryMock>> m_gstPlayerFactoryMock;
    StrictMock<GstPlayerMock> *m_gstPlayerMock = nullptr;
    std::shared_ptr<StrictMock<SharedMemoryBufferMock>> m_sharedMemoryBufferMock;
    std::unique_ptr<IDataReaderFactory> m_dataReaderFactory;
    StrictMock<DataReaderFactoryMock> *m_dataReaderFactoryMock;
    std::unique_ptr<IActiveRequests> m_activeRequests;
    StrictMock<ActiveRequestsMock> *m_activeRequestsMock;
    StrictMock<DecryptionServiceMock> m_decryptionServiceMock;
    const int m_kSessionId{1};
    const uint32_t m_kNumFrames{24};
    const uint32_t m_kNeedDataRequestId{0};

    virtual void SetUp()
    {
        m_mediaPipelineClient = std::make_shared<StrictMock<MediaPipelineClientMock>>();

        m_gstPlayerFactoryMock = std::make_shared<StrictMock<GstPlayerFactoryMock>>();
        m_sharedMemoryBufferMock = std::make_shared<StrictMock<SharedMemoryBufferMock>>();

        m_activeRequests = std::make_unique<StrictMock<ActiveRequestsMock>>();
        m_activeRequestsMock = static_cast<StrictMock<ActiveRequestsMock> *>(m_activeRequests.get());

        m_dataReaderFactory = std::make_unique<StrictMock<DataReaderFactoryMock>>();
        m_dataReaderFactoryMock = static_cast<StrictMock<DataReaderFactoryMock> *>(m_dataReaderFactory.get());

        createMediaPipeline();
    }

    virtual void TearDown()
    {
        EXPECT_CALL(*m_sharedMemoryBufferMock, unmapPartition(m_kSessionId)).WillOnce(Return(true));
        m_mediaPipeline.reset();

        m_gstPlayerFactoryMock.reset();
        m_gstPlayerMock = nullptr;

        m_mediaPipelineClient.reset();
    }

    void createMediaPipeline()
    {
        VideoRequirements videoReq = {};

        EXPECT_CALL(*m_sharedMemoryBufferMock, mapPartition(m_kSessionId)).WillOnce(Return(true));
        EXPECT_NO_THROW(
            m_mediaPipeline =
                std::make_unique<MediaPipelineServerInternal>(m_mediaPipelineClient, videoReq, m_gstPlayerFactoryMock,
                                                              m_kSessionId, m_sharedMemoryBufferMock,
                                                              std::move(m_dataReaderFactory),
                                                              std::move(m_activeRequests), m_decryptionServiceMock););
        EXPECT_NE(m_mediaPipeline, nullptr);
    }

    void LoadGstPlayer()
    {
        std::unique_ptr<StrictMock<GstPlayerMock>> gstPlayerMock = std::make_unique<StrictMock<GstPlayerMock>>();

        // Save a raw pointer to the unique object for use when testing mocks
        // Object shall be freed by the holder of the unique ptr on destruction
        m_gstPlayerMock = gstPlayerMock.get();

        EXPECT_CALL(*m_gstPlayerFactoryMock, createGstPlayer(_, _, _)).WillOnce(Return(ByMove(std::move(gstPlayerMock))));
        MediaPipelineClientMock &mediaPipelineClientMock =
            dynamic_cast<StrictMock<MediaPipelineClientMock> &>(*m_mediaPipelineClient);
        EXPECT_CALL(mediaPipelineClientMock, notifyNetworkState(NetworkState::BUFFERING));

        EXPECT_EQ(m_mediaPipeline->load(MediaType::MSE, "mime", "mse://1"), true);
    }
};

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataFailureDueToUninitializedPlayer)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    IMediaPipeline *mediaPipeline = m_mediaPipeline.get();
    EXPECT_FALSE(mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataSuccessWithUnknownRequestId)
{
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::UNKNOWN));
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    IMediaPipeline *mediaPipeline = m_mediaPipeline.get();
    EXPECT_TRUE(mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataSuccessWithNeedMediaDataResend)
{
    auto status = firebolt::rialto::MediaSourceStatus::ERROR;
    IMediaPipeline *mediaPipeline = m_mediaPipeline.get();
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    int sourceId{static_cast<int>(mediaSourceType)};
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId)).WillOnce(Return(mediaSourceType));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    ASSERT_TRUE(m_sharedMemoryBufferMock);
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_sharedMemoryBufferMock, clearBuffer(m_kSessionId, mediaSourceType)).WillOnce(Return(true));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferLen(m_kSessionId, mediaSourceType)).WillOnce(Return(7 * 1024 * 1024));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferOffset(m_kSessionId, mediaSourceType)).WillOnce(Return(0));
    EXPECT_CALL(*m_activeRequestsMock, insert(mediaSourceType, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mediaPipelineClient,
                notifyNeedMediaData(sourceId, m_kNumFrames, 0, _)); // params tested in NeedMediaDataTests
    EXPECT_TRUE(mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataGettingSamplesThrows)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    IMediaPipeline::MediaSegmentVector dataVec;
    IMediaPipeline *mediaPipeline = m_mediaPipeline.get();
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, getSegments(m_kNeedDataRequestId))
        .WillOnce(Throw(std::runtime_error("runtime_error")));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_FALSE(mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataSuccess)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    IMediaPipeline::MediaSegmentVector dataVec;
    IMediaPipeline *mediaPipeline = m_mediaPipeline.get();
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, getSegments(m_kNeedDataRequestId)).WillOnce(ReturnRef(dataVec));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_gstPlayerMock, attachSamples(A<const IMediaPipeline::MediaSegmentVector &>()));
    EXPECT_TRUE(mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataSuccessEos)
{
    auto status = firebolt::rialto::MediaSourceStatus::EOS;
    IMediaPipeline::MediaSegmentVector dataVec;
    IMediaPipeline *mediaPipeline = m_mediaPipeline.get();
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, getSegments(m_kNeedDataRequestId)).WillOnce(ReturnRef(dataVec));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_gstPlayerMock, attachSamples(A<const IMediaPipeline::MediaSegmentVector &>()));
    EXPECT_CALL(*m_gstPlayerMock, setEos(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_TRUE(mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, AddSegmentReturnsError)
{
    IMediaPipeline *mediaPipeline = m_mediaPipeline.get();
    std::unique_ptr<IMediaPipeline::MediaSegment> segment = std::make_unique<IMediaPipeline::MediaSegment>();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, addSegment(m_kNeedDataRequestId, Ref(segment)))
        .WillOnce(Return(AddSegmentStatus::ERROR));
    EXPECT_EQ(mediaPipeline->addSegment(m_kNeedDataRequestId, segment), AddSegmentStatus::ERROR);
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, AddSegmentReturnsNoSpace)
{
    IMediaPipeline *mediaPipeline = m_mediaPipeline.get();
    std::unique_ptr<IMediaPipeline::MediaSegment> segment = std::make_unique<IMediaPipeline::MediaSegment>();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, addSegment(m_kNeedDataRequestId, Ref(segment)))
        .WillOnce(Return(AddSegmentStatus::NO_SPACE));
    EXPECT_EQ(mediaPipeline->addSegment(m_kNeedDataRequestId, segment), AddSegmentStatus::NO_SPACE);
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, AddSegmentSuccess)
{
    IMediaPipeline *mediaPipeline = m_mediaPipeline.get();
    std::unique_ptr<IMediaPipeline::MediaSegment> segment = std::make_unique<IMediaPipeline::MediaSegment>();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, addSegment(m_kNeedDataRequestId, Ref(segment)))
        .WillOnce(Return(AddSegmentStatus::OK));
    EXPECT_EQ(mediaPipeline->addSegment(m_kNeedDataRequestId, segment), AddSegmentStatus::OK);
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataFailureDueToUninitializedPlayer)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    EXPECT_FALSE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataSuccessWithUnknownRequestId)
{
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::UNKNOWN));
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    EXPECT_TRUE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataSuccessWithNeedMediaDataResend)
{
    auto status = firebolt::rialto::MediaSourceStatus::ERROR;
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    int sourceId{static_cast<int>(mediaSourceType)};
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId)).WillOnce(Return(mediaSourceType));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    ASSERT_TRUE(m_sharedMemoryBufferMock);
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_sharedMemoryBufferMock, clearBuffer(m_kSessionId, mediaSourceType)).WillOnce(Return(true));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferLen(m_kSessionId, mediaSourceType)).WillOnce(Return(7 * 1024 * 1024));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferOffset(m_kSessionId, mediaSourceType)).WillOnce(Return(0));
    EXPECT_CALL(*m_activeRequestsMock, insert(mediaSourceType, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mediaPipelineClient,
                notifyNeedMediaData(sourceId, m_kNumFrames, 0, _)); // params tested in NeedMediaDataTests
    EXPECT_TRUE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataFailureDueToShmBufferError)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferForSession(m_kSessionId)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_mediaPipelineClient, notifyPlaybackState(PlaybackState::FAILURE));
    EXPECT_FALSE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataFailureDueToInvalidBufferOffset)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    std::uint8_t data{123};
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferForSession(m_kSessionId)).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferOffset(m_kSessionId, firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Throw(std::runtime_error("runtime_error")));
    EXPECT_CALL(*m_mediaPipelineClient, notifyPlaybackState(PlaybackState::FAILURE));
    EXPECT_FALSE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataFailureDueToUnsupportedMetadataVersion)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    std::uint8_t data{123};
    int offset = 0;
    std::shared_ptr<IDataReader> dataReader;
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferForSession(m_kSessionId)).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferOffset(m_kSessionId, firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(offset));
    EXPECT_CALL(*m_dataReaderFactoryMock,
                createDataReader(firebolt::rialto::MediaSourceType::VIDEO, &data, offset, m_kNumFrames))
        .WillOnce(Return(dataReader));
    EXPECT_CALL(*m_mediaPipelineClient, notifyPlaybackState(PlaybackState::FAILURE));
    EXPECT_FALSE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataSuccess)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    std::uint8_t data{123};
    int offset = 0;
    std::shared_ptr<IDataReader> dataReader{std::make_shared<DataReaderMock>()};
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferForSession(m_kSessionId)).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferOffset(m_kSessionId, firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(offset));
    EXPECT_CALL(*m_dataReaderFactoryMock,
                createDataReader(firebolt::rialto::MediaSourceType::VIDEO, &data, offset, m_kNumFrames))
        .WillOnce(Return(dataReader));
    EXPECT_CALL(*m_gstPlayerMock, attachSamples(dataReader));
    EXPECT_TRUE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataAudioSuccess)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    std::uint8_t data{123};
    int offset = 64;
    std::shared_ptr<IDataReader> dataReader{std::make_shared<DataReaderMock>()};
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::AUDIO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferForSession(m_kSessionId)).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferOffset(m_kSessionId, firebolt::rialto::MediaSourceType::AUDIO))
        .WillOnce(Return(offset));
    EXPECT_CALL(*m_dataReaderFactoryMock,
                createDataReader(firebolt::rialto::MediaSourceType::AUDIO, &data, offset, m_kNumFrames))
        .WillOnce(Return(dataReader));
    EXPECT_CALL(*m_gstPlayerMock, attachSamples(dataReader));
    EXPECT_TRUE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataSuccessWithEos)
{
    auto status = firebolt::rialto::MediaSourceStatus::EOS;
    std::uint8_t data{123};
    int offset = 0;
    std::shared_ptr<IDataReader> dataReader{std::make_shared<DataReaderMock>()};
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferForSession(m_kSessionId)).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferOffset(m_kSessionId, firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(offset));
    EXPECT_CALL(*m_dataReaderFactoryMock,
                createDataReader(firebolt::rialto::MediaSourceType::VIDEO, &data, offset, m_kNumFrames))
        .WillOnce(Return(dataReader));
    EXPECT_CALL(*m_gstPlayerMock, attachSamples(dataReader));
    EXPECT_CALL(*m_gstPlayerMock, setEos(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_TRUE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataSuccessEosWithoutFrames)
{
    auto status = firebolt::rialto::MediaSourceStatus::EOS;
    std::uint8_t data{123};
    int offset = 0;
    std::shared_ptr<IDataReader> dataReader{std::make_shared<DataReaderMock>()};
    LoadGstPlayer();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferForSession(m_kSessionId)).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferOffset(m_kSessionId, firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(offset));
    EXPECT_CALL(*m_gstPlayerMock, setEos(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_TRUE(m_mediaPipeline->haveData(status, 0, m_kNeedDataRequestId));
}
