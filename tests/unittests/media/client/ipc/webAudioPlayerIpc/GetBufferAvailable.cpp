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

MATCHER_P(WebAudioGetBufferAvailableRequestMatcher, webAaudioPlayerHandle, "")
{
    const ::firebolt::rialto::WebAudioGetBufferAvailableRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::WebAudioGetBufferAvailableRequest *>(arg);
    return ((kRequest->web_audio_player_handle() == webAaudioPlayerHandle));
}

class RialtoClientWebAudioPlayerIpcGetBufferAvailableTest : public WebAudioPlayerIpcTestBase
{
protected:
    uint32_t m_availableFrames{0};
    std::shared_ptr<WebAudioShmInfo> m_webAudioShmInfo;

    virtual void SetUp()
    {
        WebAudioPlayerIpcTestBase::SetUp();
        createWebAudioPlayerIpc();
        m_webAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    }

    virtual void TearDown()
    {
        destroyWebAudioPlayerIpc();
        WebAudioPlayerIpcTestBase::TearDown();
    }
};

/**
 * Test that getBufferAvailable can be called successfully.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetBufferAvailableTest, Success)
{
    constexpr uint32_t kAvailableFrames = 7;

    constexpr uint32_t kOffsetMain = 17;
    constexpr uint32_t kLengthMain = 13;
    constexpr uint32_t kOffsetWrap = 11;
    constexpr uint32_t kLengthWrap = 34;

    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getBufferAvailable"), m_controllerMock.get(),
                                           WebAudioGetBufferAvailableRequestMatcher(m_webAaudioPlayerHandle), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [&](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                const google::protobuf::Message *, google::protobuf::Message *response, google::protobuf::Closure *)
            {
                ::firebolt::rialto::WebAudioGetBufferAvailableResponse *resp =
                    dynamic_cast<::firebolt::rialto::WebAudioGetBufferAvailableResponse *>(response);
                resp->set_available_frames(kAvailableFrames);
                resp->mutable_shm_info()->set_offset_main(kOffsetMain);
                resp->mutable_shm_info()->set_length_main(kLengthMain);
                resp->mutable_shm_info()->set_offset_wrap(kOffsetWrap);
                resp->mutable_shm_info()->set_length_wrap(kLengthWrap);
            }));

    EXPECT_TRUE(m_webAudioPlayerIpc->getBufferAvailable(m_availableFrames, m_webAudioShmInfo));
    EXPECT_EQ(kAvailableFrames, m_availableFrames);
    EXPECT_EQ(kOffsetMain, m_webAudioShmInfo->offsetMain);
    EXPECT_EQ(kLengthMain, m_webAudioShmInfo->lengthMain);
    EXPECT_EQ(kOffsetWrap, m_webAudioShmInfo->offsetWrap);
    EXPECT_EQ(kLengthWrap, m_webAudioShmInfo->lengthWrap);
}

/**
 * Test that getBufferAvailable fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetBufferAvailableTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_FALSE(m_webAudioPlayerIpc->getBufferAvailable(m_availableFrames, m_webAudioShmInfo));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getBufferAvailable fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetBufferAvailableTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getBufferAvailable"), _, _, _, _));

    EXPECT_TRUE(m_webAudioPlayerIpc->getBufferAvailable(m_availableFrames, m_webAudioShmInfo));
}

/**
 * Test that getBufferAvailable fails when ipc fails.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetBufferAvailableTest, GetBufferAvailableFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getBufferAvailable"), _, _, _, _));

    EXPECT_FALSE(m_webAudioPlayerIpc->getBufferAvailable(m_availableFrames, m_webAudioShmInfo));
}

/**
 * Test when webAudioShmInfo parameter is null.
 */
TEST_F(RialtoClientWebAudioPlayerIpcGetBufferAvailableTest, nullShmInfoParameter)
{
    m_webAudioShmInfo.reset();
    EXPECT_FALSE(m_webAudioPlayerIpc->getBufferAvailable(m_availableFrames, m_webAudioShmInfo));
}
