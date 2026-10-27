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
                EXPECT_EQ(101U, decoded.rendered_frames());
                EXPECT_EQ(7U, decoded.dropped_frames());
                EXPECT_EQ(3U, decoded.corrupted_frames());
                EXPECT_EQ(9001U, decoded.pts_ns());
                EXPECT_EQ("decoder error", decoded.error_info());
                return true;
            }));

    client.notifyMetricEvent(kSourceId, 101, 7, 3, 9001, "decoder error");
}