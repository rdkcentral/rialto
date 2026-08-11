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

#include "ClientMock.h"
#include "ClosureMock.h"
#include "IpcControllerMock.h"
#include "PrivateMetricsModuleService.h"
#include "PrivateMetricsServiceMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::server::ipc;
using namespace firebolt::rialto::server::service;
using testing::_;
using testing::Invoke;
using testing::Return;
using testing::SaveArg;
using testing::StrictMock;

MATCHER_P(MetricsSampleRequestMatcher, expectedReason, "")
{
    auto event{std::dynamic_pointer_cast<MetricsSampleRequestEvent>(arg)};
    return event && event->sample_id() == 12 && event->reason() == expectedReason;
}

TEST(PrivateMetricsModuleServiceTests, handlesClientLifecycleReportsAndSampleRequests)
{
    StrictMock<PrivateMetricsServiceMock> metricsService;
    auto client{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()};
    StrictMock<firebolt::rialto::ipc::ControllerMock> controller;
    StrictMock<firebolt::rialto::ipc::ClosureMock> closure;
    auto sut{std::make_shared<PrivateMetricsModuleService>(metricsService)};

    EXPECT_CALL(*client, exportService(_));
    sut->clientConnected(client);

    std::shared_ptr<IMetricsCollectorClient> collectorClient;
    EXPECT_CALL(controller, getClient()).WillOnce(Return(client));
    EXPECT_CALL(closure, Run());
    EXPECT_CALL(metricsService, clientReady(1, _)).WillOnce(SaveArg<1>(&collectorClient));
    NotifyClientReadyRequest readyRequest;
    NotifyClientReadyResponse readyResponse;
    sut->notifyClientReady(&controller, &readyRequest, &readyResponse, &closure);
    ASSERT_NE(collectorClient, nullptr);

    EXPECT_CALL(*client, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*client, sendEvent(MetricsSampleRequestMatcher(METRICS_SAMPLE_REASON_PERIODIC))).WillOnce(Return(true));
    collectorClient->requestMetricsSample(1, 12, firebolt::rialto::server::MetricsSampleReason::PERIODIC);

    ReportClientMetricsRequest reportRequest;
    auto *protoMetrics{reportRequest.mutable_metrics()};
    protoMetrics->set_sample_id(12);
    protoMetrics->set_reason(METRICS_SAMPLE_REASON_PERIODIC);
    protoMetrics->set_app_name("test-app");
    protoMetrics->set_process_id(42);
    protoMetrics->set_monotonic_time_ms(100);
    protoMetrics->set_epoch_time_ms(200);
    protoMetrics->set_process_cpu_time_ms(300);
    protoMetrics->set_process_memory_kb(400);
    ReportClientMetricsResponse reportResponse;
    EXPECT_CALL(controller, getClient()).WillOnce(Return(client));
    EXPECT_CALL(closure, Run());
    EXPECT_CALL(metricsService, reportMetrics(1, _))
        .WillOnce(Invoke(
            [](int, const ClientMetricsData &metrics)
            {
                EXPECT_EQ(metrics.sampleId, 12);
                EXPECT_EQ(metrics.reason, firebolt::rialto::server::MetricsSampleReason::PERIODIC);
                EXPECT_EQ(metrics.appName, "test-app");
                EXPECT_EQ(metrics.processId, 42);
                EXPECT_EQ(metrics.monotonicTimeMs, 100);
                EXPECT_EQ(metrics.epochTimeMs, 200);
                EXPECT_EQ(metrics.processCpuTimeMs, 300);
                EXPECT_EQ(metrics.processMemoryKb, 400);
            }));
    sut->reportClientMetrics(&controller, &reportRequest, &reportResponse, &closure);

    EXPECT_CALL(metricsService, notifyApplicationStateChanged(ApplicationState::RUNNING, ApplicationState::INACTIVE));
    sut->notifyApplicationStateChanged(ApplicationState::RUNNING, ApplicationState::INACTIVE);

    EXPECT_CALL(metricsService, clientDisconnected(1));
    sut->clientDisconnected(client);
}

TEST(PrivateMetricsModuleServiceTests, factoryCreatesService)
{
    StrictMock<PrivateMetricsServiceMock> metricsService;
    PrivateMetricsModuleServiceFactory factory;
    EXPECT_NE(factory.create(metricsService), nullptr);
    EXPECT_NE(IPrivateMetricsModuleServiceFactory::createFactory(), nullptr);
}
