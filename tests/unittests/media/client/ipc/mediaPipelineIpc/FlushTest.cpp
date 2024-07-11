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

class RialtoClientMediaPipelineIpcFlushTest : public MediaPipelineIpcTestBase
{
protected:
    virtual void SetUp()
    {
        MediaPipelineIpcTestBase::SetUp();

        EXPECT_CALL(*m_eventThread, flush());
        createMediaPipelineIpc();
    }

    virtual void TearDown()
    {
        destroyMediaPipelineIpc();

        MediaPipelineIpcTestBase::TearDown();
    }

    const int32_t m_kSourceId{1};
    const bool m_kResetTime{true};
};

/**
 * Test that Flush can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcFlushTest, FlushSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("flush"), m_controllerMock.get(),
                                           flushRequestMatcher(m_sessionId, m_kSourceId, m_kResetTime), _,
                                           m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->flush(m_kSourceId, m_kResetTime), true);
}

/**
 * Test that Flush fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcFlushTest, FlushChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->flush(m_kSourceId, m_kResetTime), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that Flush fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcFlushTest, FlushReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("flush"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->flush(m_kSourceId, m_kResetTime), true);
}

/**
 * Test that Flush fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcFlushTest, FlushFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("flush"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->flush(m_kSourceId, m_kResetTime), false);
}
