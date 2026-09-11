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

#include "ClientComponentTest.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <unistd.h>

namespace firebolt::rialto::client::ct
{
class PrivateMetricsTest : public ClientComponentTest
{
};

TEST_F(PrivateMetricsTest, reportsMetricsWhenSampleIsRequested)
{
    constexpr std::uint64_t kSampleId{42};
    constexpr auto kReason{::firebolt::rialto::METRICS_SAMPLE_REASON_PERIODIC};

    EXPECT_CALL(*m_privateMetricsModuleMock, notifyClientReady(_, _, _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_privateMetricsModuleMock), &PrivateMetricsModuleMock::defaultReturn)));

    // ClientController is process-wide. Complete the lifecycle so the asynchronous
    // disconnect is handled before the next test fixture starts.
    ClientComponentTest::startApplicationRunning();

    EXPECT_CALL(*m_privateMetricsModuleMock, reportClientMetrics(_, _, _, _))
        .WillOnce(Invoke(
            [this, kSampleId,
             kReason](::google::protobuf::RpcController *, const ::firebolt::rialto::ReportClientMetricsRequest *request,
                      ::firebolt::rialto::ReportClientMetricsResponse *, ::google::protobuf::Closure *done)
            {
                EXPECT_NE(request, nullptr);
                if (request && request->has_metrics())
                {
                    EXPECT_EQ(request->metrics().sample_id(), kSampleId);
                    EXPECT_EQ(request->metrics().reason(), kReason);
                    EXPECT_EQ(request->metrics().process_id(), static_cast<std::uint32_t>(getpid()));
                }
                else
                {
                    ADD_FAILURE() << "Missing client process metrics";
                }
                done->Run();
                notifyEvent();
            }));

    sendMetricsSampleRequestEvent(kSampleId, kReason);
    waitEvent();

    ClientComponentTest::stopApplication();
}
} // namespace firebolt::rialto::client::ct
