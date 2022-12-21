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

using ::testing::A;
using ::testing::ByMove;
using ::testing::Ref;
using ::testing::ReturnRef;
using ::testing::Throw;

class RialtoServerMediaPipelineHaveDataTest : public MediaPipelineTestBase
{
protected:
    const uint32_t m_kNumFrames{24};
    const uint32_t m_kNeedDataRequestId{0};
    const std::chrono::milliseconds m_kNeedMediaDataResendTimeout{100};

    RialtoServerMediaPipelineHaveDataTest() { createMediaPipeline(); }

    ~RialtoServerMediaPipelineHaveDataTest() { destroyMediaPipeline(); }
};

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataFailureDueToUninitializedPlayer)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataSuccessWithUnknownRequestId)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::UNKNOWN));
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    EXPECT_TRUE(m_mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataSuccessWithSchedulingNeedMediaDataResend)
{
    auto status = firebolt::rialto::MediaSourceStatus::ERROR;
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId)).WillOnce(Return(mediaSourceType));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_timerMock, isActive()).WillOnce(Return(true));
    EXPECT_CALL(*m_timerMock, cancel());
    EXPECT_CALL(*m_timerFactoryMock, createTimer(m_kNeedMediaDataResendTimeout, _, _))
        .WillOnce(Return(ByMove(std::move(m_timerMock))));
    EXPECT_TRUE(m_mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataGettingSamplesThrows)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    IMediaPipeline::MediaSegmentVector dataVec;
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, getSegments(m_kNeedDataRequestId))
        .WillOnce(Throw(std::runtime_error("runtime_error")));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_FALSE(m_mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataSuccess)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    IMediaPipeline::MediaSegmentVector dataVec;
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, getSegments(m_kNeedDataRequestId)).WillOnce(ReturnRef(dataVec));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_gstPlayerMock, attachSamples(A<const IMediaPipeline::MediaSegmentVector &>()));
    EXPECT_TRUE(m_mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, CommonHaveDataSuccessEos)
{
    auto status = firebolt::rialto::MediaSourceStatus::EOS;
    IMediaPipeline::MediaSegmentVector dataVec;
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, getSegments(m_kNeedDataRequestId)).WillOnce(ReturnRef(dataVec));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_gstPlayerMock, attachSamples(A<const IMediaPipeline::MediaSegmentVector &>()));
    EXPECT_CALL(*m_gstPlayerMock, setEos(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_TRUE(m_mediaPipeline->haveData(status, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, AddSegmentReturnsError)
{
    std::unique_ptr<IMediaPipeline::MediaSegment> segment = std::make_unique<IMediaPipeline::MediaSegment>();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, addSegment(m_kNeedDataRequestId, Ref(segment)))
        .WillOnce(Return(AddSegmentStatus::ERROR));
    EXPECT_EQ(m_mediaPipeline->addSegment(m_kNeedDataRequestId, segment), AddSegmentStatus::ERROR);
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, AddSegmentReturnsNoSpace)
{
    std::unique_ptr<IMediaPipeline::MediaSegment> segment = std::make_unique<IMediaPipeline::MediaSegment>();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, addSegment(m_kNeedDataRequestId, Ref(segment)))
        .WillOnce(Return(AddSegmentStatus::NO_SPACE));
    EXPECT_EQ(m_mediaPipeline->addSegment(m_kNeedDataRequestId, segment), AddSegmentStatus::NO_SPACE);
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, AddSegmentSuccess)
{
    std::unique_ptr<IMediaPipeline::MediaSegment> segment = std::make_unique<IMediaPipeline::MediaSegment>();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, addSegment(m_kNeedDataRequestId, Ref(segment)))
        .WillOnce(Return(AddSegmentStatus::OK));
    EXPECT_EQ(m_mediaPipeline->addSegment(m_kNeedDataRequestId, segment), AddSegmentStatus::OK);
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataFailureDueToUninitializedPlayer)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataSuccessWithUnknownRequestId)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::UNKNOWN));
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    EXPECT_TRUE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataSuccessWithSchedulingNeedMediaDataResend)
{
    auto status = firebolt::rialto::MediaSourceStatus::ERROR;
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId)).WillOnce(Return(mediaSourceType));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_timerMock, isActive()).WillOnce(Return(true));
    EXPECT_CALL(*m_timerMock, cancel());
    EXPECT_CALL(*m_timerFactoryMock, createTimer(m_kNeedMediaDataResendTimeout, _, _))
        .WillOnce(Return(ByMove(std::move(m_timerMock))));
    EXPECT_TRUE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataFailureDueToShmBufferError)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTask();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::FAILURE));
    EXPECT_FALSE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataFailureDueToInvalidBufferOffset)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    std::uint8_t data{123};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTask();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataOffset(m_kSessionId, firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Throw(std::runtime_error("runtime_error")));
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::FAILURE));
    EXPECT_FALSE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataFailureDueToUnsupportedMetadataVersion)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    std::uint8_t data{123};
    int offset = 0;
    std::shared_ptr<IDataReader> dataReader;
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTask();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataOffset(m_kSessionId, firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(offset));
    EXPECT_CALL(*m_dataReaderFactoryMock,
                createDataReader(firebolt::rialto::MediaSourceType::VIDEO, &data, offset, m_kNumFrames))
        .WillOnce(Return(dataReader));
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::FAILURE));
    EXPECT_FALSE(m_mediaPipeline->haveData(status, m_kNumFrames, m_kNeedDataRequestId));
}

TEST_F(RialtoServerMediaPipelineHaveDataTest, ServerInternalHaveDataSuccess)
{
    auto status = firebolt::rialto::MediaSourceStatus::OK;
    std::uint8_t data{123};
    int offset = 0;
    std::shared_ptr<IDataReader> dataReader{std::make_shared<DataReaderMock>()};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataOffset(m_kSessionId, firebolt::rialto::MediaSourceType::VIDEO))
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
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::AUDIO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataOffset(m_kSessionId, firebolt::rialto::MediaSourceType::AUDIO))
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
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataOffset(m_kSessionId, firebolt::rialto::MediaSourceType::VIDEO))
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
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_activeRequestsMock, getType(m_kNeedDataRequestId))
        .WillOnce(Return(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(*m_activeRequestsMock, erase(m_kNeedDataRequestId));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(&data));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataOffset(m_kSessionId, firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(offset));
    EXPECT_CALL(*m_gstPlayerMock, setEos(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_TRUE(m_mediaPipeline->haveData(status, 0, m_kNeedDataRequestId));
}
