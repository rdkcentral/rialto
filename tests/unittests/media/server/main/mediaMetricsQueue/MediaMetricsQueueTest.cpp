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

#include "MediaMetricsQueue.h"
#include <gtest/gtest.h>

using firebolt::rialto::MediaSourceType;
using firebolt::rialto::MetricEvent;
using firebolt::rialto::MetricPhase;
using firebolt::rialto::MetricType;
using firebolt::rialto::server::MediaMetricsQueue;

namespace
{
MetricEvent createEvent(MetricType type, MetricPhase phase, uint64_t observedAtNs = 0)
{
    MetricEvent event;
    event.type = type;
    event.phase = phase;
    event.observedAtMonotonicNs = observedAtNs;
    return event;
}
} // namespace

class MediaMetricsQueueTest : public testing::Test
{
protected:
    MediaMetricsQueue m_sut;

    void subscribe(const std::vector<MetricType> &types, uint64_t nowNs = 0)
    {
        bool isDeliveryRequired{false};
        ASSERT_TRUE(m_sut.setSubscription(types, nowNs, 0, isDeliveryRequired));
    }
};

TEST_F(MediaMetricsQueueTest, ShouldDeliverNothingWithEmptySubscription)
{
    EXPECT_FALSE(m_sut.push(MediaSourceType::VIDEO, createEvent(MetricType::VIDEO_FRAME_DROP, MetricPhase::OCCURRENCE)));
    uint32_t missed{0};
    EXPECT_TRUE(m_sut.take(missed).empty());
    EXPECT_EQ(missed, 0u);
}

TEST_F(MediaMetricsQueueTest, ShouldQueueSubscribedOccurrencesWithoutMerging)
{
    subscribe({MetricType::VIDEO_FRAME_DROP});
    EXPECT_TRUE(m_sut.push(MediaSourceType::VIDEO, createEvent(MetricType::VIDEO_FRAME_DROP, MetricPhase::OCCURRENCE)));
    // Delivery is already scheduled
    EXPECT_FALSE(m_sut.push(MediaSourceType::VIDEO, createEvent(MetricType::VIDEO_FRAME_DROP, MetricPhase::OCCURRENCE)));
    EXPECT_FALSE(m_sut.push(MediaSourceType::AUDIO, createEvent(MetricType::AUDIO_FRAME_DROP, MetricPhase::OCCURRENCE)));

    uint32_t missed{0};
    const auto kEntries{m_sut.take(missed)};
    ASSERT_EQ(kEntries.size(), 2u);
    EXPECT_EQ(kEntries[0].sourceType, MediaSourceType::VIDEO);
    EXPECT_FALSE(kEntries[0].event.count);
    EXPECT_EQ(missed, 0u);

    // The next observation schedules a new delivery
    EXPECT_TRUE(m_sut.push(MediaSourceType::VIDEO, createEvent(MetricType::VIDEO_FRAME_DROP, MetricPhase::OCCURRENCE)));
}

TEST_F(MediaMetricsQueueTest, ShouldDiscardOldestAndCountMissedOnOverflow)
{
    subscribe({MetricType::VIDEO_DECODE_ERROR});
    for (std::size_t i = 0; i < MediaMetricsQueue::kMaxQueuedMetrics + 3; ++i)
    {
        m_sut.push(MediaSourceType::VIDEO, createEvent(MetricType::VIDEO_DECODE_ERROR, MetricPhase::OCCURRENCE, i));
    }
    uint32_t missed{0};
    const auto kEntries{m_sut.take(missed)};
    ASSERT_EQ(kEntries.size(), MediaMetricsQueue::kMaxQueuedMetrics);
    EXPECT_EQ(missed, 3u);
    EXPECT_EQ(kEntries.front().event.observedAtMonotonicNs, 3u);

    m_sut.take(missed);
    EXPECT_EQ(missed, 0u);
}

TEST_F(MediaMetricsQueueTest, ShouldRejectDuplicateAndUnknownTypesAndKeepPreviousSubscription)
{
    subscribe({MetricType::AUDIO_GAP});
    bool isDeliveryRequired{false};
    EXPECT_FALSE(m_sut.setSubscription({MetricType::AUDIO_GAP, MetricType::AUDIO_GAP}, 0, 0, isDeliveryRequired));
    EXPECT_FALSE(m_sut.setSubscription({static_cast<MetricType>(7)}, 0, 0, isDeliveryRequired));
    EXPECT_TRUE(m_sut.push(MediaSourceType::AUDIO, createEvent(MetricType::AUDIO_GAP, MetricPhase::STARTED)));
}

