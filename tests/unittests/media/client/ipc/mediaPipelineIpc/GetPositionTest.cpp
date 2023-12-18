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
#include "MediaPipelineProtoRequestMatchers.h"

class RialtoClientMediaPipelineIpcGetPositionTest : public MediaPipelineIpcTestBase
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
 * Test that getPosition can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcGetPositionTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getPosition"), m_controllerMock.get(),
                                           GetPositionRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()));

    int64_t position;
    EXPECT_TRUE(m_mediaPipelineIpc->getPosition(position));
}

/**
 * Test that getPosition fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcGetPositionTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    int64_t position;
    EXPECT_FALSE(m_mediaPipelineIpc->getPosition(position));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getPosition fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcGetPositionTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getPosition"), _, _, _, _));

    int64_t position;
    EXPECT_TRUE(m_mediaPipelineIpc->getPosition(position));
}

/**
 * Test that getPosition fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcGetPositionTest, GetPositionFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getPosition"), _, _, _, _));

    int64_t position;
    EXPECT_FALSE(m_mediaPipelineIpc->getPosition(position));
}
