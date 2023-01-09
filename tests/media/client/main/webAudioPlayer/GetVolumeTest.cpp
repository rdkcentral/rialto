/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 202 Sky UK
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

class RialtoClientWebAudioPlayerGetVolumeTest : public WebAudioPlayerTestBase
{
protected:
    double m_volume{0};
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
 * Test that getVolume returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientWebAudioPlayerGetVolumeTest, getVolumeSuccess)
{
    constexpr double kVolume{0.7};
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getVolume(_))
        .WillOnce(Invoke(
            [&](double &vol)
            {
                vol = kVolume;
                return true;
            }));
    EXPECT_EQ(m_webAudioPlayer->getVolume(m_volume), true);
    EXPECT_EQ(kVolume, m_volume);
}

/**
 * Test that getVolume returns failure if the IPC API fails.
 */
TEST_F(RialtoClientWebAudioPlayerGetVolumeTest, getVolumeFailure)
{
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getVolume(m_volume)).WillOnce(Return(false));

    EXPECT_EQ(m_webAudioPlayer->getVolume(m_volume), false);
}
