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

MATCHER_P(PlayRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::PlayRequest *request = dynamic_cast<const ::firebolt::rialto::PlayRequest *>(arg);
    return (request->session_id() == sessionId);
}

MATCHER_P(PauseRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::PauseRequest *request = dynamic_cast<const ::firebolt::rialto::PauseRequest *>(arg);
    return (request->session_id() == sessionId);
}

class RialtoClientMediaPipelineIpcPlayPauseTest : public MediaPipelineIpcTestBase
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
};

/**
 * Test that Play can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcPlayPauseTest, PlaySuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("play"), m_controllerMock.get(),
                                           PlayRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->play(), true);
}

/**
 * Test that Play fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcPlayPauseTest, PlayChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->play(), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that Play fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcPlayPauseTest, PlayReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("play"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->play(), true);
}

/**
 * Test that Play fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcPlayPauseTest, PlayFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("play"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->play(), false);
}

/**
 * Test that Pause can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcPlayPauseTest, PauseSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("pause"), m_controllerMock.get(),
                                           PauseRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->pause(), true);
}

/**
 * Test that Pause fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcPlayPauseTest, PauseChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->pause(), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that Pause fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcPlayPauseTest, PauseReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("pause"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->pause(), true);
}

/**
 * Test that Pause fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcPlayPauseTest, PauseFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("pause"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->pause(), false);
}
