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

#ifndef FIREBOLT_RIALTO_SERVER_PLAYBACK_METRICS_TRACKER_H_
#define FIREBOLT_RIALTO_SERVER_PLAYBACK_METRICS_TRACKER_H_

#include "MediaCommon.h"
#include <chrono>
#include <map>
#include <mutex>
#include <optional>

namespace firebolt::rialto::server
{
/**
 * @brief Tracks the playback metrics of the video sink.
 *
 * Counts video decode errors, computes episode durations and limits how often occurrence
 * metrics are forwarded. Thread safe.
 */
class PlaybackMetricsTracker
{
public:
    /**
     * @brief The minimum interval between two forwarded occurrence metrics of the same type.
     */
    static constexpr std::chrono::milliseconds kOccurrenceInterval{1000};

    /**
     * @brief Handles a metric reported by the sink.
     *
     * @param[in] type  : The metric type.
     * @param[in] phase : The metric phase.
     * @param[in] ptsNs : The producer PTS in nanoseconds.
     * @param[in] now   : The current time.
     *
     * @retval the metric to forward, without frame counters, or nullopt if it is merged into a later one.
     */
    std::optional<PlaybackMetric> onMetric(PlaybackMetricType type, PlaybackMetricPhase phase, uint64_t ptsNs,
                                           std::chrono::steady_clock::time_point now);

    /**
     * @brief Gets the number of video decode errors since the last reset.
     */
    uint64_t getCorruptedFrames() const;

    /**
     * @brief Clears all state. Called when the video source is attached, removed or flushed.
     */
    void reset();

private:
    struct OccurrenceState
    {
        uint64_t pendingCount{0};
        std::optional<std::chrono::steady_clock::time_point> lastForwarded;
    };

    mutable std::mutex m_mutex;
    uint64_t m_corruptedFrames{0};
    std::map<PlaybackMetricType, OccurrenceState> m_occurrences;
    std::map<PlaybackMetricType, uint64_t> m_episodeStartPtsNs;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_PLAYBACK_METRICS_TRACKER_H_
