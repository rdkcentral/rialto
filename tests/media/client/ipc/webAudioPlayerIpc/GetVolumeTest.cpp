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

MATCHER_P(WebAudioGetVolumeRequestMatcher, webAaudioPlayerHandle, "")
{
    const ::firebolt::rialto::WebAudioGetVolumeRequest *request = dynamic_cast<const ::firebolt::rialto::WebAudioGetVolumeRequest *>(arg);
    return ((request->web_audio_player_handle() == webAaudioPlayerHandle));
}

class RialtoClientWebAudioPlayerIpcGetVolumeTest : public WebAudioPlayerIpcTestBase
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
 * Test that getVolume can be called successfully.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetVolumeTest, Success)
{
    constexpr double kVolume{0.7};
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getVolume"), m_controllerMock.get(),
                                           WebAudioGetVolumeRequestMatcher(m_webAaudioPlayerHandle), _, m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [&](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                const google::protobuf::Message *, google::protobuf::Message *response, google::protobuf::Closure *)
            {
                ::firebolt::rialto::WebAudioGetVolumeResponse *resp =
                    dynamic_cast<::firebolt::rialto::WebAudioGetVolumeResponse *>(response);
                resp->set_volume(kVolume);
            }));

    double resultVolume;
    EXPECT_TRUE(m_webAudioPlayerIpc->getVolume(resultVolume));
    EXPECT_EQ(resultVolume, kVolume);
}

/**
 * Test that getVolume fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetVolumeTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    double volume;
    EXPECT_FALSE(m_webAudioPlayerIpc->getVolume(volume));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getVolume fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetVolumeTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getVolume"), _, _, _, _));

    double volume;
    EXPECT_TRUE(m_webAudioPlayerIpc->getVolume(volume));
}

/**
 * Test that getVolume fails when ipc fails.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetVolumeTest, GetVolumeFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getVolume"), _, _, _, _));

    double volume;
    EXPECT_FALSE(m_webAudioPlayerIpc->getVolume(volume));
}
