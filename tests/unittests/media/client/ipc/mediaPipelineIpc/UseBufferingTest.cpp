/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

class RialtoClientMediaPipelineIpcUseBufferingTest : public MediaPipelineIpcTestBase
{
protected:
    const bool m_kUseBuffering{true};

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
 * Test that SetUseBuffering can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcUseBufferingTest, SetUseBufferingSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setUseBuffering"), m_controllerMock.get(),
                                           setUseBufferingRequestMatcher(m_sessionId, m_kUseBuffering), _,
                                           m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->setUseBuffering(m_kUseBuffering), true);
}

/**
 * Test that SetUseBuffering fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcUseBufferingTest, SetUseBufferingChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->setUseBuffering(m_kUseBuffering), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that SetUseBuffering fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcUseBufferingTest, SetUseBufferingReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setUseBuffering"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setUseBuffering(m_kUseBuffering), true);
}

/**
 * Test that SetUseBuffering fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcUseBufferingTest, SetUseBufferingFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setUseBuffering"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setUseBuffering(m_kUseBuffering), false);
}

/**
 * Test that GetUseBuffering can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcUseBufferingTest, GetUseBufferingSuccess)
{
    bool useBuffering{false};
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getUseBuffering"), m_controllerMock.get(),
                                           getUseBufferingRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [&](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                const google::protobuf::Message *, google::protobuf::Message *response, google::protobuf::Closure *)
            {
                ::firebolt::rialto::GetUseBufferingResponse *resp =
                    dynamic_cast<::firebolt::rialto::GetUseBufferingResponse *>(response);
                resp->set_use_buffering(m_kUseBuffering);
            }));

    EXPECT_EQ(m_mediaPipelineIpc->getUseBuffering(useBuffering), true);
    EXPECT_EQ(m_kUseBuffering, useBuffering);
}

/**
 * Test that GetUseBuffering fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcUseBufferingTest, GetUseBufferingChannelDisconnected)
{
    bool useBuffering{false};
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->getUseBuffering(useBuffering), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that GetUseBuffering fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcUseBufferingTest, GetUseBufferingReconnectChannel)
{
    bool useBuffering{false};
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getUseBuffering"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->getUseBuffering(useBuffering), true);
}

/**
 * Test that GetUseBuffering fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcUseBufferingTest, GetUseBufferingFailure)
{
    bool useBuffering{false};
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getUseBuffering"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->getUseBuffering(useBuffering), false);
}
