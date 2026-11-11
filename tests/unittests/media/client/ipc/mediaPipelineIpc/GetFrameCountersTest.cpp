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
                CallMethod(methodMatcher("getStats"), m_controllerMock.get(),
                           getStatsRequestMatcher(m_sessionId, m_kSourceId), _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(
            [](google::protobuf::Message *response)
            {
                auto *statsResponse = dynamic_cast<firebolt::rialto::GetStatsResponse *>(response);
                statsResponse->set_rendered_frames(100);
                statsResponse->set_dropped_frames(2);
                statsResponse->set_corrupted_frames(1);
            })));

    VideoFrameCounters frameCounters;
    EXPECT_TRUE(m_mediaPipelineIpc->getFrameCounters(m_kSourceId, frameCounters));
    EXPECT_EQ(frameCounters.renderedFrames, 100u);
    EXPECT_EQ(frameCounters.droppedFrames, 2u);
    EXPECT_EQ(frameCounters.corruptedFrames, 1u);
}

TEST_F(RialtoClientMediaPipelineIpcGetFrameCountersTest, FailsWhenCorruptedIsNotReturned)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getStats"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(
            [](google::protobuf::Message *response)
            {
                auto *statsResponse = dynamic_cast<firebolt::rialto::GetStatsResponse *>(response);
                statsResponse->set_rendered_frames(100);
                statsResponse->set_dropped_frames(2);
            })));

    VideoFrameCounters frameCounters;
    EXPECT_FALSE(m_mediaPipelineIpc->getFrameCounters(m_kSourceId, frameCounters));
}

TEST_F(RialtoClientMediaPipelineIpcGetFrameCountersTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    VideoFrameCounters frameCounters;
    EXPECT_FALSE(m_mediaPipelineIpc->getFrameCounters(m_kSourceId, frameCounters));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

TEST_F(RialtoClientMediaPipelineIpcGetFrameCountersTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getStats"), _, _, _, _));

    VideoFrameCounters frameCounters;
    EXPECT_FALSE(m_mediaPipelineIpc->getFrameCounters(m_kSourceId, frameCounters));
}

TEST_F(RialtoClientMediaPipelineIpcGetFrameCountersTest, SetMetricSubscriptionSuccess)
{
    const std::vector<MetricType> kTypes{MetricType::VIDEO_FRAME_DROP, MetricType::BUFFER_UNDERFLOW};
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("setMetricSubscription"), m_controllerMock.get(),
                           setMetricSubscriptionRequestMatcher(m_sessionId, kTypes), _, m_blockingClosureMock.get()));

    EXPECT_TRUE(m_mediaPipelineIpc->setMetricSubscription(kTypes));
}

TEST_F(RialtoClientMediaPipelineIpcGetFrameCountersTest, SetMetricSubscriptionFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setMetricSubscription"), _, _, _, _));

    EXPECT_FALSE(m_mediaPipelineIpc->setMetricSubscription({}));
}
