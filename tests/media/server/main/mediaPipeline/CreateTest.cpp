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

class RialtoServerCreateMediaPipelineTest : public MediaPipelineTestBase
{
};

/**
 * Test that a MediaPipeline object can be created successfully.
 */
TEST_F(RialtoServerCreateMediaPipelineTest, Create)
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

    EXPECT_CALL(*m_sharedMemoryBufferMock, unmapPartition(m_kSessionId)).WillOnce(Return(true));
    EXPECT_CALL(*m_mainThreadMock, unregisterClient(m_kMainThreadClientId));
    // Objects are destroyed on the main thread
    mainThreadWillEnqueueTaskAndWait();
}
