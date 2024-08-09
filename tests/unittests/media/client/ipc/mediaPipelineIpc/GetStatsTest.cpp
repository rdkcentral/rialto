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

class RialtoClientMediaPipelineIpcGetStatsTest : public MediaPipelineIpcTestBase
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

    const int32_t m_kSourceId{1};
};

/**
 * Test that getStats can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcGetStatsTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("getStats"), m_controllerMock.get(),
                           getStatsRequestMatcher(m_sessionId, m_kSourceId), _, m_blockingClosureMock.get()));

    uint64_t renderedFrames;
    uint64_t droppedFrames;
    EXPECT_TRUE(m_mediaPipelineIpc->getStats(m_kSourceId, renderedFrames, droppedFrames));
}

/**
 * Test that getStats fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcGetStatsTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    uint64_t renderedFrames;
    uint64_t droppedFrames;
    EXPECT_FALSE(m_mediaPipelineIpc->getStats(m_kSourceId, renderedFrames, droppedFrames));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getStats fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcGetStatsTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getStats"), _, _, _, _));

    uint64_t renderedFrames;
    uint64_t droppedFrames;
    EXPECT_TRUE(m_mediaPipelineIpc->getStats(m_kSourceId, renderedFrames, droppedFrames));
}

/**
 * Test that getStats fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcGetStatsTest, GetStatsFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getStats"), _, _, _, _));

    uint64_t renderedFrames;
    uint64_t droppedFrames;
    EXPECT_FALSE(m_mediaPipelineIpc->getStats(m_kSourceId, renderedFrames, droppedFrames));
}
