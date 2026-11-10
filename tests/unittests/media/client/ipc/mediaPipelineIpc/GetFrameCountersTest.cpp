/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

class RialtoClientMediaPipelineIpcGetFrameCountersTest : public MediaPipelineIpcTestBase
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

TEST_F(RialtoClientMediaPipelineIpcGetFrameCountersTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("getFrameCounters"), m_controllerMock.get(),
                           getFrameCountersRequestMatcher(m_sessionId, m_kSourceId), _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(
            [](google::protobuf::Message *response)
            {
                auto *frameCountersResponse = dynamic_cast<firebolt::rialto::GetFrameCountersResponse *>(response);
                frameCountersResponse->set_rendered_frames(100);
                frameCountersResponse->set_dropped_frames(2);
                frameCountersResponse->set_corrupted_frames(1);
            })));

    FrameCounters frameCounters;
    EXPECT_TRUE(m_mediaPipelineIpc->getFrameCounters(m_kSourceId, frameCounters));
    EXPECT_EQ(frameCounters.renderedFrames, 100u);
    EXPECT_EQ(frameCounters.droppedFrames, 2u);
    EXPECT_EQ(frameCounters.corruptedFrames, 1u);
}

TEST_F(RialtoClientMediaPipelineIpcGetFrameCountersTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    FrameCounters frameCounters;
    EXPECT_FALSE(m_mediaPipelineIpc->getFrameCounters(m_kSourceId, frameCounters));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

TEST_F(RialtoClientMediaPipelineIpcGetFrameCountersTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getFrameCounters"), _, _, _, _));

    FrameCounters frameCounters;
    EXPECT_TRUE(m_mediaPipelineIpc->getFrameCounters(m_kSourceId, frameCounters));
}

TEST_F(RialtoClientMediaPipelineIpcGetFrameCountersTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getFrameCounters"), _, _, _, _));

    FrameCounters frameCounters;
    EXPECT_FALSE(m_mediaPipelineIpc->getFrameCounters(m_kSourceId, frameCounters));
}
