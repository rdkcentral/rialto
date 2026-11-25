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

#include "MediaPipelineIpcTestBase.h"

class RialtoClientMediaPipelineIpcSetMetricSubscriptionTest : public MediaPipelineIpcTestBase
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
};

TEST_F(RialtoClientMediaPipelineIpcSetMetricSubscriptionTest, SendsSessionIdAndMetricTypes)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setMetricSubscription"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(Invoke(
            [this](const google::protobuf::MethodDescriptor *, google::protobuf::RpcController *,
                   const google::protobuf::Message *request, google::protobuf::Message *response,
                   google::protobuf::Closure *)
            {
                const auto *metricsRequest = dynamic_cast<const firebolt::rialto::SetMediaMetricsRequest *>(request);
                ASSERT_NE(nullptr, metricsRequest);
                EXPECT_EQ(m_sessionId, metricsRequest->session_id());
                ASSERT_EQ(1, metricsRequest->types_size());
                EXPECT_EQ(static_cast<uint32_t>(firebolt::rialto::MetricType::VIDEO_FRAME_DROP),
                          static_cast<uint32_t>(metricsRequest->types(0)));

                auto *metricsResponse = dynamic_cast<firebolt::rialto::SetMediaMetricsResponse *>(response);
                ASSERT_NE(nullptr, metricsResponse);
                metricsResponse->set_result(true);
            }));

    EXPECT_TRUE(m_mediaPipelineIpc->setMetricSubscription({firebolt::rialto::MetricType::VIDEO_FRAME_DROP}));
}

TEST_F(RialtoClientMediaPipelineIpcSetMetricSubscriptionTest, ReturnsFalseWhenRpcFails)
{
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setMetricSubscription"), _, _, _, _));

    EXPECT_FALSE(m_mediaPipelineIpc->setMetricSubscription({firebolt::rialto::MetricType::VIDEO_FRAME_DROP}));
}
