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

class RialtoClientWebAudioPlayerSetVolumeTest : public WebAudioPlayerTestBase
{
protected:
    const double m_kVolume{0.7};

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
 * Test that setVolume returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientWebAudioPlayerSetVolumeTest, setVolumeSuccess)
{
    EXPECT_CALL(*m_webAudioPlayerIpcMock, setVolume(m_kVolume)).WillOnce(Return(true));

    EXPECT_EQ(m_webAudioPlayer->setVolume(m_kVolume), true);
}

/**
 * Test that setVolume returns failure if the IPC API fails.
 */
TEST_F(RialtoClientWebAudioPlayerSetVolumeTest, setVolumeFailure)
{
    EXPECT_CALL(*m_webAudioPlayerIpcMock, setVolume(m_kVolume)).WillOnce(Return(false));

    EXPECT_EQ(m_webAudioPlayer->setVolume(m_kVolume), false);
}
