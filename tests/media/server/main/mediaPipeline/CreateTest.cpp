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
#include "MediaPipelineClientMock.h"
#include "MediaPipelineServerInternal.h"
#include "SharedMemoryBufferMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::Return;
using ::testing::StrictMock;

namespace
{
constexpr int sessionId{1};
} // namespace

class RialtoServerCreateMediaPipelineTest : public ::testing::Test
{
protected:
    std::shared_ptr<IMediaPipelineClient> m_mediaPipelineClient;
    std::shared_ptr<StrictMock<GstPlayerFactoryMock>> m_gstPlayerFactoryMock;
    VideoRequirements m_videoReq = {};
    std::shared_ptr<StrictMock<SharedMemoryBufferMock>> m_sharedMemoryBufferMock;
    std::unique_ptr<IDataReaderFactory> m_dataReaderFactoryMock;
    std::unique_ptr<IActiveRequests> m_activeRequestsMock;
    StrictMock<DecryptionServiceMock> m_decryptionServiceMock;

    virtual void SetUp()
    {
        m_mediaPipelineClient = std::make_shared<StrictMock<MediaPipelineClientMock>>();

        m_gstPlayerFactoryMock = std::make_shared<StrictMock<GstPlayerFactoryMock>>();
        m_sharedMemoryBufferMock = std::make_shared<StrictMock<SharedMemoryBufferMock>>();
    }

    virtual void TearDown()
    {
        m_gstPlayerFactoryMock.reset();

        m_mediaPipelineClient.reset();
    }
};

/**
 * Test that a MediaPipeline object can be created successfully.
 */
TEST_F(RialtoServerCreateMediaPipelineTest, Create)
{
    std::unique_ptr<IMediaPipeline> mediaPipeline;

    EXPECT_CALL(*m_sharedMemoryBufferMock, mapPartition(sessionId)).WillOnce(Return(true));
    EXPECT_NO_THROW(mediaPipeline = std::make_unique<MediaPipelineServerInternal>(m_mediaPipelineClient, m_videoReq,
                                                                                  m_gstPlayerFactoryMock, sessionId,
                                                                                  m_sharedMemoryBufferMock,
                                                                                  std::move(m_dataReaderFactoryMock),
                                                                                  std::move(m_activeRequestsMock),
                                                                                  m_decryptionServiceMock););
    EXPECT_NE(mediaPipeline, nullptr);
    EXPECT_CALL(*m_sharedMemoryBufferMock, unmapPartition(sessionId)).WillOnce(Return(true));
}
