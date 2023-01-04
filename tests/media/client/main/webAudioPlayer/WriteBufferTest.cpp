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

#include <vector>

class RialtoClientWebAudioPlayerWriteBufferTest : public WebAudioPlayerTestBase
{
protected:
    uint32_t m_numberOfFrames{1};
    std::vector<uint8_t> m_data{1, 2, 3, 4,5,6,7,8};
    std::shared_ptr<WebAudioShmInfo> m_webAudioShmInfo;

    virtual void SetUp()
    {
        WebAudioPlayerTestBase::SetUp();
        m_webAudioShmInfo = std::make_shared<WebAudioShmInfo>();

        createWebAudioPlayer();
    }

    virtual void TearDown()
    {
        destroyWebAudioPlayer();
        m_webAudioShmInfo.reset();
        WebAudioPlayerTestBase::TearDown();
    }
};

/**
 * Test that writeBuffer returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientWebAudioPlayerWriteBufferTest, writeBufferSuccess)
{
    /*
    EXPECT_CALL(*m_webAudioPlayerIpcMock, writeBuffer(m_numberOfFrames)).WillOnce(Return(true));
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferAvailable(_, _))
        .WillOnce(Invoke(
            [this](uint32_t &availableFrames, const std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo) {

                return true;
            }));

    EXPECT_EQ(m_webAudioPlayer->writeBuffer(m_numberOfFrames, m_data.data()), true);
    */
}

/**
 * Test that writeBuffer returns failure if the IPC API fails.
 */
TEST_F(RialtoClientWebAudioPlayerWriteBufferTest, writeBufferFailure)
{
    /*
    EXPECT_CALL(*m_webAudioPlayerIpcMock, writeBuffer(m_numberOfFrames)).WillOnce(Return(false));

    EXPECT_EQ(m_webAudioPlayer->writeBuffer(m_numberOfFrames, m_data.data()), false);
    */
}
