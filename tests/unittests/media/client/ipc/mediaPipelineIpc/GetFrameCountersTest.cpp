#include "MediaPipelineIpcTestBase.h"

class RialtoClientMediaPipelineIpcGetFrameCountersTest : public MediaPipelineIpcTestBase
{
protected:
    void SetUp() override
    {
        MediaPipelineIpcTestBase::SetUp();
        createMediaPipelineIpc();
    }

    void TearDown() override
    {
        destroyMediaPipelineIpc();
        MediaPipelineIpcTestBase::TearDown();
    }

    const int32_t m_sourceId{4};
};

TEST_F(RialtoClientMediaPipelineIpcGetFrameCountersTest, ReturnsCounterSnapshot)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("getFrameCounters"), m_controllerMock.get(), _, _,
                           m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [this](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                   const google::protobuf::Message *request, google::protobuf::Message *response,
                   google::protobuf::Closure *)
            {
                const auto *frameCountersRequest =
                    dynamic_cast<const firebolt::rialto::GetFrameCountersRequest *>(request);
                ASSERT_NE(nullptr, frameCountersRequest);
                std::string requestPayload;
                ASSERT_TRUE(frameCountersRequest->SerializeToString(&requestPayload));
                firebolt::rialto::GetFrameCountersRequest decodedRequest;
                ASSERT_TRUE(decodedRequest.ParseFromString(requestPayload));
                EXPECT_EQ(m_sessionId, decodedRequest.session_id());
                EXPECT_EQ(m_sourceId, decodedRequest.source_id());

                auto *frameCountersResponse = dynamic_cast<firebolt::rialto::GetFrameCountersResponse *>(response);
                ASSERT_NE(nullptr, frameCountersResponse);
                frameCountersResponse->set_rendered_frames(101);
                frameCountersResponse->set_dropped_frames(7);
                frameCountersResponse->set_corrupted_frames(3);
                frameCountersResponse->set_timestamp(987654321);
            }));

    uint64_t renderedFrames{0};
    uint64_t droppedFrames{0};
    uint64_t corruptedFrames{0};
    uint64_t timestamp{0};
    ASSERT_TRUE(m_mediaPipelineIpc->getFrameCounters(m_sourceId, renderedFrames, droppedFrames, corruptedFrames,
                                                     timestamp));
    EXPECT_EQ(101U, renderedFrames);
    EXPECT_EQ(7U, droppedFrames);
    EXPECT_EQ(3U, corruptedFrames);
    EXPECT_EQ(987654321U, timestamp);
}

TEST_F(RialtoClientMediaPipelineIpcGetFrameCountersTest, ReturnsFalseWhenRpcFails)
{
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getFrameCounters"), _, _, _, _));

    uint64_t renderedFrames{0};
    uint64_t droppedFrames{0};
    uint64_t corruptedFrames{0};
    uint64_t timestamp{0};
    EXPECT_FALSE(m_mediaPipelineIpc->getFrameCounters(m_sourceId, renderedFrames, droppedFrames, corruptedFrames,
                                                      timestamp));
}