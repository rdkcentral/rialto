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

MATCHER_P2(SetPlaybackRateRequestMatcher, sessionId, rate, "")
{
    const ::firebolt::rialto::SetPlaybackRateRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::SetPlaybackRateRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->rate() == rate));
}

class RialtoClientMediaPipelineIpcSetPlaybackRateTest : public MediaPipelineIpcTestBase
{
protected:
    double m_rate = 1.5;

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
 * Test that setPlaybackRate can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSetPlaybackRateTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("setPlaybackRate"), m_controllerMock.get(),
                           SetPlaybackRateRequestMatcher(m_sessionId, m_rate), _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->setPlaybackRate(m_rate), true);
}

/**
 * Test that setPlaybackRate fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetPlaybackRateTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->setPlaybackRate(m_rate), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setPlaybackRate fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetPlaybackRateTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setPlaybackRate"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setPlaybackRate(m_rate), true);
}

/**
 * Test that setPlaybackRate fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSetPlaybackRateTest, SetPlaybackRateFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setPlaybackRate"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setPlaybackRate(m_rate), false);
}
