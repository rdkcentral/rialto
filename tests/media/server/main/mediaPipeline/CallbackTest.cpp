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

#include "IGstGenericPlayerClient.h"
#include "MediaPipelineTestBase.h"

using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::SaveArg;

MATCHER_P(QosInfoMatcher, expectedQosInfo, "")
{
    return ((expectedQosInfo.processed == arg.processed) && (expectedQosInfo.dropped == arg.dropped));
}

class RialtoServerMediaPipelineCallbackTest : public MediaPipelineTestBase
{
protected:
    IGstGenericPlayerClient *m_gstPlayerCallback;

    RialtoServerMediaPipelineCallbackTest()
    {
        createMediaPipeline();

        GetGstPlayerClient();
    }

    ~RialtoServerMediaPipelineCallbackTest() { destroyMediaPipeline(); }

    void GetGstPlayerClient()
    {
        mainThreadWillEnqueueTaskAndWait();
        EXPECT_CALL(*m_gstPlayerFactoryMock, createGstGenericPlayer(_, _, _, _))
            .WillOnce(DoAll(SaveArg<0>(&m_gstPlayerCallback), Return(ByMove(std::move(m_gstPlayer)))));

        // notifyNetworkState posts a task onto the main thread but doesnt wait
        mainThreadWillEnqueueTask();
        EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(NetworkState::BUFFERING));

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
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(state));

    m_gstPlayerCallback->notifyPlaybackState(state);
}

/**
 * Test a notification of the position is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyPosition)
{
    int64_t position{12345};
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPosition(position));

    m_gstPlayerCallback->notifyPosition(position);
}

/**
 * Test a notification of the network state is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyNetworkState)
{
    auto state{firebolt::rialto::NetworkState::BUFFERING};
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(state));

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
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_sharedMemoryBufferMock);
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                clearData(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_kSessionId, mediaSourceType))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                getMaxDataLen(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_kSessionId, mediaSourceType))
        .WillOnce(Return(7 * 1024 * 1024));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                getDataOffset(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_kSessionId, mediaSourceType))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_activeRequestsMock, insert(mediaSourceType, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mediaPipelineClientMock,
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

    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyQos(sourceId, QosInfoMatcher(qosInfo)));

    m_gstPlayerCallback->notifyQos(mediaSourceType, qosInfo);
}

/**
 * Tests if active request cache is cleared.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, clearActiveRequestsCache)
{
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_activeRequestsMock, clear());

    m_gstPlayerCallback->clearActiveRequestsCache();
}
