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

class RialtoClientMediaPipelineIpcBufferingLimitTest : public MediaPipelineIpcTestBase
{
protected:
    const uint32_t m_kBufferingLimit{1234};

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
 * Test that SetBufferingLimit can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcBufferingLimitTest, SetBufferingLimitSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setBufferingLimit"), m_controllerMock.get(),
                                           setBufferingLimitRequestMatcher(m_sessionId, m_kBufferingLimit), _,
                                           m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->setBufferingLimit(m_kBufferingLimit), true);
}

/**
 * Test that SetBufferingLimit fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcBufferingLimitTest, SetBufferingLimitChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->setBufferingLimit(m_kBufferingLimit), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that SetBufferingLimit fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcBufferingLimitTest, SetBufferingLimitReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setBufferingLimit"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setBufferingLimit(m_kBufferingLimit), true);
}

/**
 * Test that SetBufferingLimit fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcBufferingLimitTest, SetBufferingLimitFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setBufferingLimit"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setBufferingLimit(m_kBufferingLimit), false);
}

/**
 * Test that GetBufferingLimit can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcBufferingLimitTest, GetBufferingLimitSuccess)
{
    uint32_t bufferingLimit{0};
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getBufferingLimit"), m_controllerMock.get(),
                                           getBufferingLimitRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [&](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                const google::protobuf::Message *, google::protobuf::Message *response, google::protobuf::Closure *)
            {
                ::firebolt::rialto::GetBufferingLimitResponse *resp =
                    dynamic_cast<::firebolt::rialto::GetBufferingLimitResponse *>(response);
                resp->set_limit_buffering_ms(m_kBufferingLimit);
            }));

    EXPECT_EQ(m_mediaPipelineIpc->getBufferingLimit(bufferingLimit), true);
    EXPECT_EQ(m_kBufferingLimit, bufferingLimit);
}

/**
 * Test that GetBufferingLimit fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcBufferingLimitTest, GetBufferingLimitChannelDisconnected)
{
    uint32_t bufferingLimit{0};
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->getBufferingLimit(bufferingLimit), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that GetBufferingLimit fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcBufferingLimitTest, GetBufferingLimitReconnectChannel)
{
    uint32_t bufferingLimit{0};
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getBufferingLimit"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->getBufferingLimit(bufferingLimit), true);
}

/**
 * Test that GetBufferingLimit fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcBufferingLimitTest, GetBufferingLimitFailure)
{
    uint32_t bufferingLimit{0};
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getBufferingLimit"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->getBufferingLimit(bufferingLimit), false);
}
