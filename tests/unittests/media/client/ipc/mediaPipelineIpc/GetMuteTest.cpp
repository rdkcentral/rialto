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

#include "MediaPipelineIpcTestBase.h"
#include "MediaPipelineProtoRequestMatchers.h"

class RialtoClientMediaPipelineIpcGetMuteTest : public MediaPipelineIpcTestBase
{
protected:
    const int32_t m_kSourceId{1};

    virtual void SetUp()
    {
        MediaPipelineIpcTestBase::SetUp();

        createMediaPipelineIpc();
    }

    virtual void TearDown()
    {
        destroyMediaPipelineIpc();

        MediaPipelineIpcTestBase::TearDown();
    }
};

/**
 * Test that getMute can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcGetMuteTest, Success)
{
    constexpr bool kMute{false};
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getMute"), m_controllerMock.get(),
                                           getMuteRequestMatcher(m_sessionId, m_kSourceId), _, m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [&](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                const google::protobuf::Message *, google::protobuf::Message *response, google::protobuf::Closure *)
            {
                ::firebolt::rialto::GetMuteResponse *resp = dynamic_cast<::firebolt::rialto::GetMuteResponse *>(response);
                resp->set_mute(kMute);
            }));

    bool resultMute;
    EXPECT_TRUE(m_mediaPipelineIpc->getMute(m_kSourceId, resultMute));
    EXPECT_EQ(resultMute, kMute);
}

/**
 * Test that getMute fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcGetMuteTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    bool mute;
    EXPECT_FALSE(m_mediaPipelineIpc->getMute(m_kSourceId, mute));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getMute fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcGetMuteTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getMute"), _, _, _, _));

    bool mute;
    EXPECT_TRUE(m_mediaPipelineIpc->getMute(m_kSourceId, mute));
}

/**
 * Test that getMute fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcGetMuteTest, GetMuteFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getMute"), _, _, _, _));

    bool mute;
    EXPECT_FALSE(m_mediaPipelineIpc->getMute(m_kSourceId, mute));
}
