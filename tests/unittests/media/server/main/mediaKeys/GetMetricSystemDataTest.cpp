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

#include "MediaKeysTestBase.h"

class RialtoServerMediaKeysGetMetricSystemDataTest : public MediaKeysTestBase
{
protected:
    std::vector<uint8_t> m_buffer{};
    uint32_t m_bufferLength{1024};
    const uint32_t kMaxBufferLength{65536};
    
    RialtoServerMediaKeysGetMetricSystemDataTest()
    {
        createMediaKeys(kNetflixKeySystem);
        createKeySession(kNetflixKeySystem);
    }
    ~RialtoServerMediaKeysGetMetricSystemDataTest() { destroyMediaKeys(); }
};

/**
* Test that GetMetricSystemData returns success.
*/
TEST_F(RialtoServerMediaKeysGetMetricSystemDataTest, Success)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_ocdmSystemMock, getMetricSystemData(_, _)).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->getMetricSystemData(m_buffer));
}
 
/**
* Test that GetMetricSystemData interface not implemented returns failure.
 */
TEST_F(RialtoServerMediaKeysGetMetricSystemDataTest, InterfaceNotImplementedFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_ocdmSystemMock, getMetricSystemData(_, _)).WillOnce(Return(MediaKeyErrorStatus::INTERFACE_NOT_IMPLEMENTED));

    EXPECT_EQ(MediaKeyErrorStatus::INTERFACE_NOT_IMPLEMENTED, m_mediaKeys->getMetricSystemData(m_buffer));
}

/**
* Test that GetMetricSystemData buffer is too small and returns success after resizing the buffer.
 */
TEST_F(RialtoServerMediaKeysGetMetricSystemDataTest, BufferTooSmallSuccess)
{
    mainThreadWillEnqueueTaskAndWaitMultiple(2);

    EXPECT_CALL(*m_ocdmSystemMock, getMetricSystemData(_, _))
        .WillOnce(Return(MediaKeyErrorStatus::BUFFER_TOO_SMALL)) 
        .WillOnce(Return(MediaKeyErrorStatus::OK));             

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->getMetricSystemData(m_buffer));
}

/**
* Test that GetMetricSystemData buffer is too small and returns failure after buffer size exceeding the maximum allowed size.
 */
TEST_F(RialtoServerMediaKeysGetMetricSystemDataTest, BufferTooSmallFailure)
{
    mainThreadWillEnqueueTaskAndWaitMultiple(7);

    EXPECT_CALL(*m_ocdmSystemMock, getMetricSystemData(_, _))
        .Times(7)
        .WillRepeatedly(Return(MediaKeyErrorStatus::BUFFER_TOO_SMALL));             

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeys->getMetricSystemData(m_buffer));
}