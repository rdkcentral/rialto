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

#include "EventThreadFactoryMock.h"
#include "EventThreadMock.h"
#include "IpcModuleBase.h"
#include "PrivateMetricsIpc.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using namespace firebolt::rialto::common;
using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;
using testing::WithArgs;

class PrivateMetricsIpcClientMock : public IPrivateMetricsIpcClient
{
public:
    MOCK_METHOD(void, reportClientMetrics, (std::uint64_t sampleId, std::uint32_t reason), (override));
};

class PrivateMetricsIpcTests : public IpcModuleBase, public testing::Test
{
protected:
    void createIpc()
    {
        expectInitIpc();
        EXPECT_CALL(*m_eventThreadFactory, createEventThread("rialto-metrics-events"))
            .WillOnce(Return(ByMove(std::move(m_eventThread))));
        EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.MetricsSampleRequestEvent", _, _))
            .WillOnce(Invoke(
                [this](const std::string &, const google::protobuf::Descriptor *,
                       std::function<void(const std::shared_ptr<google::protobuf::Message> &)> &&handler)
                {
                    m_eventCallback = std::move(handler);
                    return kEventTag;
                }));
        expectIpcApiCallSuccess();
        EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("notifyClientReady"), m_controllerMock.get(), _, _,
                                               m_blockingClosureMock.get()));
        m_sut = std::make_unique<PrivateMetricsIpc>(&m_client, *m_ipcClientMock, m_eventThreadFactory);
    }

    void destroyIpc()
    {
        EXPECT_CALL(*m_channelMock, unsubscribe(kEventTag)).WillOnce(Return(true));
        m_sut.reset();
    }

    static constexpr int kEventTag{6};
    StrictMock<PrivateMetricsIpcClientMock> m_client;
    std::shared_ptr<StrictMock<EventThreadFactoryMock>> m_eventThreadFactory{
        std::make_shared<StrictMock<EventThreadFactoryMock>>()};
    std::unique_ptr<StrictMock<EventThreadMock>> m_eventThread{std::make_unique<StrictMock<EventThreadMock>>()};
    StrictMock<EventThreadMock> *m_eventThreadMock{m_eventThread.get()};
    std::function<void(const std::shared_ptr<google::protobuf::Message> &)> m_eventCallback;
    std::unique_ptr<PrivateMetricsIpc> m_sut;
};

TEST_F(PrivateMetricsIpcTests, reportsMetricsAndForwardsSampleEvent)
{
    createIpc();
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("reportClientMetrics"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<2>(Invoke(
            [](const google::protobuf::Message *request)
            {
                const auto *report{dynamic_cast<const ReportClientMetricsRequest *>(request)};
                ASSERT_NE(report, nullptr);
                EXPECT_EQ(report->metrics().sample_id(), 12);
                EXPECT_EQ(report->metrics().reason(), METRICS_SAMPLE_REASON_PERIODIC);
                EXPECT_EQ(report->metrics().app_name(), "app");
                EXPECT_EQ(report->metrics().process_id(), 42);
                EXPECT_EQ(report->metrics().monotonic_time_ms(), 100);
                EXPECT_EQ(report->metrics().epoch_time_ms(), 200);
                EXPECT_EQ(report->metrics().process_cpu_time_ms(), 300);
                EXPECT_EQ(report->metrics().process_memory_kb(), 400);
            })));
    EXPECT_TRUE(m_sut->reportClientMetrics(12, METRICS_SAMPLE_REASON_PERIODIC, "app", 42, 100, 200, 300, 400));

    auto event{std::make_shared<MetricsSampleRequestEvent>()};
    event->set_sample_id(13);
    event->set_reason(METRICS_SAMPLE_REASON_STATE_TRANSITION);
    std::function<void()> eventTask;
    EXPECT_CALL(*m_eventThreadMock, addImpl(_))
        .WillOnce(Invoke([&eventTask](std::function<void()> &&task) { eventTask = std::move(task); }));
    m_eventCallback(event);
    ASSERT_TRUE(static_cast<bool>(eventTask));
    EXPECT_CALL(m_client, reportClientMetrics(13, METRICS_SAMPLE_REASON_STATE_TRANSITION));
    eventTask();
    destroyIpc();
}

TEST_F(PrivateMetricsIpcTests, reportsRpcFailure)
{
    createIpc();
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("reportClientMetrics"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()));
    EXPECT_FALSE(m_sut->reportClientMetrics(1, METRICS_SAMPLE_REASON_CONNECTED, "", 0, 0, 0, 0, 0));
    destroyIpc();
}