TEST_F(MediaMetricsQueueTest, ShouldReplaceSubscriptionAndDropQueuedEntriesOfRemovedTypes)
{
    subscribe({MetricType::VIDEO_FRAME_DROP, MetricType::AUDIO_FRAME_DROP});
    m_sut.push(MediaSourceType::VIDEO, createEvent(MetricType::VIDEO_FRAME_DROP, MetricPhase::OCCURRENCE));
    m_sut.push(MediaSourceType::AUDIO, createEvent(MetricType::AUDIO_FRAME_DROP, MetricPhase::OCCURRENCE));
    subscribe({MetricType::AUDIO_FRAME_DROP});

    EXPECT_FALSE(m_sut.push(MediaSourceType::VIDEO, createEvent(MetricType::VIDEO_FRAME_DROP, MetricPhase::OCCURRENCE)));
    uint32_t missed{0};
    const auto kEntries{m_sut.take(missed)};
    ASSERT_EQ(kEntries.size(), 1u);
    EXPECT_EQ(kEntries[0].event.type, MetricType::AUDIO_FRAME_DROP);
    EXPECT_EQ(missed, 0u);
}

TEST_F(MediaMetricsQueueTest, ShouldNotDeliverResolvedOfRemovedEpisodeType)
{
    subscribe({MetricType::BUFFER_UNDERFLOW});
    m_sut.push(MediaSourceType::VIDEO, createEvent(MetricType::BUFFER_UNDERFLOW, MetricPhase::STARTED));
    uint32_t missed{0};
    m_sut.take(missed);
    subscribe({});

    MetricEvent resolved{createEvent(MetricType::BUFFER_UNDERFLOW, MetricPhase::RESOLVED)};
    resolved.durationNs = 100;
    EXPECT_FALSE(m_sut.push(MediaSourceType::VIDEO, resolved));
    EXPECT_TRUE(m_sut.take(missed).empty());
}

TEST_F(MediaMetricsQueueTest, ShouldStartActiveEpisodeAtEnablementAndMeasureSubscribedInterval)
{
    MetricEvent started{createEvent(MetricType::VIDEO_FRAME_REPEAT, MetricPhase::STARTED, 1000)};
    started.presentationTimeStampNs = 42;
    EXPECT_FALSE(m_sut.push(MediaSourceType::VIDEO, started));

    bool isDeliveryRequired{false};
    ASSERT_TRUE(m_sut.setSubscription({MetricType::VIDEO_FRAME_REPEAT}, 5000, 77, isDeliveryRequired));
    EXPECT_TRUE(isDeliveryRequired);
    uint32_t missed{0};
    auto entries{m_sut.take(missed)};
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].event.phase, MetricPhase::STARTED);
    EXPECT_EQ(entries[0].event.presentationTimeStampNs, 42);
    EXPECT_EQ(entries[0].event.observedAtMonotonicNs, 5000u);
    EXPECT_EQ(entries[0].event.observedAtUnixNs, 77);

    MetricEvent resolved{createEvent(MetricType::VIDEO_FRAME_REPEAT, MetricPhase::RESOLVED, 8000)};
    resolved.durationNs = 7000;
    resolved.count = 5;
    EXPECT_TRUE(m_sut.push(MediaSourceType::VIDEO, resolved));
    entries = m_sut.take(missed);
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].event.durationNs, 3000u);
    EXPECT_EQ(entries[0].event.count, 5u);
}

TEST_F(MediaMetricsQueueTest, ShouldKeepEpisodeStateForTypesInBothSubscriptions)
{
    subscribe({MetricType::AUDIO_GAP});
    m_sut.push(MediaSourceType::AUDIO, createEvent(MetricType::AUDIO_GAP, MetricPhase::STARTED, 1000));
    uint32_t missed{0};
    m_sut.take(missed);

    bool isDeliveryRequired{false};
    ASSERT_TRUE(m_sut.setSubscription({MetricType::AUDIO_GAP, MetricType::VIDEO_FRAME_DROP}, 5000, 0, isDeliveryRequired));
    EXPECT_FALSE(isDeliveryRequired);

    MetricEvent resolved{createEvent(MetricType::AUDIO_GAP, MetricPhase::RESOLVED, 8000)};
    resolved.durationNs = 7000;
    resolved.count = 2;
    m_sut.push(MediaSourceType::AUDIO, resolved);
    const auto kEntries{m_sut.take(missed)};
    ASSERT_EQ(kEntries.size(), 1u);
    EXPECT_EQ(kEntries[0].event.durationNs, 7000u);
}

TEST_F(MediaMetricsQueueTest, ShouldForgetEpisodesWhenCleared)
{
    m_sut.push(MediaSourceType::VIDEO, createEvent(MetricType::BUFFER_UNDERFLOW, MetricPhase::STARTED));
    m_sut.clearEpisodes();
    bool isDeliveryRequired{false};
    ASSERT_TRUE(m_sut.setSubscription({MetricType::BUFFER_UNDERFLOW}, 5000, 0, isDeliveryRequired));
    EXPECT_FALSE(isDeliveryRequired);
}
