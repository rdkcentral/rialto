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

class RialtoClientMediaPipelineIpcSetLowLatencyTest : public MediaPipelineIpcTestBase
{
protected:
    bool m_lowLatency{false};

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
 * Test that setLowLatency can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSetLowLatencyTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setLowLatency"), m_controllerMock.get(),
                                           setLowLatencyRequestMatcher(m_sessionId, m_lowLatency), _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->setLowLatency(m_lowLatency), true);
}

/**
 * Test that setLowLatency fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetLowLatencyTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->setLowLatency(m_lowLatency), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setLowLatency fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetLowLatencyTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setLowLatency"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setLowLatency(m_lowLatency), true);
}

/**
 * Test that setLowLatency fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSetLowLatencyTest, SetLowLatencyFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setLowLatency"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setLowLatency(m_lowLatency), false);
}
