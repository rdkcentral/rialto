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

#include "WebAudioPlayerIpcTestBase.h"

MATCHER_P2(WebAudioSetVolumeRequestMatcher, web_audio_player_handle, volume, "")
{
    std::cout << "vlume: " << volume << std::endl;
    const ::firebolt::rialto::WebAudioSetVolumeRequest *request =
        dynamic_cast<const ::firebolt::rialto::WebAudioSetVolumeRequest *>(arg);
    std::cout << "vlume2: " << request->volume() << std::endl;    
    std::cout << "web_audio_player_handle: " << request->web_audio_player_handle() << "\n";   
    return ((request->web_audio_player_handle() == web_audio_player_handle) && (request->volume() < volume + 0.001) &&
            (request->volume() > volume - 0.001));
}

class RialtoClientWebAudioPlayerIpcSetVolumeTest : public WebAudioPlayerIpcTestBase
{
protected:
    double m_volume{0.7};

    virtual void SetUp()
    {
        WebAudioPlayerIpcTestBase::SetUp();

        createWebAudioPlayerIpc();
    }

    virtual void TearDown()
    {
        destroyWebAudioPlayerIpc();

        WebAudioPlayerIpcTestBase::TearDown();
    }
};

/**
 * Test that setVolume can be called successfully.
 */
TEST_F(RialtoClientWebAudioPlayerIpcSetVolumeTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setVolume"), m_controllerMock.get(),
                                           WebAudioSetVolumeRequestMatcher(m_web_audio_player_handle, m_volume), _,
                                           m_blockingClosureMock.get()));

    EXPECT_EQ(m_webAudioPlayerIpc->setVolume(m_volume), true);
}

/**
 * Test that setVolume fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcSetVolumeTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_webAudioPlayerIpc->setVolume(m_volume), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setVolume fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcSetVolumeTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setVolume"), _, _, _, _));

    EXPECT_EQ(m_webAudioPlayerIpc->setVolume(m_volume), true);
}

/**
 * Test that setVolume fails when ipc fails.
 */
TEST_F(RialtoClientWebAudioPlayerIpcSetVolumeTest, SetVolumeFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setVolume"), _, _, _, _));

    EXPECT_EQ(m_webAudioPlayerIpc->setVolume(m_volume), false);
}
