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
#include <algorithm>

namespace firebolt::rialto::server
{
bool MediaMetricsQueue::setSubscription(const std::vector<MetricType> &types, uint64_t nowMonotonicNs,
                                        int64_t nowUnixNs, bool &isDeliveryRequired)
{
    isDeliveryRequired = false;
    std::set<MetricType> newSubscription;
    for (const auto &type : types)
    {
        if (static_cast<uint32_t>(type) > static_cast<uint32_t>(MetricType::BUFFER_UNDERFLOW) ||
            !newSubscription.insert(type).second)
        {
            return false;
        }
    }

    std::lock_guard<std::mutex> lock{m_mutex};
    m_queue.erase(std::remove_if(m_queue.begin(), m_queue.end(),
                                 [&](const Entry &entry) { return newSubscription.count(entry.event.type) == 0; }),
                  m_queue.end());
    for (auto &[key, episode] : m_activeEpisodes)
    {
        const MetricType kType{key.first};
        if (newSubscription.count(kType) != 0 && m_subscription.count(kType) == 0)
        {
            episode.enabledAtMonotonicNs = nowMonotonicNs;
            MetricEvent started;
            started.type = kType;
            started.phase = MetricPhase::STARTED;
            started.presentationTimeStampNs = episode.presentationTimeStampNs;
            started.observedAtMonotonicNs = nowMonotonicNs;
            started.observedAtUnixNs = nowUnixNs;
            isDeliveryRequired = enqueue(Entry{key.second, started}) || isDeliveryRequired;
        }
    }
    m_subscription = std::move(newSubscription);
    return true;
}

bool MediaMetricsQueue::push(MediaSourceType sourceType, const MetricEvent &event)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    MetricEvent forwarded{event};
    const EpisodeKey kKey{event.type, sourceType};
    if (MetricPhase::STARTED == event.phase)
    {
        m_activeEpisodes[kKey] = ActiveEpisode{event.presentationTimeStampNs, std::nullopt};
    }
    else if (MetricPhase::RESOLVED == event.phase)
    {
        auto episodeIt = m_activeEpisodes.find(kKey);
        if (episodeIt != m_activeEpisodes.end())
        {
            const auto &kEnabledAt = episodeIt->second.enabledAtMonotonicNs;
            if (kEnabledAt && forwarded.durationNs)
            {
                const uint64_t kSubscribedNs{
                    event.observedAtMonotonicNs > *kEnabledAt ? event.observedAtMonotonicNs - *kEnabledAt : 0};
                forwarded.durationNs = std::min(*forwarded.durationNs, kSubscribedNs);
            }
            m_activeEpisodes.erase(episodeIt);
        }
    }

    if (m_subscription.count(event.type) == 0)
    {
        return false;
    }
    return enqueue(Entry{sourceType, forwarded});
}

std::vector<MediaMetricsQueue::Entry> MediaMetricsQueue::take(uint32_t &missedCount)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    std::vector<Entry> entries{m_queue.begin(), m_queue.end()};
    m_queue.clear();
    missedCount = m_missedCount;
    m_missedCount = 0;
    m_isDeliveryScheduled = false;
    return entries;
}

void MediaMetricsQueue::clearEpisodes()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    m_activeEpisodes.clear();
}

bool MediaMetricsQueue::enqueue(const Entry &entry)
{
    if (m_queue.size() >= kMaxQueuedMetrics)
    {
        m_queue.pop_front();
        ++m_missedCount;
    }
    m_queue.push_back(entry);
    if (m_isDeliveryScheduled)
    {
        return false;
    }
    m_isDeliveryScheduled = true;
    return true;
}
} // namespace firebolt::rialto::server
