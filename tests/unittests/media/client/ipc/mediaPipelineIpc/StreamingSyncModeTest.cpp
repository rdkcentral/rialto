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

class RialtoClientMediaPipelineIpcStreamingSyncModeTest : public MediaPipelineIpcTestBase
{
protected:
    const bool m_kEnabled{false};

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
 * Test that SetStreamingSyncMode can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcStreamingSyncModeTest, SetStreamingSyncModeSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setStreamingSyncMode"), m_controllerMock.get(),
                                           setStreamingSyncModeRequestMatcher(m_sessionId, m_kEnabled), _,
                                           m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->setStreamingSyncMode(m_kEnabled), true);
}

/**
 * Test that SetStreamingSyncMode fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcStreamingSyncModeTest, SetStreamingSyncModeChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->setStreamingSyncMode(m_kEnabled), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that SetStreamingSyncMode fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcStreamingSyncModeTest, SetStreamingSyncModeReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setStreamingSyncMode"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setStreamingSyncMode(m_kEnabled), true);
}

/**
 * Test that SetStreamingSyncMode fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcStreamingSyncModeTest, SetStreamingSyncModeFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setStreamingSyncMode"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setStreamingSyncMode(m_kEnabled), false);
}
