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

#include "ClientMock.h"
#include "MediaCommon.h"
#include "MediaPipelineClient.h"
#include "mediapipelinemodule.pb.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server::ipc;
using ::testing::_;
using ::testing::Invoke;
using ::testing::StrictMock;

TEST(MediaPipelineClientTests, NotifyMetricEventSerializesAndPreservesPayload)
{
    constexpr int kSessionId{8};
    constexpr int32_t kSourceId{14};
    constexpr uint32_t kPhase{2};
    constexpr uint64_t kCount{12};
    constexpr uint64_t kDurationNs{45678};
    constexpr uint64_t kObservedAtMonotonicNs{123456789};
    constexpr int64_t kObservedAtUnixNs{1700000000000000000};
    auto ipcClient = std::make_shared<StrictMock<ipc::ClientMock>>();
    MediaPipelineClient client{kSessionId, ipcClient};

    EXPECT_CALL(*ipcClient, sendEvent(_))
        .WillOnce(Invoke(
            [=](const std::shared_ptr<google::protobuf::Message> &message)
            {
                auto event = std::dynamic_pointer_cast<MediaPipelineMetricEvent>(message);
                if (!event)
                {
                    ADD_FAILURE() << "Unexpected event type";
                    return false;
                }

                std::string payload;
                EXPECT_TRUE(event->SerializeToString(&payload));

                MediaPipelineMetricEvent decoded;
                EXPECT_TRUE(decoded.ParseFromString(payload));
                EXPECT_EQ(static_cast<uint32_t>(kSessionId), decoded.session_id());
                EXPECT_EQ(kSourceId, decoded.source_id());
                EXPECT_EQ(9001, decoded.pts_ns());
                EXPECT_EQ(3U, static_cast<uint32_t>(decoded.type()));
                EXPECT_EQ(kPhase, static_cast<uint32_t>(decoded.phase()));
                EXPECT_EQ(kCount, decoded.count());
                EXPECT_EQ(kDurationNs, decoded.duration_ns());
                EXPECT_EQ(kObservedAtMonotonicNs, decoded.observed_at_monotonic_ns());
                EXPECT_EQ(kObservedAtUnixNs, decoded.observed_at_unix_ns());
                return true;
            }));

    MetricEvent metric;
    metric.type = MetricType::VIDEO_DECODE_ERROR;
    metric.phase = MetricPhase::EPISODE_STARTED;
    metric.sourceId = kSourceId;
    metric.presentationTimeStampNs = 9001;
    metric.count = kCount;
    metric.durationNs = kDurationNs;
    metric.observedAtMonotonicNs = kObservedAtMonotonicNs;
    metric.observedAtUnixNs = kObservedAtUnixNs;
    client.notifyMetric(metric);
}
