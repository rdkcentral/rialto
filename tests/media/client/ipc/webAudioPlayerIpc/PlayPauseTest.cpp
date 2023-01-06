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
#include "webaudioplayermodule.pb.h"

#include <gtest/gtest.h>

MATCHER_P(WebAudioPlayRequestMatcher, web_audio_player_handle, "")
{
    const ::firebolt::rialto::WebAudioPlayRequest *request = dynamic_cast<const ::firebolt::rialto::WebAudioPlayRequest *>(arg);
    return (request->web_audio_player_handle() == web_audio_player_handle);
}

MATCHER_P(WebAudioPauseRequestMatcher, web_audio_player_handle, "")
{
    const ::firebolt::rialto::WebAudioPauseRequest *request = dynamic_cast<const ::firebolt::rialto::WebAudioPauseRequest *>(arg);
    return (request->web_audio_player_handle() == web_audio_player_handle);
}

class RialtoClientWebAudioPlayerIpcPlayPauseTest : public WebAudioPlayerIpcTestBase
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
 * Test that Play can be called successfully.
 */
TEST_F(RialtoClientWebAudioPlayerIpcPlayPauseTest, PlaySuccess)
{
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("play"), m_controllerMock.get(),
                                           WebAudioPlayRequestMatcher(m_webAaudioPlayerHandle), _, m_blockingClosureMock.get()));
    EXPECT_EQ(m_webAudioPlayerIpc->play(), true);
}

/**
 * Test that Play fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcPlayPauseTest, PlayChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_webAudioPlayerIpc->play(), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that Play fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcPlayPauseTest, PlayReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("play"), _, _, _, _));

    EXPECT_EQ(m_webAudioPlayerIpc->play(), true);
}

/**
 * Test that Play fails when ipc fails.
 */
TEST_F(RialtoClientWebAudioPlayerIpcPlayPauseTest, PlayFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("play"), _, _, _, _));

    EXPECT_EQ(m_webAudioPlayerIpc->play(), false);
}

/**
 * Test that Pause can be called successfully.
 */
TEST_F(RialtoClientWebAudioPlayerIpcPlayPauseTest, PauseSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("pause"), m_controllerMock.get(),
                                           WebAudioPauseRequestMatcher(m_webAaudioPlayerHandle), _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_webAudioPlayerIpc->pause(), true);
}

/**
 * Test that Pause fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcPlayPauseTest, PauseChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_webAudioPlayerIpc->pause(), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that Pause fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcPlayPauseTest, PauseReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("pause"), _, _, _, _));

    EXPECT_EQ(m_webAudioPlayerIpc->pause(), true);
}

/**
 * Test that Pause fails when ipc fails.
 */
TEST_F(RialtoClientWebAudioPlayerIpcPlayPauseTest, PauseFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("pause"), _, _, _, _));

    EXPECT_EQ(m_webAudioPlayerIpc->pause(), false);
}
