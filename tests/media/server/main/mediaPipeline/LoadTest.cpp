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
#include "MediaPipelineClientMock.h"
#include "MediaPipelineServerInternal.h"
#include "SharedMemoryBufferMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::server;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrictMock;

class RialtoServerMediaPipelineLoadTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<MediaPipelineClientMock>> m_mediaPipelineClientMock;
    std::unique_ptr<IMediaPipeline> m_mediaPipeline;
    std::shared_ptr<StrictMock<GstPlayerFactoryMock>> m_gstPlayerFactoryMock;
    std::unique_ptr<StrictMock<GstPlayerMock>> m_gstPlayerMock;
    std::shared_ptr<StrictMock<SharedMemoryBufferMock>> m_sharedMemoryBufferMock;
    std::unique_ptr<IDataReaderFactory> m_dataReaderFactoryMock;
    std::unique_ptr<IActiveRequests> m_activeRequestsMock;
    StrictMock<DecryptionServiceMock> m_decryptionServiceMock;
    MediaType m_type = MediaType::MSE;
    const int m_kSessionId{1};
    const std::string m_kMimeType = "mime";
    const std::string m_kUrl = "mse://1";

    virtual void SetUp()
    {
        m_mediaPipelineClientMock = std::make_shared<StrictMock<MediaPipelineClientMock>>();

        m_gstPlayerFactoryMock = std::make_shared<StrictMock<GstPlayerFactoryMock>>();
        m_gstPlayerMock = std::make_unique<StrictMock<GstPlayerMock>>();
        m_sharedMemoryBufferMock = std::make_shared<StrictMock<SharedMemoryBufferMock>>();

        createMediaPipeline();
    }

    virtual void TearDown()
    {
        EXPECT_CALL(*m_sharedMemoryBufferMock, unmapPartition(m_kSessionId)).WillOnce(Return(true));
        m_mediaPipeline.reset();

        m_gstPlayerMock.reset();
        m_gstPlayerFactoryMock.reset();

        m_mediaPipelineClientMock.reset();
    }

    void createMediaPipeline()
    {
        VideoRequirements videoReq = {};

        EXPECT_CALL(*m_sharedMemoryBufferMock, mapPartition(m_kSessionId)).WillOnce(Return(true));
        EXPECT_NO_THROW(
            m_mediaPipeline = std::make_unique<MediaPipelineServerInternal>(m_mediaPipelineClientMock, videoReq,
                                                                            m_gstPlayerFactoryMock, m_kSessionId,
                                                                            m_sharedMemoryBufferMock,
                                                                            std::move(m_dataReaderFactoryMock),
                                                                            std::move(m_activeRequestsMock),
                                                                            m_decryptionServiceMock););
        EXPECT_NE(m_mediaPipeline, nullptr);
    }
};

/**
 * Test that Load returns success if create gstreamer player succeeds.
 */
TEST_F(RialtoServerMediaPipelineLoadTest, Success)
{
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstPlayer(_, _, m_type)).WillOnce(Return(ByMove(std::move(m_gstPlayerMock))));
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(NetworkState::BUFFERING));

    EXPECT_EQ(m_mediaPipeline->load(m_type, m_kMimeType, m_kUrl), true);
}

/**
 * Test that Load returns failure if the create gstreamer player API fails.
 * No update of NetworkState.
 */
TEST_F(RialtoServerMediaPipelineLoadTest, CreateGstPlayerFailure)
{
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstPlayer(_, _, m_type)).WillOnce(Return(ByMove(nullptr)));
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(_)).Times(0);

    EXPECT_EQ(m_mediaPipeline->load(m_type, m_kMimeType, m_kUrl), false);
}
