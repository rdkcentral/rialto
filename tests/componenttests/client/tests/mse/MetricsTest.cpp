/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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
class MetricsTest : public ClientComponentTest
{
public:
    MetricsTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~MetricsTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Metrics
 * Test Objective:
 *  Test that the metric subscription is propagated to the server.
 *  Test that the frame counters are requested from the server for the given source.
 *  Test that a metric notification from the server is propagated to the client.
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline
 *
 * Test Initialize:
 *  Create memory region for the shared buffer.
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *  Initalise a audio video media session paused after preroll.
 *
 * Test Steps:
 *  Step 1: Set metric subscription
 *   Expect that the subscribed metric types are propagated to the server.
 *
 *  Step 2: Get frame counters
 *   Expect that the frame counters request for the video source reaches the server
 *   and the returned counters are propagated to the client.
 *
 *  Step 3: Notify metric
 *   Server notifies the client of a video frame drop metric.
 *   Expect that the metric is propagated to the client.
 *
 * Test Tear-down:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Metric subscription, frame counters and metric notifications are handled by the rialto client.
 *
 * Code:
 */
TEST_F(MetricsTest, setSubscriptionGetFrameCountersAndNotifyMetric)
{
    // Step 1: Set metric subscription
    const std::vector<MetricType> kMetricTypes{MetricType::VIDEO_FRAME_DROP, MetricType::AUDIO_FRAME_DROP};
    MediaPipelineTestMethods::shouldSetMetricSubscription(kMetricTypes);
    MediaPipelineTestMethods::setMetricSubscription(kMetricTypes);

    // Step 2: Get frame counters
    {
        const uint64_t kRenderedFrames = 1000;
        const uint64_t kDroppedFrames = 20;
        const uint64_t kCorruptedFrames = 3;
        const uint64_t kTimestamp = 123456789;
        MediaPipelineTestMethods::shouldGetFrameCounters(kRenderedFrames, kDroppedFrames, kCorruptedFrames, kTimestamp);
        MediaPipelineTestMethods::getFrameCounters(kRenderedFrames, kDroppedFrames, kCorruptedFrames, kTimestamp);
    }

    // Step 3: Notify metric
    MediaPipelineTestMethods::shouldNotifyMetric();
    MediaPipelineTestMethods::sendNotifyMetric();
}
} // namespace firebolt::rialto::client::ct
