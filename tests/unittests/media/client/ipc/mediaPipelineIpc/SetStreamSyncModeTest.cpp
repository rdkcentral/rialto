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

class RialtoClientMediaPipelineIpcSetStreamSyncModeTest : public MediaPipelineIpcTestBase
{
protected:
    int32_t m_streamSyncMode{1};

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
 * Test that setStreamSyncMode can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSetStreamSyncModeTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setStreamSyncMode"), m_controllerMock.get(),
                                           setStreamSyncModeRequestMatcher(m_sessionId, m_streamSyncMode), _,
                                           m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->setStreamSyncMode(m_streamSyncMode), true);
}

/**
 * Test that setStreamSyncMode fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetStreamSyncModeTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->setStreamSyncMode(m_streamSyncMode), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setStreamSyncMode fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetStreamSyncModeTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setStreamSyncMode"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setStreamSyncMode(m_streamSyncMode), true);
}

/**
 * Test that setStreamSyncMode fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSetStreamSyncModeTest, SetStreamSyncModeFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setStreamSyncMode"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setStreamSyncMode(m_streamSyncMode), false);
}
