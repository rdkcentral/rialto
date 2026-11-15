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
    uint64_t renderedFrames{0}, droppedFrames{0}, corruptedFrames{0}, timestamp{0};
    EXPECT_CALL(*m_mediaPipelineIpcMock, getFrameCounters(m_kSourceId, _, _, _, _))
        .WillOnce(Invoke(
            [](int32_t, uint64_t &rendered, uint64_t &dropped, uint64_t &corrupted, uint64_t &time)
            {
                rendered = 1234;
                dropped = 5;
                corrupted = 2;
                time = 9876543210ULL;
                return true;
            }));
    EXPECT_TRUE(m_mediaPipeline->getFrameCounters(m_kSourceId, renderedFrames, droppedFrames, corruptedFrames, timestamp));
    EXPECT_EQ(renderedFrames, 1234u);
    EXPECT_EQ(droppedFrames, 5u);
    EXPECT_EQ(corruptedFrames, 2u);
    EXPECT_EQ(timestamp, 9876543210ULL);
}

TEST_F(RialtoClientMediaPipelineGetStatsTest, GetFrameCountersFailure)
{
    uint64_t renderedFrames{1234}, droppedFrames{5}, corruptedFrames{2}, timestamp{9876543210ULL};
    EXPECT_CALL(*m_mediaPipelineIpcMock, getFrameCounters(m_kSourceId, _, _, _, _)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getFrameCounters(m_kSourceId, renderedFrames, droppedFrames, corruptedFrames, timestamp));
    EXPECT_EQ(renderedFrames, 1234u);
    EXPECT_EQ(droppedFrames, 5u);
    EXPECT_EQ(corruptedFrames, 2u);
    EXPECT_EQ(timestamp, 9876543210ULL);
}
