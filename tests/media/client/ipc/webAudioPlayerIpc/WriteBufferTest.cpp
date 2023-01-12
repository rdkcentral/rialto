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

#include "WebAudioPlayerIpcTestBase.h"

MATCHER_P2(WebAudioWriteBufferRequestMatcher, web_audio_player_handle, number_of_frames, "")
{
    const ::firebolt::rialto::WebAudioWriteBufferRequest *request =
        dynamic_cast<const ::firebolt::rialto::WebAudioWriteBufferRequest *>(arg);
    return ((request->web_audio_player_handle() == web_audio_player_handle) &&
            (request->number_of_frames() == number_of_frames));
}

class RialtoClientWebAudioPlayerIpcWriteBufferTest : public WebAudioPlayerIpcTestBase
{
protected:
    uint32_t m_numberOfFrames = 7;

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
 * Test that writeBuffer can be called successfully.
 */
TEST_F(RialtoClientWebAudioPlayerIpcWriteBufferTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("writeBuffer"), m_controllerMock.get(),
                                           WebAudioWriteBufferRequestMatcher(m_webAaudioPlayerHandle, m_numberOfFrames),
                                           _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_webAudioPlayerIpc->writeBuffer(m_numberOfFrames), true);
}

/**
 * Test that writeBuffer fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcWriteBufferTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_webAudioPlayerIpc->writeBuffer(m_numberOfFrames), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that writeBuffer fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcWriteBufferTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("writeBuffer"), _, _, _, _));

    EXPECT_EQ(m_webAudioPlayerIpc->writeBuffer(m_numberOfFrames), true);
}

/**
 * Test that writeBuffer fails when ipc fails.
 */
TEST_F(RialtoClientWebAudioPlayerIpcWriteBufferTest, WriteBufferFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("writeBuffer"), _, _, _, _));

    EXPECT_EQ(m_webAudioPlayerIpc->writeBuffer(m_numberOfFrames), false);
}
