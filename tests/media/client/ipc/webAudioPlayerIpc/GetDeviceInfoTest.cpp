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

MATCHER_P(WebAudioGetDeviceInfoRequestMatcher, webAaudioPlayerHandle, "")
{
    const ::firebolt::rialto::WebAudioGetDeviceInfoRequest *request =
        dynamic_cast<const ::firebolt::rialto::WebAudioGetDeviceInfoRequest *>(arg);
    return ((request->web_audio_player_handle() == webAaudioPlayerHandle));
}

class RialtoClientWebAudioPlayerIpcGetDeviceInfoTest : public WebAudioPlayerIpcTestBase
{
protected:
    uint32_t m_preferredFrames{0};
    uint32_t m_maximumFrames{0};
    bool m_supportDeferredPlay{false};

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
 * Test that getDeviceInfo can be called successfully.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetDeviceInfoTest, Success)
{
    constexpr uint32_t kPreferredFrames = 7;
    constexpr uint32_t kMaximumFrames = 70;
    constexpr bool kSupportDeferredPlay = true;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getDeviceInfo"), m_controllerMock.get(),
                                           WebAudioGetDeviceInfoRequestMatcher(m_webAaudioPlayerHandle), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [&](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                const google::protobuf::Message *, google::protobuf::Message *response, google::protobuf::Closure *)
            {
                ::firebolt::rialto::WebAudioGetDeviceInfoResponse *resp =
                    dynamic_cast<::firebolt::rialto::WebAudioGetDeviceInfoResponse *>(response);
                resp->set_preferred_frames(kPreferredFrames);
                resp->set_maximum_frames(kMaximumFrames);
                resp->set_support_deferred_play(kSupportDeferredPlay);
            }));

    EXPECT_TRUE(m_webAudioPlayerIpc->getDeviceInfo(m_preferredFrames, m_maximumFrames, m_supportDeferredPlay));
    EXPECT_EQ(kPreferredFrames, m_preferredFrames);
    EXPECT_EQ(kMaximumFrames, m_maximumFrames);
    EXPECT_EQ(kSupportDeferredPlay, m_supportDeferredPlay);
}

/**
 * Test that getDeviceInfo fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetDeviceInfoTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_FALSE(m_webAudioPlayerIpc->getDeviceInfo(m_preferredFrames, m_maximumFrames, m_supportDeferredPlay));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getDeviceInfo fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetDeviceInfoTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getDeviceInfo"), _, _, _, _));

    EXPECT_TRUE(m_webAudioPlayerIpc->getDeviceInfo(m_preferredFrames, m_maximumFrames, m_supportDeferredPlay));
}

/**
 * Test that getDeviceInfo fails when ipc fails.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetDeviceInfoTest, GetDeviceInfoFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getDeviceInfo"), _, _, _, _));

    EXPECT_FALSE(m_webAudioPlayerIpc->getDeviceInfo(m_preferredFrames, m_maximumFrames, m_supportDeferredPlay));
}
