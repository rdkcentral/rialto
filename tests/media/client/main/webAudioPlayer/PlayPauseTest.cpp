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

class RialtoClientWebAudioPlayerPlayPauseTest : public WebAudioPlayerTestBase
{
protected:
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
 * Test that Play returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientWebAudioPlayerPlayPauseTest, PlaySuccess)
{
    EXPECT_CALL(*m_webAudioPlayerIpcMock, play()).WillOnce(Return(true));

    EXPECT_EQ(m_webAudioPlayer->play(), true);
}

/**
 * Test that Play returns failure if the IPC API fails.
 */
TEST_F(RialtoClientWebAudioPlayerPlayPauseTest, PlayFailure)
{
    EXPECT_CALL(*m_webAudioPlayerIpcMock, play()).WillOnce(Return(false));

    EXPECT_EQ(m_webAudioPlayer->play(), false);
}

/**
 * Test that Pause returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientWebAudioPlayerPlayPauseTest, PauseSuccess)
{
    EXPECT_CALL(*m_webAudioPlayerIpcMock, pause()).WillOnce(Return(true));

    EXPECT_EQ(m_webAudioPlayer->pause(), true);
}

/**
 * Test that Pause returns failure if the IPC API fails.
 */
TEST_F(RialtoClientWebAudioPlayerPlayPauseTest, PauseFailure)
{
    EXPECT_CALL(*m_webAudioPlayerIpcMock, pause()).WillOnce(Return(false));

    EXPECT_EQ(m_webAudioPlayer->pause(), false);
}
