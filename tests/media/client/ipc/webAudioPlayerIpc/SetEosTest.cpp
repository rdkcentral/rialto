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

MATCHER_P(WebAudioSetWosRequestMatcher, web_audio_player_handle, "")
{
    const ::firebolt::rialto::WebAudioSetEosRequest *request =
        dynamic_cast<const ::firebolt::rialto::WebAudioSetEosRequest *>(arg);
    return ((request->web_audio_player_handle() == web_audio_player_handle));
}

class RialtoClientWebAudioPlayerIpcSetEosTest : public WebAudioPlayerIpcTestBase
{
protected:

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
 * Test that setEos can be called successfully.
 */
TEST_F(RialtoClientWebAudioPlayerIpcSetEosTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setEos"), m_controllerMock.get(),
                                           WebAudioSetWosRequestMatcher(m_webAaudioPlayerHandle), _,
                                           m_blockingClosureMock.get()));

    EXPECT_EQ(m_webAudioPlayerIpc->setEos(), true);
}

/**
 * Test that setEos fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcSetEosTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_webAudioPlayerIpc->setEos(), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setEos fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcSetEosTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setEos"), _, _, _, _));

    EXPECT_EQ(m_webAudioPlayerIpc->setEos(), true);
}

/**
 * Test that setEos fails when ipc fails.
 */
TEST_F(RialtoClientWebAudioPlayerIpcSetEosTest, SetEosFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setEos"), _, _, _, _));

    EXPECT_EQ(m_webAudioPlayerIpc->setEos(), false);
}
