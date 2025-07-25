/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

class RialtoClientMediaPipelineIpcIsVideoMasterTest : public MediaPipelineIpcTestBase
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
 * Test that isVideoMaster can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcIsVideoMasterTest, Success)
{
    constexpr bool kIsVideoMaster{true};
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isVideoMaster"), m_controllerMock.get(),
                                           isVideoMasterRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [&](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                const google::protobuf::Message *, google::protobuf::Message *response, google::protobuf::Closure *)
            {
                ::firebolt::rialto::IsVideoMasterResponse *resp =
                    dynamic_cast<::firebolt::rialto::IsVideoMasterResponse *>(response);
                resp->set_is_video_master(kIsVideoMaster);
            }));

    bool resultIsVideoMaster{false};
    EXPECT_TRUE(m_mediaPipelineIpc->isVideoMaster(resultIsVideoMaster));
    EXPECT_EQ(resultIsVideoMaster, kIsVideoMaster);
}

/**
 * Test that isVideoMaster fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcIsVideoMasterTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    bool isVideoMaster{false};
    EXPECT_FALSE(m_mediaPipelineIpc->isVideoMaster(isVideoMaster));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that isVideoMaster fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcIsVideoMasterTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isVideoMaster"), _, _, _, _));

    bool isVideoMaster{false};
    EXPECT_TRUE(m_mediaPipelineIpc->isVideoMaster(isVideoMaster));
}

/**
 * Test that isVideoMaster fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcIsVideoMasterTest, IsVideoMasterFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isVideoMaster"), _, _, _, _));

    bool isVideoMaster{false};
    EXPECT_FALSE(m_mediaPipelineIpc->isVideoMaster(isVideoMaster));
}
