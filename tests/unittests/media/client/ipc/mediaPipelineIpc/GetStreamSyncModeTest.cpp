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

class RialtoClientMediaPipelineIpcGetStreamSyncModeTest : public MediaPipelineIpcTestBase
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
 * Test that getStreamSyncMode can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcGetStreamSyncModeTest, Success)
{
    constexpr int32_t kStreamSyncMode{1};
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getStreamSyncMode"), m_controllerMock.get(),
                                           getStreamSyncModeRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [&](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                const google::protobuf::Message *, google::protobuf::Message *response, google::protobuf::Closure *)
            {
                ::firebolt::rialto::GetStreamSyncModeResponse *resp =
                    dynamic_cast<::firebolt::rialto::GetStreamSyncModeResponse *>(response);
                resp->set_stream_sync_mode(kStreamSyncMode);
            }));

    int32_t resultStreamSyncMode;
    EXPECT_TRUE(m_mediaPipelineIpc->getStreamSyncMode(resultStreamSyncMode));
    EXPECT_EQ(resultStreamSyncMode, kStreamSyncMode);
}

/**
 * Test that getStreamSyncMode fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcGetStreamSyncModeTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    int32_t streamSyncMode;
    EXPECT_FALSE(m_mediaPipelineIpc->getStreamSyncMode(streamSyncMode));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getStreamSyncMode fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcGetStreamSyncModeTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getStreamSyncMode"), _, _, _, _));

    int32_t streamSyncMode;
    EXPECT_TRUE(m_mediaPipelineIpc->getStreamSyncMode(streamSyncMode));
}

/**
 * Test that getStreamSyncMode fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcGetStreamSyncModeTest, GetStreamSyncModeFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getStreamSyncMode"), _, _, _, _));

    int32_t streamSyncMode;
    EXPECT_FALSE(m_mediaPipelineIpc->getStreamSyncMode(streamSyncMode));
}
