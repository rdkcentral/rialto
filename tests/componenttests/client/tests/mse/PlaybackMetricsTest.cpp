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

#include "ClientComponentTest.h"
#include <gtest/gtest.h>

namespace firebolt::rialto::client::ct
{
class PlaybackMetricsTest : public ClientComponentTest
{
public:
    PlaybackMetricsTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~PlaybackMetricsTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Playback metrics
 * Test Objective:
 *  Test that a playback metric notification is forwarded to the client and that the frame counters of the
 *  video source can be queried.
 *
 * Test Steps:
 *  Step 1: Notify playback metric
 *   Server sends a PlaybackMetricEvent for the video source.
 *   Expect that the client is notified with the same metric.
 *  Step 2: Get frame counters
 *   Expect that GetFrameCounters is sent to the server and the counters are returned.
 *
 * Code:
 */
TEST_F(PlaybackMetricsTest, notificationAndQuery)
{
    // Step 1: Notify playback metric
    PlaybackMetric metric;
    metric.type = PlaybackMetricType::VIDEO_FRAME_DROP;
    metric.phase = PlaybackMetricPhase::RESOLVED;
    metric.ptsNs = 10500;
    metric.count = 1;
    metric.durationNs = 1500;
    metric.frameCounters = FrameCounters{100, 3, 1};
    MediaPipelineTestMethods::shouldNotifyPlaybackMetric(metric);
    MediaPipelineTestMethods::sendNotifyPlaybackMetric(metric);

    // Step 2: Get frame counters
    const FrameCounters kFrameCounters{2345, 6, 2};
    MediaPipelineTestMethods::shouldGetFrameCounters(kFrameCounters);
    MediaPipelineTestMethods::getFrameCounters(kFrameCounters);
}
} // namespace firebolt::rialto::client::ct
