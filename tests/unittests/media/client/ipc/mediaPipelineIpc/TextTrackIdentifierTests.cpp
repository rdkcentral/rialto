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
#include <string>

class RialtoClientMediaPipelineIpcTextTrackIdentifierTest : public MediaPipelineIpcTestBase
{
protected:
    const std::string m_kTextTrackIdentifier{"Identifier"};

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
 * Test that SetTextTrackIdentifier can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcTextTrackIdentifierTest, SetSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setTextTrackIdentifier"), m_controllerMock.get(),
                                           setTextTrackIdentifierRequestMatcher(m_sessionId, m_kTextTrackIdentifier), _,
                                           m_blockingClosureMock.get()));

    EXPECT_TRUE(m_mediaPipelineIpc->setTextTrackIdentifier(m_kTextTrackIdentifier));
}

/**
 * Test that SetTextTrackIdentifier fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcTextTrackIdentifierTest, SetChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_FALSE(m_mediaPipelineIpc->setTextTrackIdentifier(m_kTextTrackIdentifier));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that SetTextTrackIdentifier fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcTextTrackIdentifierTest, SetReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setTextTrackIdentifier"), _, _, _, _));

    EXPECT_TRUE(m_mediaPipelineIpc->setTextTrackIdentifier(m_kTextTrackIdentifier));
}

/**
 * Test that SetTextTrackIdentifier fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcTextTrackIdentifierTest, SetTextTrackIdentifierFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setTextTrackIdentifier"), _, _, _, _));

    EXPECT_FALSE(m_mediaPipelineIpc->setTextTrackIdentifier(m_kTextTrackIdentifier));
}

/**
 * Test that GetTextTrackIdentifier can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcTextTrackIdentifierTest, GetSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("getTextTrackIdentifier"), m_controllerMock.get(),
                           getTextTrackIdentifierRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [&](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                const google::protobuf::Message *, google::protobuf::Message *response, google::protobuf::Closure *)
            {
                ::firebolt::rialto::GetTextTrackIdentifierResponse *resp =
                    dynamic_cast<::firebolt::rialto::GetTextTrackIdentifierResponse *>(response);
                resp->set_text_track_identifier(m_kTextTrackIdentifier);
            }));

    std::string textTrackIdentifier;
    EXPECT_TRUE(m_mediaPipelineIpc->getTextTrackIdentifier(textTrackIdentifier));
    EXPECT_EQ(textTrackIdentifier, m_kTextTrackIdentifier);
}

/**
 * Test that GetTextTrackIdentifier fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcTextTrackIdentifierTest, GetChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    std::string textTrackIdentifier;
    EXPECT_FALSE(m_mediaPipelineIpc->getTextTrackIdentifier(textTrackIdentifier));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that GetTextTrackIdentifier fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcTextTrackIdentifierTest, GetReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getTextTrackIdentifier"), _, _, _, _));

    std::string textTrackIdentifier;
    EXPECT_TRUE(m_mediaPipelineIpc->getTextTrackIdentifier(textTrackIdentifier));
}

/**
 * Test that GetTextTrackIdentifier fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcTextTrackIdentifierTest, GetTextTrackIdentifierFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getTextTrackIdentifier"), _, _, _, _));

    std::string textTrackIdentifier;
    EXPECT_FALSE(m_mediaPipelineIpc->getTextTrackIdentifier(textTrackIdentifier));
}
