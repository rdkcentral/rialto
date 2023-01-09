/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "WebAudioPlayerTestBase.h"

class RialtoClientWebAudioPlayerGetBufferDelayTest : public WebAudioPlayerTestBase
{
protected:
    uint32_t m_bufferDelay{51};

    virtual void SetUp()
    {
        WebAudioPlayerTestBase::SetUp();

        createWebAudioPlayer();
    }

    virtual void TearDown()
    {
        destroyWebAudioPlayer();

        WebAudioPlayerTestBase::TearDown();
    }
};

/**
 * Test that getBufferDelay returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientWebAudioPlayerGetBufferDelayTest, getBufferDelaySuccess)
{
    constexpr uint32_t kBufferDelay{12};
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferDelay(_))
        .WillOnce(Invoke(
            [&](uint32_t &bufferDelay)
            {
                bufferDelay = kBufferDelay;
                return true;
            }));

    EXPECT_EQ(m_webAudioPlayer->getBufferDelay(m_bufferDelay), true);
    EXPECT_EQ(kBufferDelay, m_bufferDelay);
}

/**
 * Test that getBufferDelay returns failure if the IPC API fails.
 */
TEST_F(RialtoClientWebAudioPlayerGetBufferDelayTest, getBufferDelayFailure)
{
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferDelay(m_bufferDelay)).WillOnce(Return(false));

    EXPECT_EQ(m_webAudioPlayer->getBufferDelay(m_bufferDelay), false);
}
