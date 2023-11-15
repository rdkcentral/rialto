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

#include "MediaPipelineIpcTestBase.h"

MATCHER_P2(SetPositionRequestMatcher, sessionId, position, "")
{
    const ::firebolt::rialto::SetPositionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::SetPositionRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->position() == position));
}

class RialtoClientMediaPipelineIpcSetPositionTest : public MediaPipelineIpcTestBase
{
protected:
    int64_t m_position = 123;

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
 * Test that setPosition can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSetPositionTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("setPosition"), m_controllerMock.get(),
                           SetPositionRequestMatcher(m_sessionId, m_position), _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->setPosition(m_position), true);
}

/**
 * Test that setPosition fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetPositionTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->setPosition(m_position), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setPosition fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetPositionTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setPosition"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setPosition(m_position), true);
}

/**
 * Test that setPosition fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSetPositionTest, SetPositionFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setPosition"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setPosition(m_position), false);
}
