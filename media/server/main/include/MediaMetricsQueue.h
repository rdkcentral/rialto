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

#ifndef FIREBOLT_RIALTO_SERVER_MEDIA_METRICS_QUEUE_H_
#define FIREBOLT_RIALTO_SERVER_MEDIA_METRICS_QUEUE_H_

#include "MediaCommon.h"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace firebolt::rialto::server
{
/**
 * @brief Filters media metric observations against the subscription of one pipeline and queues them for delivery.
 *
 * Producer threads push observations; the main thread takes them in batches. The queue is bounded: on overflow the
 * oldest observation is discarded and counted as missed. The subscription belongs to the pipeline session and
 * persists across load(). Thread safe.
 */
class MediaMetricsQueue
{
public:
    /**
     * @brief The maximum number of queued observations.
     */
    static constexpr std::size_t kMaxQueuedMetrics{256};

    /**
     * @brief A queued observation. The source id is resolved from the source type at delivery.
     */
    struct Entry
    {
        MediaSourceType sourceType;
        MetricEvent event;
    };

    /**
     * @brief Replaces the subscription.
     *
     * Queued observations of removed types are discarded. An episode type that is added while one of its episodes
     * is active starts an episode at enablement; its later RESOLVED duration covers only the subscribed interval.
     *
     * @param[in]  types                 : The complete set of metric types.
     * @param[in]  nowMonotonicNs        : The current monotonic time.
     * @param[in]  nowUnixNs             : The current wall-clock time.
     * @param[out] isDeliveryRequired    : Set to true when the caller must schedule a delivery.
     *
     * @retval false, leaving the previous subscription effective, for an unknown or duplicate type.
     */
    bool setSubscription(const std::vector<MetricType> &types, uint64_t nowMonotonicNs, int64_t nowUnixNs,
                         bool &isDeliveryRequired);

    /**
     * @brief Handles one observation from a platform producer.
     *
     * @param[in] sourceType : The source type the observation belongs to.
     * @param[in] event      : The observation, with both observed-at clocks set.
     *
     * @retval true when the caller must schedule a delivery.
     */
    bool push(MediaSourceType sourceType, const MetricEvent &event);

    /**
     * @brief Takes all queued observations. Called by the scheduled delivery.
     *
     * @param[out] missedCount : The number of observations discarded since the previous delivery.
     *
     * @retval the queued observations in acceptance order.
     */
    std::vector<Entry> take(uint32_t &missedCount);

    /**
     * @brief Forgets all active episodes. Called when a new player is created.
     */
    void clearEpisodes();

private:
    struct ActiveEpisode
    {
        std::optional<int64_t> presentationTimeStampNs;
        std::optional<uint64_t> enabledAtMonotonicNs;
    };
    using EpisodeKey = std::pair<MetricType, MediaSourceType>;

    bool enqueue(const Entry &entry);

    mutable std::mutex m_mutex;
    std::set<MetricType> m_subscription;
    std::deque<Entry> m_queue;
    uint32_t m_missedCount{0};
    bool m_isDeliveryScheduled{false};
    std::map<EpisodeKey, ActiveEpisode> m_activeEpisodes;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_METRICS_QUEUE_H_
