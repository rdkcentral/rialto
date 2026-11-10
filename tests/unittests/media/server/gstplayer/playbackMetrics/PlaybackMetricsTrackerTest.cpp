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

#include "PlaybackMetricsTracker.h"
#include <gtest/gtest.h>

using firebolt::rialto::PlaybackMetricPhase;
using firebolt::rialto::PlaybackMetricType;
using firebolt::rialto::server::PlaybackMetricsTracker;
using namespace std::chrono_literals;

class PlaybackMetricsTrackerTest : public testing::Test
{
protected:
    PlaybackMetricsTracker m_sut;
    const std::chrono::steady_clock::time_point m_kNow{std::chrono::steady_clock::now()};
};

TEST_F(PlaybackMetricsTrackerTest, ShouldForwardFirstOccurrence)
{
    const auto kMetric{m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::OCCURRENCE, 100, m_kNow)};
    ASSERT_TRUE(kMetric);
    EXPECT_EQ(kMetric->type, PlaybackMetricType::VIDEO_FRAME_DROP);
    EXPECT_EQ(kMetric->phase, PlaybackMetricPhase::OCCURRENCE);
    EXPECT_EQ(kMetric->ptsNs, 100u);
    EXPECT_EQ(kMetric->count, 1u);
    EXPECT_EQ(kMetric->durationNs, 0u);
}

TEST_F(PlaybackMetricsTrackerTest, ShouldMergeOccurrencesWithinInterval)
{
    EXPECT_TRUE(m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::OCCURRENCE, 1, m_kNow));
    EXPECT_FALSE(m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::OCCURRENCE, 2, m_kNow + 100ms));
    EXPECT_FALSE(m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::OCCURRENCE, 3, m_kNow + 999ms));

    const auto kMetric{
        m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::OCCURRENCE, 4, m_kNow + 1000ms)};
    ASSERT_TRUE(kMetric);
    EXPECT_EQ(kMetric->count, 3u);
    EXPECT_EQ(kMetric->ptsNs, 4u);
}

TEST_F(PlaybackMetricsTrackerTest, ShouldLimitEachTypeSeparately)
{
    EXPECT_TRUE(m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::OCCURRENCE, 1, m_kNow));
    EXPECT_TRUE(m_sut.onMetric(PlaybackMetricType::VIDEO_DECODE_ERROR, PlaybackMetricPhase::OCCURRENCE, 2, m_kNow));
}

TEST_F(PlaybackMetricsTrackerTest, ShouldAlwaysForwardEpisodePhases)
{
    EXPECT_TRUE(m_sut.onMetric(PlaybackMetricType::BUFFER_UNDERFLOW, PlaybackMetricPhase::STARTED, 1, m_kNow));
    EXPECT_TRUE(m_sut.onMetric(PlaybackMetricType::BUFFER_UNDERFLOW, PlaybackMetricPhase::RESOLVED, 2, m_kNow));
    EXPECT_TRUE(m_sut.onMetric(PlaybackMetricType::BUFFER_UNDERFLOW, PlaybackMetricPhase::STARTED, 3, m_kNow));
    EXPECT_TRUE(m_sut.onMetric(PlaybackMetricType::BUFFER_UNDERFLOW, PlaybackMetricPhase::RESOLVED, 4, m_kNow));
}

TEST_F(PlaybackMetricsTrackerTest, ShouldComputeEpisodeDuration)
{
    const auto kStarted{m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::STARTED, 9000, m_kNow)};
    ASSERT_TRUE(kStarted);
    EXPECT_EQ(kStarted->durationNs, 0u);

    const auto kResolved{
        m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::RESOLVED, 10500, m_kNow)};
    ASSERT_TRUE(kResolved);
    EXPECT_EQ(kResolved->phase, PlaybackMetricPhase::RESOLVED);
    EXPECT_EQ(kResolved->count, 1u);
    EXPECT_EQ(kResolved->durationNs, 1500u);
}

TEST_F(PlaybackMetricsTrackerTest, ShouldNotComputeDurationWithoutStarted)
{
    const auto kResolved{
        m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::RESOLVED, 10500, m_kNow)};
    ASSERT_TRUE(kResolved);
    EXPECT_EQ(kResolved->durationNs, 0u);
}

TEST_F(PlaybackMetricsTrackerTest, ShouldNotComputeNegativeDuration)
{
    EXPECT_TRUE(m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::STARTED, 2000, m_kNow));
    const auto kResolved{
        m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::RESOLVED, 1000, m_kNow)};
    ASSERT_TRUE(kResolved);
    EXPECT_EQ(kResolved->durationNs, 0u);
}

TEST_F(PlaybackMetricsTrackerTest, ShouldTrackEpisodesPerType)
{
    EXPECT_TRUE(m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::STARTED, 1000, m_kNow));
    EXPECT_TRUE(m_sut.onMetric(PlaybackMetricType::BUFFER_UNDERFLOW, PlaybackMetricPhase::STARTED, 5000, m_kNow));
    const auto kResolved{
        m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::RESOLVED, 3000, m_kNow)};
    ASSERT_TRUE(kResolved);
    EXPECT_EQ(kResolved->durationNs, 2000u);
}

TEST_F(PlaybackMetricsTrackerTest, ShouldCountVideoDecodeErrorOccurrencesOnly)
{
    EXPECT_EQ(m_sut.getCorruptedFrames(), 0u);
    m_sut.onMetric(PlaybackMetricType::VIDEO_DECODE_ERROR, PlaybackMetricPhase::OCCURRENCE, 1, m_kNow);
    m_sut.onMetric(PlaybackMetricType::VIDEO_DECODE_ERROR, PlaybackMetricPhase::OCCURRENCE, 2, m_kNow + 1ms);
    m_sut.onMetric(PlaybackMetricType::VIDEO_DECODE_ERROR, PlaybackMetricPhase::STARTED, 3, m_kNow);
    m_sut.onMetric(PlaybackMetricType::VIDEO_DECODE_ERROR, PlaybackMetricPhase::RESOLVED, 4, m_kNow);
    m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::OCCURRENCE, 5, m_kNow);
    EXPECT_EQ(m_sut.getCorruptedFrames(), 2u);
}

TEST_F(PlaybackMetricsTrackerTest, ShouldClearStateOnReset)
{
    m_sut.onMetric(PlaybackMetricType::VIDEO_DECODE_ERROR, PlaybackMetricPhase::OCCURRENCE, 1, m_kNow);
    m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::STARTED, 1000, m_kNow);

    m_sut.reset();

    EXPECT_EQ(m_sut.getCorruptedFrames(), 0u);
    const auto kResolved{
        m_sut.onMetric(PlaybackMetricType::VIDEO_FRAME_DROP, PlaybackMetricPhase::RESOLVED, 3000, m_kNow)};
    ASSERT_TRUE(kResolved);
    EXPECT_EQ(kResolved->durationNs, 0u);
    const auto kOccurrence{
        m_sut.onMetric(PlaybackMetricType::VIDEO_DECODE_ERROR, PlaybackMetricPhase::OCCURRENCE, 2, m_kNow + 1ms)};
    ASSERT_TRUE(kOccurrence);
    EXPECT_EQ(kOccurrence->count, 1u);
}
