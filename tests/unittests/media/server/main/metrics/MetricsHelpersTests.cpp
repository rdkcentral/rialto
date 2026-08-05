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

#include "CompositeMetricsReporter.h"
#include "MetricsAccumulator.h"
#include "MetricsReporterMock.h"
#include "MetricsThresholdChecker.h"
#include "StateMetricsAggregator.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto::server;
using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::StrictMock;

TEST(MetricsAccumulatorTests, calculatesStatisticsAndResets)
{
    MetricsAccumulator sut;
    EXPECT_EQ(sut.getStats().count, 0);

    sut.addSample(1.0);
    sut.addSample(2.0);
    sut.addSample(3.0);
    const auto stats{sut.getStats()};
    EXPECT_EQ(stats.count, 3);
    EXPECT_DOUBLE_EQ(stats.min, 1.0);
    EXPECT_DOUBLE_EQ(stats.max, 3.0);
    EXPECT_DOUBLE_EQ(stats.mean, 2.0);
    EXPECT_DOUBLE_EQ(stats.stddev, 1.0);

    sut.reset();
    EXPECT_EQ(sut.getCount(), 0);
}

TEST(StateMetricsAggregatorTests, finalizesAllMetricsForState)
{
    StateMetricsAggregator sut;
    sut.begin("PLAYING", 100);
    sut.addSample(MetricsSample{1.0, 2.0, 3.0, 4, 5, 6, 7});
    sut.addSample(MetricsSample{3.0, 4.0, 5.0, 6, 7, 8, 9});

    const auto report{sut.finalize(250)};
    EXPECT_TRUE(sut.hasData());
    EXPECT_EQ(sut.getStateName(), "PLAYING");
    EXPECT_EQ(report.stateName, "PLAYING");
    EXPECT_EQ(report.durationMs, 150);
    EXPECT_DOUBLE_EQ(report.clientCpu.mean, 2.0);
    EXPECT_DOUBLE_EQ(report.serverCpu.mean, 3.0);
    EXPECT_DOUBLE_EQ(report.combinedCpu.mean, 4.0);
    EXPECT_DOUBLE_EQ(report.clientMemoryKb.mean, 5.0);
    EXPECT_DOUBLE_EQ(report.serverMemoryKb.mean, 6.0);
    EXPECT_DOUBLE_EQ(report.cgroupMemoryUsageKb.mean, 7.0);
    EXPECT_DOUBLE_EQ(report.cgroupMemoryLimitKb.mean, 8.0);

    sut.reset();
    EXPECT_FALSE(sut.hasData());
    EXPECT_EQ(sut.finalize(10).durationMs, 10);
}

TEST(CompositeMetricsReporterTests, forwardsEveryReportAndIgnoresNullReporter)
{
    CompositeMetricsReporter sut;
    auto first{std::make_unique<StrictMock<MetricsReporterMock>>()};
    auto second{std::make_unique<StrictMock<MetricsReporterMock>>()};
    auto *firstMock{first.get()};
    auto *secondMock{second.get()};
    sut.addReporter(nullptr);
    sut.addReporter(std::move(first));
    sut.addReporter(std::move(second));

    PeriodicMetricsReport periodic;
    StateTransitionReport transition;
    ThresholdAlert alert;
    EXPECT_CALL(*firstMock, reportPeriodicSample(testing::Ref(periodic)));
    EXPECT_CALL(*secondMock, reportPeriodicSample(testing::Ref(periodic)));
    sut.reportPeriodicSample(periodic);
    EXPECT_CALL(*firstMock, reportStateTransition(testing::Ref(transition)));
    EXPECT_CALL(*secondMock, reportStateTransition(testing::Ref(transition)));
    sut.reportStateTransition(transition);
    EXPECT_CALL(*firstMock, reportThresholdExceeded(testing::Ref(alert)));
    EXPECT_CALL(*secondMock, reportThresholdExceeded(testing::Ref(alert)));
    sut.reportThresholdExceeded(alert);
}

TEST(MetricsThresholdCheckerTests, reportsOnceAndRearmsAfterTwoLowerSamples)
{
    StrictMock<MetricsReporterMock> reporter;
    MetricsThresholdConfig config;
    MetricsThresholdChecker sut{config, &reporter};

    EXPECT_CALL(reporter, reportThresholdExceeded(_))
        .WillOnce(Invoke([](const ThresholdAlert &alert)
                         {
                             EXPECT_EQ(alert.metricName, "client_cpu");
                             EXPECT_EQ(alert.severity, ThresholdSeverity::CRITICAL);
                         }));
    sut.checkSample(96.0, 0.0, 0.0, 0, 0, 0, 0);
    sut.checkSample(96.0, 0.0, 0.0, 0, 0, 0, 0);

    sut.checkSample(0.0, 0.0, 0.0, 0, 0, 0, 0);
    sut.checkSample(0.0, 0.0, 0.0, 0, 0, 0, 0);
    EXPECT_CALL(reporter, reportThresholdExceeded(_))
        .WillOnce(Invoke([](const ThresholdAlert &alert)
                         { EXPECT_EQ(alert.severity, ThresholdSeverity::CRITICAL); }));
    sut.checkSample(96.0, 0.0, 0.0, 0, 0, 0, 0);
}

TEST(MetricsThresholdCheckerTests, warningEscalatesToCriticalWithoutDuplicateWarning)
{
    StrictMock<MetricsReporterMock> reporter;
    MetricsThresholdChecker sut{MetricsThresholdConfig{}, &reporter};

    {
        testing::InSequence sequence;
        EXPECT_CALL(reporter, reportThresholdExceeded(_))
            .WillOnce(Invoke([](const ThresholdAlert &alert)
                             { EXPECT_EQ(alert.severity, ThresholdSeverity::WARNING); }));
        EXPECT_CALL(reporter, reportThresholdExceeded(_))
            .WillOnce(Invoke([](const ThresholdAlert &alert)
                             { EXPECT_EQ(alert.severity, ThresholdSeverity::CRITICAL); }));

        sut.checkSample(81.0, 0.0, 0.0, 0, 0, 0, 0);
        sut.checkSample(96.0, 0.0, 0.0, 0, 0, 0, 0);
    }

    sut.checkSample(90.0, 0.0, 0.0, 0, 0, 0, 0);
    sut.checkSample(0.0, 0.0, 0.0, 0, 0, 0, 0);
    sut.checkSample(0.0, 0.0, 0.0, 0, 0, 0, 0);

    EXPECT_CALL(reporter, reportThresholdExceeded(_))
        .WillOnce(Invoke([](const ThresholdAlert &alert)
                         { EXPECT_EQ(alert.severity, ThresholdSeverity::WARNING); }));
    sut.checkSample(81.0, 0.0, 0.0, 0, 0, 0, 0);
}

TEST(MetricsThresholdCheckerTests, reportsConfiguredMetricsIncludingCgroupPercentage)
{
    StrictMock<MetricsReporterMock> reporter;
    MetricsThresholdChecker sut{MetricsThresholdConfig{}, &reporter};

    EXPECT_CALL(reporter, reportThresholdExceeded(_)).Times(6);
    sut.checkSample(81.0, 81.0, 151.0, 512000, 512000, 81, 100);
}

TEST(MetricsThresholdCheckerTests, acceptsNullReporter)
{
    MetricsThresholdChecker sut{MetricsThresholdConfig{}, nullptr};
    sut.checkSample(100.0, 100.0, 200.0, 1000000, 1000000, 100, 100);
}
