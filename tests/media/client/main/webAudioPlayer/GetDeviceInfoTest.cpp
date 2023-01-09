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

class RialtoClientWebAudioPlayerGetDeviceInfoTest : public WebAudioPlayerTestBase
{
protected:
    uint32_t m_preferredFrames = 8;
    uint32_t m_maximumFrames = 12;
    bool m_supportDeferredPlay = true;

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
 * Test that getDeviceInfo returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientWebAudioPlayerGetDeviceInfoTest, getDeviceInfoSuccess)
{
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getDeviceInfo(m_preferredFrames, m_maximumFrames, m_supportDeferredPlay))
        .WillOnce(Return(true));

    EXPECT_EQ(m_webAudioPlayer->getDeviceInfo(m_preferredFrames, m_maximumFrames, m_supportDeferredPlay), true);
}

/**
 * Test that getDeviceInfo returns failure if the IPC API fails.
 */
TEST_F(RialtoClientWebAudioPlayerGetDeviceInfoTest, getDeviceInfoFailure)
{
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getDeviceInfo(_, _, _)).WillOnce(Return(false));

    EXPECT_EQ(m_webAudioPlayer->getDeviceInfo(m_preferredFrames, m_maximumFrames, m_supportDeferredPlay), false);
}
