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

#include "LogMetricsReporter.h"
#include "RialtoLogging.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::logging;
using namespace firebolt::rialto::server;

namespace
{
std::size_t g_logCount{0};

void countLog(RIALTO_DEBUG_LEVEL, const char *, int, const char *, const char *, std::size_t)
{
    ++g_logCount;
}

PeriodicMetricsReport makeReport(ApplicationState state, std::uint64_t timeMs)
{
    PeriodicMetricsReport report;
    report.sampleId = timeMs;
    report.monotonicTimeMs = timeMs;
    report.reason = "PERIODIC";
    report.applicationState = state;
    report.clientCpuPercent = 20.0;
    report.serverCpuPercent = 10.0;
    report.combinedCpuPercent = 30.0;
    report.clientMemoryKb = 100000;
    report.serverMemoryKb = 200000;
    report.cgroupMemoryUsageKb = 300000;
    report.cgroupMemoryLimitKb = 400000;
    report.shmMemoryKb = 50000;
    return report;
}
} // namespace

class LogMetricsReporterTests : public testing::Test
{
protected:
    void SetUp() override
    {
        g_logCount = 0;
        m_previousLevels = getLogLevels(RIALTO_COMPONENT_SERVER);
        ASSERT_EQ(setLogHandler(RIALTO_COMPONENT_SERVER, countLog, true), RIALTO_LOGGING_STATUS_OK);
    }

    void TearDown() override
    {
        EXPECT_EQ(setLogHandler(RIALTO_COMPONENT_SERVER, nullptr, false), RIALTO_LOGGING_STATUS_OK);
        EXPECT_EQ(setLogLevels(RIALTO_COMPONENT_SERVER, m_previousLevels), RIALTO_LOGGING_STATUS_OK);
    }

    RIALTO_DEBUG_LEVEL m_previousLevels{RIALTO_DEBUG_LEVEL_DEFAULT};
};

TEST_F(LogMetricsReporterTests, inactiveSamplesOnlyLogAfterSignificantGaugeChange)
{
    LogMetricsReporter sut;
    auto report{makeReport(ApplicationState::INACTIVE, 0)};
    sut.reportPeriodicSample(report);
    EXPECT_EQ(g_logCount, 1);

    report.monotonicTimeMs = 10 * 60 * 1000;
    report.clientCpuTimeMs = 1000000;
    report.serverCpuTimeMs = 2000000;
    report.clientCpuPercent = 21.9;
    report.clientMemoryKb = 109000;
    sut.reportPeriodicSample(report);
    EXPECT_EQ(g_logCount, 1);

    report.clientCpuPercent = 22.0;
    sut.reportPeriodicSample(report);
    EXPECT_EQ(g_logCount, 2);
}

TEST_F(LogMetricsReporterTests, activeSamplesLogAtTenMinutesOrAfterSignificantChange)
{
    LogMetricsReporter sut;
    auto report{makeReport(ApplicationState::RUNNING, 100)};
    sut.reportPeriodicSample(report);
    EXPECT_EQ(g_logCount, 1);

    report.monotonicTimeMs += 10 * 60 * 1000 - 1;
    sut.reportPeriodicSample(report);
    EXPECT_EQ(g_logCount, 1);

    report.monotonicTimeMs += 1;
    sut.reportPeriodicSample(report);
    EXPECT_EQ(g_logCount, 2);

    report.monotonicTimeMs += 1;
    report.serverMemoryKb += 20000;
    sut.reportPeriodicSample(report);
    EXPECT_EQ(g_logCount, 3);
}

TEST_F(LogMetricsReporterTests, applicationStateChangesLogImmediatelyButNonPeriodicSamplesAreSuppressed)
{
    LogMetricsReporter sut;
    auto report{makeReport(ApplicationState::INACTIVE, 100)};
    sut.reportPeriodicSample(report);

    report.monotonicTimeMs = 101;
    report.applicationState = ApplicationState::RUNNING;
    sut.reportPeriodicSample(report);
    report.reason = "STATE_TRANSITION";
    sut.reportPeriodicSample(report);
    EXPECT_EQ(g_logCount, 2);
}

TEST_F(LogMetricsReporterTests, transitionAndThresholdReportsAreLogged)
{
    LogMetricsReporter sut;
    sut.reportStateTransition(StateTransitionReport{});
    sut.reportThresholdExceeded(ThresholdAlert{});
    EXPECT_EQ(g_logCount, 2);
}
