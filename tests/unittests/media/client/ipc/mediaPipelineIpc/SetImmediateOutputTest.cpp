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

class RialtoClientMediaPipelineIpcSetImmediateOutputTest : public MediaPipelineIpcTestBase
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

    const int32_t m_kSourceId{1};
};

/**
 * Test that setImmediateOutput can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSetImmediateOutputTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("setImmediateOutput"), m_controllerMock.get(),
                           setImmediateOutputRequestMatcher(m_sessionId, m_kSourceId), _, m_blockingClosureMock.get()));

    EXPECT_TRUE(m_mediaPipelineIpc->setImmediateOutput(m_kSourceId, true));
}

/**
 * Test that setImmediateOutput fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetImmediateOutputTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_FALSE(m_mediaPipelineIpc->setImmediateOutput(m_kSourceId, true));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setImmediateOutput fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetImmediateOutputTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setImmediateOutput"), _, _, _, _));

    EXPECT_TRUE(m_mediaPipelineIpc->setImmediateOutput(m_kSourceId, true));
}

/**
 * Test that setImmediateOutput fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSetImmediateOutputTest, SetImmediateOutputFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setImmediateOutput"), _, _, _, _));

    EXPECT_FALSE(m_mediaPipelineIpc->setImmediateOutput(m_kSourceId, true));
}
