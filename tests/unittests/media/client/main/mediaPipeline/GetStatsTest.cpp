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

#include "MediaPipelineTestBase.h"

using ::testing::DoAll;
using ::testing::SetArgReferee;

class RialtoClientMediaPipelineGetStatsTest : public MediaPipelineTestBase
{
protected:
    const int32_t m_kSourceId{1};

    virtual void SetUp()
    {
        MediaPipelineTestBase::SetUp();

        createMediaPipeline();
    }

    virtual void TearDown()
    {
        destroyMediaPipeline();

        MediaPipelineTestBase::TearDown();
    }
};

/**
 * Test that getStats returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineGetStatsTest, GetStatsSuccess)
{
    constexpr uint64_t kRenderedFrames{1234};
    constexpr uint64_t kDroppedFrames{5};
    uint64_t renderedFrames;
    uint64_t droppedFrames;
    EXPECT_CALL(*m_mediaPipelineIpcMock, getStats(m_kSourceId, _, _))
        .WillOnce(Invoke(
            [&](int32_t sourceId, uint64_t &renderedFrames, uint64_t &droppedFrames)
            {
                renderedFrames = kRenderedFrames;
                droppedFrames = kDroppedFrames;
                return true;
            }));
    EXPECT_TRUE(m_mediaPipeline->getStats(m_kSourceId, renderedFrames, droppedFrames));
}

/**
 * Test that getStats returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineGetStatsTest, GetStatsFailure)
{
    uint64_t renderedFrames;
    uint64_t droppedFrames;
    EXPECT_CALL(*m_mediaPipelineIpcMock, getStats(m_kSourceId, _, _)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getStats(m_kSourceId, renderedFrames, droppedFrames));
}

TEST_F(RialtoClientMediaPipelineGetStatsTest, GetFrameCountersSuccess)
{
    VideoFrameCounters kFrameCounters;
    kFrameCounters.renderedFrames = 1234;
    kFrameCounters.droppedFrames = 5;
    kFrameCounters.corruptedFrames = 2;
    VideoFrameCounters frameCounters;
    EXPECT_CALL(*m_mediaPipelineIpcMock, getFrameCounters(m_kSourceId, _))
        .WillOnce(DoAll(SetArgReferee<1>(kFrameCounters), Return(true)));
    EXPECT_TRUE(m_mediaPipeline->getFrameCounters(m_kSourceId, frameCounters));
    EXPECT_EQ(frameCounters.size, sizeof(VideoFrameCounters));
    EXPECT_EQ(frameCounters.renderedFrames, kFrameCounters.renderedFrames);
    EXPECT_EQ(frameCounters.droppedFrames, kFrameCounters.droppedFrames);
    EXPECT_EQ(frameCounters.corruptedFrames, kFrameCounters.corruptedFrames);
}

TEST_F(RialtoClientMediaPipelineGetStatsTest, GetFrameCountersFailure)
{
    VideoFrameCounters frameCounters;
    EXPECT_CALL(*m_mediaPipelineIpcMock, getFrameCounters(m_kSourceId, _)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getFrameCounters(m_kSourceId, frameCounters));
}

TEST_F(RialtoClientMediaPipelineGetStatsTest, SetMetricSubscriptionSuccess)
{
    const std::vector<MetricType> kTypes{MetricType::AUDIO_GAP, MetricType::VIDEO_FRAME_REPEAT};
    EXPECT_CALL(*m_mediaPipelineIpcMock, setMetricSubscription(kTypes)).WillOnce(Return(true));
    EXPECT_TRUE(m_mediaPipeline->setMetricSubscription(kTypes));
}

TEST_F(RialtoClientMediaPipelineGetStatsTest, SetMetricSubscriptionRejectsDuplicateAndUnknownTypes)
{
    EXPECT_FALSE(m_mediaPipeline->setMetricSubscription({MetricType::AUDIO_GAP, MetricType::AUDIO_GAP}));
    EXPECT_FALSE(m_mediaPipeline->setMetricSubscription({static_cast<MetricType>(7)}));
}

TEST_F(RialtoClientMediaPipelineGetStatsTest, SetMetricSubscriptionFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setMetricSubscription(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->setMetricSubscription({}));
}
