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

class RialtoClientMediaPipelineIpcSetSyncTest : public MediaPipelineIpcTestBase
{
protected:
    bool m_sync{false};

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
 * Test that setSync can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSetSyncTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setSync"), m_controllerMock.get(),
                                           setSyncRequestMatcher(m_sessionId, m_sync), _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->setSync(m_sync), true);
}

/**
 * Test that setSync fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetSyncTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->setSync(m_sync), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setSync fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetSyncTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setSync"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setSync(m_sync), true);
}

/**
 * Test that setSync fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSetSyncTest, SetSyncFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setSync"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setSync(m_sync), false);
}
