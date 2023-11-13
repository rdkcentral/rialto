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

MATCHER_P(WebAudioGetBufferDelayRequestMatcher, webAaudioPlayerHandle, "")
{
    const ::firebolt::rialto::WebAudioGetBufferDelayRequest *request =
        dynamic_cast<const ::firebolt::rialto::WebAudioGetBufferDelayRequest *>(arg);
    return ((request->web_audio_player_handle() == webAaudioPlayerHandle));
}

class RialtoClientWebAudioPlayerIpcGetBufferDelayTest : public WebAudioPlayerIpcTestBase
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
 * Test that getBufferDelay can be called successfully.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetBufferDelayTest, Success)
{
    constexpr uint32_t kDelayFrames{7};
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getBufferDelay"), m_controllerMock.get(),
                                           WebAudioGetBufferDelayRequestMatcher(m_webAaudioPlayerHandle), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [&](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                const google::protobuf::Message *, google::protobuf::Message *response, google::protobuf::Closure *)
            {
                ::firebolt::rialto::WebAudioGetBufferDelayResponse *resp =
                    dynamic_cast<::firebolt::rialto::WebAudioGetBufferDelayResponse *>(response);
                resp->set_delay_frames(kDelayFrames);
            }));

    uint32_t result;
    EXPECT_TRUE(m_webAudioPlayerIpc->getBufferDelay(result));
    EXPECT_EQ(result, kDelayFrames);
}

/**
 * Test that getBufferDelay fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetBufferDelayTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    uint32_t delayFrames;
    EXPECT_FALSE(m_webAudioPlayerIpc->getBufferDelay(delayFrames));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getBufferDelay fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetBufferDelayTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getBufferDelay"), _, _, _, _));

    uint32_t delayFrames;
    EXPECT_TRUE(m_webAudioPlayerIpc->getBufferDelay(delayFrames));
}

/**
 * Test that getBufferDelay fails when ipc fails.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetBufferDelayTest, GetBufferDelayFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getBufferDelay"), _, _, _, _));

    uint32_t delayFrames;
    EXPECT_FALSE(m_webAudioPlayerIpc->getBufferDelay(delayFrames));
}
