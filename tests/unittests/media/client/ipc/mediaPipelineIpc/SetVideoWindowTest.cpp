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

class RialtoClientMediaPipelineIpcSetVideoWindowTest : public MediaPipelineIpcTestBase
{
protected:
    uint32_t m_x = 1;
    uint32_t m_y = 2;
    uint32_t m_width = 3;
    uint32_t m_height = 4;

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
};

/**
 * Test that setVideoWindow can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSetVideoWindowTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setVideoWindow"), m_controllerMock.get(),
                                           setVideoWindowRequestMatcher(m_sessionId, m_x, m_y, m_width, m_height), _,
                                           m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->setVideoWindow(m_x, m_y, m_width, m_height), true);
}

/**
 * Test that setVideoWindow fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetVideoWindowTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->setVideoWindow(m_x, m_y, m_width, m_height), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setVideoWindow fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetVideoWindowTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setVideoWindow"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setVideoWindow(m_x, m_y, m_width, m_height), true);
}

/**
 * Test that setVideoWindow fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSetVideoWindowTest, SetVideoWindowFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setVideoWindow"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setVideoWindow(m_x, m_y, m_width, m_height), false);
}
