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
 * Component Test: Media metrics
 * Test Objective:
 *  Test the metric subscription, the delivery of a media metric batch and the frame counters query.
 *
 * Test Steps:
 *  Step 1: Set metric subscription
 *   Expect that setMetricSubscription is sent to the server with the selected types.
 *  Step 2: Notify media metric batch
 *   Server sends a MediaMetricBatchEvent with a missed count and one resolved audio gap.
 *   Expect that the client is notified of the missed count first, then of the metric.
 *  Step 3: Get frame counters
 *   Expect that getStats is sent to the server and the three counters are returned.
 *
 * Code:
 */
TEST_F(PlaybackMetricsTest, subscriptionNotificationAndQuery)
{
    // Step 1: Set metric subscription
    const std::vector<MetricType> kTypes{MetricType::AUDIO_GAP, MetricType::VIDEO_FRAME_DROP};
    MediaPipelineTestMethods::shouldSetMetricSubscription(kTypes);
    MediaPipelineTestMethods::setMetricSubscription(kTypes);

    // Step 2: Notify media metric batch
    MetricEvent metric;
    metric.type = MetricType::AUDIO_GAP;
    metric.phase = MetricPhase::RESOLVED;
    metric.sourceId = 1;
    metric.presentationTimeStampNs = 0;
    metric.count = 4;
    metric.durationNs = 1500;
    metric.observedAtMonotonicNs = 11;
    metric.observedAtUnixNs = 22;
    MediaPipelineTestMethods::shouldNotifyMetric(metric, 2);
    MediaPipelineTestMethods::sendNotifyMetric(metric, 2);

    // Step 3: Get frame counters
    VideoFrameCounters frameCounters;
    frameCounters.renderedFrames = 2345;
    frameCounters.droppedFrames = 6;
    frameCounters.corruptedFrames = 2;
    MediaPipelineTestMethods::shouldGetFrameCounters(frameCounters);
    MediaPipelineTestMethods::getFrameCounters(frameCounters);
}
} // namespace firebolt::rialto::client::ct
