#include "ClientMock.h"
#include "MediaPipelineClient.h"
#include "mediapipelinemodule.pb.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server::ipc;
using ::testing::StrictMock;
using ::testing::_;
using ::testing::Invoke;

TEST(MediaPipelineClientTests, NotifyMetricEventSerializesAndPreservesPayload)
{
    constexpr int kSessionId{8};
    constexpr int32_t kSourceId{14};
    constexpr uint32_t kPhase{2};
    constexpr uint64_t kCount{12};
    constexpr uint64_t kDurationNs{45678};
    auto ipcClient = std::make_shared<StrictMock<ipc::ClientMock>>();
    MediaPipelineClient client{kSessionId, ipcClient};

    EXPECT_CALL(*ipcClient, sendEvent(_))
        .WillOnce(Invoke(
            [=](const std::shared_ptr<google::protobuf::Message> &message)
            {
                auto event = std::dynamic_pointer_cast<NotifyMetricEvent>(message);
                if (!event)
                {
                    ADD_FAILURE() << "Unexpected event type";
                    return false;
                }

                std::string payload;
                EXPECT_TRUE(event->SerializeToString(&payload));

                NotifyMetricEvent decoded;
                EXPECT_TRUE(decoded.ParseFromString(payload));
                EXPECT_EQ(kSessionId, decoded.session_id());
                EXPECT_EQ(kSourceId, decoded.source_id());
                EXPECT_EQ(9001U, decoded.pts_ns());
                EXPECT_EQ("METRIC_VIDEO_DECODE_ERROR", decoded.error_info());
                EXPECT_EQ(firebolt::rialto::MetricType::VIDEO_DECODE_ERROR, decoded.type());
                EXPECT_EQ(kPhase, decoded.phase());
                EXPECT_EQ(kCount, decoded.count());
                EXPECT_EQ(kDurationNs, decoded.duration_ns());
                EXPECT_GT(decoded.observed_at_monotonic_ns(), 0ULL);
                EXPECT_GT(decoded.observed_at_unix_ns(), 0LL);
                return true;
            }));

    client.notifyMetricEvent(kSourceId, 9001, "METRIC_VIDEO_DECODE_ERROR", kPhase, kCount, kDurationNs);
}