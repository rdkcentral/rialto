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
#include "RpcControllerMock.h"
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

MATCHER_P2(MetricsSampleRequestMatcher, expectedSampleId, expectedReason, "")
{
    auto event{std::dynamic_pointer_cast<MetricsSampleRequestEvent>(arg)};
    return event && event->sample_id() == static_cast<std::uint64_t>(expectedSampleId) &&
           event->reason() == expectedReason;
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
    EXPECT_CALL(*client, sendEvent(MetricsSampleRequestMatcher(12, METRICS_SAMPLE_REASON_PERIODIC))).WillOnce(Return(true));
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

    EXPECT_CALL(metricsService, notifyApplicationStateChanged(ApplicationState::INACTIVE));
    sut->notifyApplicationStateChanged(ApplicationState::INACTIVE);

    EXPECT_CALL(metricsService, clientDisconnected(1));
    sut->clientDisconnected(client);
}

TEST(PrivateMetricsModuleServiceTests, rejectsIncompatibleControllers)
{
    StrictMock<PrivateMetricsServiceMock> metricsService;
    StrictMock<firebolt::rialto::ipc::RpcControllerMock> controller;
    StrictMock<firebolt::rialto::ipc::ClosureMock> closure;
    auto sut{std::make_shared<PrivateMetricsModuleService>(metricsService)};

    EXPECT_CALL(controller, SetFailed("ipc library provided incompatible controller object")).Times(2);
    EXPECT_CALL(closure, Run()).Times(2);

    NotifyClientReadyRequest readyRequest;
    NotifyClientReadyResponse readyResponse;
    sut->notifyClientReady(&controller, &readyRequest, &readyResponse, &closure);

    ReportClientMetricsRequest reportRequest;
    ReportClientMetricsResponse reportResponse;
    sut->reportClientMetrics(&controller, &reportRequest, &reportResponse, &closure);
}

TEST(PrivateMetricsModuleServiceTests, duplicateReadyNotificationReusesClientId)
{
    StrictMock<PrivateMetricsServiceMock> metricsService;
    auto client{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()};
    StrictMock<firebolt::rialto::ipc::ControllerMock> controller;
    StrictMock<firebolt::rialto::ipc::ClosureMock> closure;
    auto sut{std::make_shared<PrivateMetricsModuleService>(metricsService)};

    EXPECT_CALL(controller, getClient()).Times(2).WillRepeatedly(Return(client));
    EXPECT_CALL(closure, Run()).Times(2);
    EXPECT_CALL(metricsService, clientReady(1, _));
    NotifyClientReadyRequest readyRequest;
    NotifyClientReadyResponse readyResponse;
    sut->notifyClientReady(&controller, &readyRequest, &readyResponse, &closure);
    sut->notifyClientReady(&controller, &readyRequest, &readyResponse, &closure);

    EXPECT_CALL(metricsService, clientDisconnected(1));
    sut->clientDisconnected(client);
}

TEST(PrivateMetricsModuleServiceTests, rejectsMissingMetricsAndIgnoresUnknownClient)
{
    StrictMock<PrivateMetricsServiceMock> metricsService;
    auto client{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()};
    StrictMock<firebolt::rialto::ipc::ControllerMock> controller;
    StrictMock<firebolt::rialto::ipc::ClosureMock> closure;
    auto sut{std::make_shared<PrivateMetricsModuleService>(metricsService)};
    ReportClientMetricsResponse response;

    ReportClientMetricsRequest missingMetricsRequest;
    EXPECT_CALL(controller, SetFailed("Missing metrics"));
    EXPECT_CALL(closure, Run());
    sut->reportClientMetrics(&controller, &missingMetricsRequest, &response, &closure);

    ReportClientMetricsRequest unknownClientRequest;
    unknownClientRequest.mutable_metrics();
    EXPECT_CALL(controller, getClient()).WillOnce(Return(client));
    EXPECT_CALL(closure, Run());
    sut->reportClientMetrics(&controller, &unknownClientRequest, &response, &closure);
}

