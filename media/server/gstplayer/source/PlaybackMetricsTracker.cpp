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

namespace firebolt::rialto::server
{
std::optional<PlaybackMetric> PlaybackMetricsTracker::onMetric(PlaybackMetricType type, PlaybackMetricPhase phase,
                                                               uint64_t ptsNs, std::chrono::steady_clock::time_point now)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    PlaybackMetric metric;
    metric.type = type;
    metric.phase = phase;
    metric.ptsNs = ptsNs;
    metric.count = 1;

    switch (phase)
    {
    case PlaybackMetricPhase::OCCURRENCE:
    {
        if (PlaybackMetricType::VIDEO_DECODE_ERROR == type)
        {
            ++m_corruptedFrames;
        }
        auto &occurrence = m_occurrences[type];
        ++occurrence.pendingCount;
        if (occurrence.lastForwarded && now - *occurrence.lastForwarded < kOccurrenceInterval)
        {
            return std::nullopt;
        }
        metric.count = occurrence.pendingCount;
        occurrence.pendingCount = 0;
        occurrence.lastForwarded = now;
        break;
    }
    case PlaybackMetricPhase::STARTED:
    {
        m_episodeStartPtsNs[type] = ptsNs;
        break;
    }
    case PlaybackMetricPhase::RESOLVED:
    {
        auto startIt = m_episodeStartPtsNs.find(type);
        if (startIt != m_episodeStartPtsNs.end())
        {
            if (ptsNs >= startIt->second)
            {
                metric.durationNs = ptsNs - startIt->second;
            }
            m_episodeStartPtsNs.erase(startIt);
        }
        break;
    }
    }
    return metric;
}

uint64_t PlaybackMetricsTracker::getCorruptedFrames() const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_corruptedFrames;
}

void PlaybackMetricsTracker::reset()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    m_corruptedFrames = 0;
    m_occurrences.clear();
    m_episodeStartPtsNs.clear();
}
} // namespace firebolt::rialto::server
