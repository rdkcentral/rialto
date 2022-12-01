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

MATCHER_P(RenderFrameRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::RenderFrameRequest *request =
        dynamic_cast<const ::firebolt::rialto::RenderFrameRequest *>(arg);
    return ((request->session_id() == sessionId));
}

class RialtoClientMediaPipelineIpcRenderFrameTest : public MediaPipelineIpcTestBase
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

TEST_F(RialtoClientMediaPipelineIpcRenderFrameTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("renderFrame"), m_controllerMock.get(),
                                           RenderFrameRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()));

    EXPECT_TRUE(m_mediaPipelineIpc->renderFrame());
}

TEST_F(RialtoClientMediaPipelineIpcRenderFrameTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_FALSE(m_mediaPipelineIpc->renderFrame());

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

TEST_F(RialtoClientMediaPipelineIpcRenderFrameTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("renderFrame"), _, _, _, _));

    EXPECT_TRUE(m_mediaPipelineIpc->renderFrame());
}

TEST_F(RialtoClientMediaPipelineIpcRenderFrameTest, SetPlaybackRateFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("renderFrame"), _, _, _, _));

    EXPECT_FALSE(m_mediaPipelineIpc->renderFrame());
}