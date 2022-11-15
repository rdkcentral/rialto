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
#include "DecryptionServiceMock.h"
#include "GstPlayerFactoryMock.h"
#include "GstPlayerMock.h"
#include "IGstPlayerClient.h"
#include "MediaPipelineClientMock.h"
#include "MediaPipelineServerInternal.h"
#include "SharedMemoryBufferMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;

MATCHER_P(QosInfoMatcher, expectedQosInfo, "")
{
    return ((expectedQosInfo.processed == arg.processed) && (expectedQosInfo.dropped == arg.dropped));
}

class RialtoServerMediaPipelineCallbackTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<MediaPipelineClientMock>> m_mediaPipelineClient;
    IGstPlayerClient *m_gstPlayerCallback;
    std::unique_ptr<IMediaPipelineServerInternal> m_mediaPipeline;
    std::shared_ptr<StrictMock<GstPlayerFactoryMock>> m_gstPlayerFactoryMock;
    std::unique_ptr<StrictMock<GstPlayerMock>> m_gstPlayerMock;
    std::shared_ptr<StrictMock<SharedMemoryBufferMock>> m_sharedMemoryBufferMock;
    std::unique_ptr<IDataReaderFactory> m_dataReaderFactoryMock;
    std::unique_ptr<IActiveRequests> m_activeRequests;
    StrictMock<ActiveRequestsMock> *m_activeRequestsMock;
    StrictMock<DecryptionServiceMock> m_decryptionServiceMock;
    const int m_kSessionId{1};

    virtual void SetUp()
    {
        m_mediaPipelineClient = std::make_shared<StrictMock<MediaPipelineClientMock>>();

        m_gstPlayerFactoryMock = std::make_shared<StrictMock<GstPlayerFactoryMock>>();
        m_gstPlayerMock = std::make_unique<StrictMock<GstPlayerMock>>();
        m_sharedMemoryBufferMock = std::make_shared<StrictMock<SharedMemoryBufferMock>>();
        m_activeRequests = std::make_unique<StrictMock<ActiveRequestsMock>>();
        m_activeRequestsMock = static_cast<StrictMock<ActiveRequestsMock> *>(m_activeRequests.get());

        createMediaPipeline();

        GetGstPlayerClient();
    }

    virtual void TearDown()
    {
        EXPECT_CALL(*m_sharedMemoryBufferMock, unmapPartition(m_kSessionId)).WillOnce(Return(true));
        m_mediaPipeline.reset();

        m_gstPlayerMock.reset();
        m_gstPlayerFactoryMock.reset();

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
                                                              std::move(m_dataReaderFactoryMock),
                                                              std::move(m_activeRequests), m_decryptionServiceMock););
        EXPECT_NE(m_mediaPipeline, nullptr);
    }

    void GetGstPlayerClient()
    {
        EXPECT_CALL(*m_gstPlayerFactoryMock, createGstPlayer(_, _, _))
            .WillOnce(DoAll(SaveArg<0>(&m_gstPlayerCallback), Return(ByMove(std::move(m_gstPlayerMock)))));
        EXPECT_CALL(*m_mediaPipelineClient, notifyNetworkState(NetworkState::BUFFERING));

        EXPECT_EQ(m_mediaPipeline->load(MediaType::MSE, "mime", "mse://1"), true);

        ASSERT_NE(m_gstPlayerCallback, nullptr);
    }
};

/**
 * Test a notification of the playback state is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, NotifyPlaybackState)
{
    PlaybackState state = PlaybackState::IDLE;
    EXPECT_CALL(*m_mediaPipelineClient, notifyPlaybackState(state));

    m_gstPlayerCallback->notifyPlaybackState(state);
}

/**
 * Test a notification of the position is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyPosition)
{
    int64_t position{12345};
    EXPECT_CALL(*m_mediaPipelineClient, notifyPosition(position));

    m_gstPlayerCallback->notifyPosition(position);
}

/**
 * Test a notification of the network state is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyNetworkState)
{
    auto state{firebolt::rialto::NetworkState::BUFFERING};
    EXPECT_CALL(*m_mediaPipelineClient, notifyNetworkState(state));

    m_gstPlayerCallback->notifyNetworkState(state);
}

/**
 * Test a notification of the need media data is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyNeedMediaData)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    int sourceId{static_cast<int>(firebolt::rialto::MediaSourceType::VIDEO)};
    int numFrames{24};
    ASSERT_TRUE(m_sharedMemoryBufferMock);
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_sharedMemoryBufferMock, clearBuffer(m_kSessionId, mediaSourceType)).WillOnce(Return(true));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferLen(m_kSessionId, mediaSourceType)).WillOnce(Return(7 * 1024 * 1024));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBufferOffset(m_kSessionId, mediaSourceType)).WillOnce(Return(0));
    EXPECT_CALL(*m_activeRequestsMock, insert(mediaSourceType, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mediaPipelineClient,
                notifyNeedMediaData(sourceId, numFrames, 0, _)); // params tested in NeedMediaDataTests

    m_gstPlayerCallback->notifyNeedMediaData(mediaSourceType);
}

/**
 * Test a notification of qos is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyQos)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    int sourceId{static_cast<int>(firebolt::rialto::MediaSourceType::VIDEO)};
    QosInfo qosInfo{5u, 2u};

    EXPECT_CALL(*m_mediaPipelineClient, notifyQos(sourceId, QosInfoMatcher(qosInfo)));

    m_gstPlayerCallback->notifyQos(mediaSourceType, qosInfo);
}

/**
 * Tests if active request cache is cleared.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, clearActiveRequestsCache)
{
    EXPECT_CALL(*m_activeRequestsMock, clear());

    m_gstPlayerCallback->clearActiveRequestsCache();
}
