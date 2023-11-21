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

class RialtoClientWebAudioPlayerGetBufferAvailableTest : public WebAudioPlayerTestBase
{
protected:
    uint32_t m_availableFrames{51};

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
 * Test that getBufferAvailable returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientWebAudioPlayerGetBufferAvailableTest, getBufferAvailableSuccess)
{
    std::shared_ptr<WebAudioShmInfo> webAudioShmInfo;

    constexpr uint32_t kAvailableFrames{12};
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferAvailable(_, _))
        .WillOnce(Invoke(
            [&](uint32_t &availableFrames, const std::shared_ptr<WebAudioShmInfo> &)
            {
                availableFrames = kAvailableFrames;
                return true;
            }));

    EXPECT_EQ(m_webAudioPlayer->getBufferAvailable(m_availableFrames, webAudioShmInfo), true);
    EXPECT_EQ(kAvailableFrames, m_availableFrames);
}

/**
 * Test that getBufferAvailable returns failure if the IPC API fails.
 */
TEST_F(RialtoClientWebAudioPlayerGetBufferAvailableTest, getBufferAvailableFailure)
{
    std::shared_ptr<WebAudioShmInfo> webAudioShmInfo;

    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferAvailable(m_availableFrames, _)).WillOnce(Return(false));

    EXPECT_EQ(m_webAudioPlayer->getBufferAvailable(m_availableFrames, webAudioShmInfo), false);
}
