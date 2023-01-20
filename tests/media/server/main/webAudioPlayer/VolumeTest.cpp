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

class RialtoServerWebAudioPlayerApiTest : public WebAudioPlayerTestBase
{
protected:
    RialtoServerWebAudioPlayerApiTest() { createWebAudioPlayer(); }

    ~RialtoServerWebAudioPlayerApiTest() { destroyWebAudioPlayer(); }
};

// Play tests
TEST_F(RialtoServerWebAudioPlayerApiTest, playSuccess)
{
    EXPECT_CALL(*m_gstPlayerMock, play()).WillOnce(Return(true));

    bool status = m_webAudioPlayer->play();
    EXPECT_EQ(status, true);
}

TEST_F(RialtoServerWebAudioPlayerApiTest, playFailure)
{
    EXPECT_CALL(*m_gstPlayerMock, play()).WillOnce(Return(false));

    bool status = m_webAudioPlayer->play();
    EXPECT_EQ(status, false);
}

// Pause tests
TEST_F(RialtoServerWebAudioPlayerApiTest, pauseSuccess)
{
    EXPECT_CALL(*m_gstPlayerMock, pause()).WillOnce(Return(true));

    bool status = m_webAudioPlayer->pause();
    EXPECT_EQ(status, true);
}

TEST_F(RialtoServerWebAudioPlayerApiTest, pauseFailure)
{
    EXPECT_CALL(*m_gstPlayerMock, pause()).WillOnce(Return(false));

    bool status = m_webAudioPlayer->pause();
    EXPECT_EQ(status, false);
}
// SetEos tests
TEST_F(RialtoServerWebAudioPlayerApiTest, setEosSuccess)
{
    EXPECT_CALL(*m_gstPlayerMock, pause()).WillOnce(Return(true));

    bool status = m_webAudioPlayer->setEos();
    EXPECT_EQ(status, true);
}
// GetVolume tests

// SetVolume tests
