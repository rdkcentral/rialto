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

class RialtoClientMediaPipelineIpcGetSyncTest : public MediaPipelineIpcTestBase
{
protected:
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
 * Test that getSync can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcGetSyncTest, Success)
{
    constexpr bool kSync{false};
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSync"), m_controllerMock.get(),
                                           getSyncRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [&](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                const google::protobuf::Message *, google::protobuf::Message *response, google::protobuf::Closure *)
            {
                ::firebolt::rialto::GetSyncResponse *resp = dynamic_cast<::firebolt::rialto::GetSyncResponse *>(response);
                resp->set_sync(kSync);
            }));

    bool resultSync;
    EXPECT_TRUE(m_mediaPipelineIpc->getSync(resultSync));
    EXPECT_EQ(resultSync, kSync);
}

/**
 * Test that getSync fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcGetSyncTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    bool sync;
    EXPECT_FALSE(m_mediaPipelineIpc->getSync(sync));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getSync fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcGetSyncTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSync"), _, _, _, _));

    bool sync;
    EXPECT_TRUE(m_mediaPipelineIpc->getSync(sync));
}

/**
 * Test that getSync fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcGetSyncTest, GetSyncFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSync"), _, _, _, _));

    bool sync;
    EXPECT_FALSE(m_mediaPipelineIpc->getSync(sync));
}