TEST(PrivateMetricsModuleServiceTests, mapsAllReportedSampleReasons)
{
    StrictMock<PrivateMetricsServiceMock> metricsService;
    auto client{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()};
    StrictMock<firebolt::rialto::ipc::ControllerMock> controller;
    StrictMock<firebolt::rialto::ipc::ClosureMock> closure;
    auto sut{std::make_shared<PrivateMetricsModuleService>(metricsService)};

    EXPECT_CALL(controller, getClient()).WillOnce(Return(client));
    EXPECT_CALL(closure, Run());
    EXPECT_CALL(metricsService, clientReady(1, _));
    NotifyClientReadyRequest readyRequest;
    NotifyClientReadyResponse readyResponse;
    sut->notifyClientReady(&controller, &readyRequest, &readyResponse, &closure);

    auto reportReason = [&](firebolt::rialto::MetricsSampleReason protoReason,
                            firebolt::rialto::server::MetricsSampleReason expectedReason)
    {
        ReportClientMetricsRequest request;
        request.mutable_metrics()->set_reason(protoReason);
        ReportClientMetricsResponse response;
        EXPECT_CALL(controller, getClient()).WillOnce(Return(client));
        EXPECT_CALL(closure, Run());
        EXPECT_CALL(metricsService, reportMetrics(1, _))
            .WillOnce(Invoke([expectedReason](int, const ClientMetricsData &metrics)
                             { EXPECT_EQ(metrics.reason, expectedReason); }));
        sut->reportClientMetrics(&controller, &request, &response, &closure);
    };

    reportReason(METRICS_SAMPLE_REASON_CONNECTED, firebolt::rialto::server::MetricsSampleReason::CONNECTED);
    reportReason(METRICS_SAMPLE_REASON_STATE_TRANSITION, firebolt::rialto::server::MetricsSampleReason::STATE_TRANSITION);
    reportReason(METRICS_SAMPLE_REASON_UNKNOWN, firebolt::rialto::server::MetricsSampleReason::UNKNOWN);
}

TEST(PrivateMetricsModuleServiceTests, handlesUnavailableClientsAndSendsAllSampleRequestReasons)
{
    StrictMock<PrivateMetricsServiceMock> metricsService;
    auto client{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()};
    StrictMock<firebolt::rialto::ipc::ControllerMock> controller;
    StrictMock<firebolt::rialto::ipc::ClosureMock> closure;
    auto sut{std::make_shared<PrivateMetricsModuleService>(metricsService)};

    EXPECT_CALL(controller, getClient()).WillOnce(Return(client));
    EXPECT_CALL(closure, Run());
    EXPECT_CALL(metricsService, clientReady(1, _));
    NotifyClientReadyRequest readyRequest;
    NotifyClientReadyResponse readyResponse;
    sut->notifyClientReady(&controller, &readyRequest, &readyResponse, &closure);

    sut->requestMetricsSample(99, 20, firebolt::rialto::server::MetricsSampleReason::CONNECTED);

    EXPECT_CALL(*client, isConnected()).WillOnce(Return(false));
    sut->requestMetricsSample(1, 21, firebolt::rialto::server::MetricsSampleReason::CONNECTED);

    EXPECT_CALL(*client, isConnected()).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*client, sendEvent(MetricsSampleRequestMatcher(22, METRICS_SAMPLE_REASON_CONNECTED))).WillOnce(Return(true));
    EXPECT_CALL(*client, sendEvent(MetricsSampleRequestMatcher(23, METRICS_SAMPLE_REASON_STATE_TRANSITION)))
        .WillOnce(Return(true));
    EXPECT_CALL(*client, sendEvent(MetricsSampleRequestMatcher(24, METRICS_SAMPLE_REASON_UNKNOWN))).WillOnce(Return(false));

    sut->requestMetricsSample(1, 22, firebolt::rialto::server::MetricsSampleReason::CONNECTED);
    sut->requestMetricsSample(1, 23, firebolt::rialto::server::MetricsSampleReason::STATE_TRANSITION);
    sut->requestMetricsSample(1, 24, firebolt::rialto::server::MetricsSampleReason::UNKNOWN);
}

TEST(PrivateMetricsModuleServiceTests, factoryCreatesService)
{
    StrictMock<PrivateMetricsServiceMock> metricsService;
    PrivateMetricsModuleServiceFactory factory;
    EXPECT_NE(factory.create(metricsService), nullptr);
    EXPECT_NE(IPrivateMetricsModuleServiceFactory::createFactory(), nullptr);
}
