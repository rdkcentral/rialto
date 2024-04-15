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

class RialtoClientMediaPipelineIpcSetSourcePositionTest : public MediaPipelineIpcTestBase
{
protected:
    const int64_t m_kPosition = 123;
    const int32_t m_kSourceId{1};

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
 * Test that setSourcePosition can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSetSourcePositionTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setSourcePosition"), m_controllerMock.get(),
                                           setSourcePositionRequestMatcher(m_sessionId, m_kSourceId, m_kPosition), _,
                                           m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->setSourcePosition(m_kSourceId, m_kPosition), true);
}

/**
 * Test that setSourcePosition fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetSourcePositionTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->setSourcePosition(m_kSourceId, m_kPosition), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setSourcePosition fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetSourcePositionTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setSourcePosition"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setSourcePosition(m_kSourceId, m_kPosition), true);
}

/**
 * Test that setSourcePosition fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSetSourcePositionTest, SetSourcePositionFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setSourcePosition"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setSourcePosition(m_kSourceId, m_kPosition), false);
}
