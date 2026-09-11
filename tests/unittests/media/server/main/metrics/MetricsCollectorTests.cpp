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

#include "MetricsCollector.h"
#include "MetricsCollectorClientMock.h"
#include "TimerFactoryMock.h"
#include "TimerMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

class MetricsCollectorTests : public testing::Test
{
protected:
    void createCollector(ApplicationState initialApplicationState = ApplicationState::UNKNOWN)
    {
        auto timer{std::make_unique<StrictMock<TimerMock>>()};
        m_timer = timer.get();
        EXPECT_CALL(*m_timerFactory, createTimer(std::chrono::milliseconds{15000}, _, common::TimerType::PERIODIC))
            .WillOnce(Invoke(
                [this, &timer](const std::chrono::milliseconds &, const std::function<void()> &callback, common::TimerType)
                {
                    m_timerCallback = callback;
                    return std::move(timer);
                }));
        EXPECT_CALL(*m_client, requestMetricsSample(kClientId, 1, MetricsSampleReason::CONNECTED));
        m_sut = std::make_unique<MetricsCollector>(kClientId, m_client, m_timerFactory, initialApplicationState);
    }

    void destroyCollector()
    {
        EXPECT_CALL(*m_timer, cancel());
        m_sut.reset();
    }

    static constexpr int kClientId{3};
    std::shared_ptr<StrictMock<MetricsCollectorClientMock>> m_client{
        std::make_shared<StrictMock<MetricsCollectorClientMock>>()};
    std::shared_ptr<StrictMock<TimerFactoryMock>> m_timerFactory{std::make_shared<StrictMock<TimerFactoryMock>>()};
    StrictMock<TimerMock> *m_timer{nullptr};
    std::function<void()> m_timerCallback;
    std::unique_ptr<MetricsCollector> m_sut;
};

TEST_F(MetricsCollectorTests, doesNotQueueRequestsWhileClientIsUnresponsive)
{
    createCollector();
    EXPECT_CALL(*m_client, requestMetricsSample(kClientId, 2, MetricsSampleReason::PERIODIC));
    m_timerCallback();
    m_timerCallback();
    EXPECT_CALL(*m_client, requestMetricsSample(kClientId, 3, MetricsSampleReason::PERIODIC));
    m_timerCallback();

    ClientMetricsData response;
    response.sampleId = 3;
    response.reason = MetricsSampleReason::PERIODIC;
    response.monotonicTimeMs = 1000;
    response.processCpuTimeMs = 100;
    response.processMemoryKb = 1000;
    m_sut->processMetrics(response);

    EXPECT_CALL(*m_client, requestMetricsSample(kClientId, 4, MetricsSampleReason::PERIODIC));
    m_timerCallback();
    destroyCollector();
}

TEST_F(MetricsCollectorTests, startsWithCurrentApplicationStateWithoutRequestingAnotherSample)
{
    createCollector(ApplicationState::RUNNING);
    destroyCollector();
}

TEST_F(MetricsCollectorTests, processesSamplesAndStateBoundaries)
{
    createCollector();
    ClientMetricsData baseline;
    baseline.sampleId = 1;
    baseline.reason = MetricsSampleReason::CONNECTED;
    baseline.monotonicTimeMs = 1000;
    baseline.processCpuTimeMs = 100;
    baseline.processMemoryKb = 1000;
    m_sut->processMetrics(baseline);

    EXPECT_CALL(*m_client, requestMetricsSample(kClientId, 2, MetricsSampleReason::STATE_TRANSITION));
    m_sut->notifyApplicationStateChanged(ApplicationState::UNKNOWN, ApplicationState::RUNNING);
    m_sut->notifyPlaybackStateChanged(10, PlaybackState::UNKNOWN, PlaybackState::PLAYING);
    EXPECT_CALL(*m_client, requestMetricsSample(kClientId, 3, MetricsSampleReason::STATE_TRANSITION));
    m_sut->notifyPlaybackStateChanged(10, PlaybackState::PLAYING, PlaybackState::PAUSED);
    m_sut->notifyWebAudioPlayerStateChanged(11, WebAudioPlayerState::UNKNOWN, WebAudioPlayerState::PLAYING);
    EXPECT_CALL(*m_client, requestMetricsSample(kClientId, 4, MetricsSampleReason::STATE_TRANSITION));
    m_sut->notifyWebAudioPlayerStateChanged(11, WebAudioPlayerState::PLAYING, WebAudioPlayerState::PAUSED);

    ClientMetricsData periodic{baseline};
    periodic.sampleId = 5;
    periodic.reason = MetricsSampleReason::PERIODIC;
    periodic.monotonicTimeMs = 2000;
    periodic.processCpuTimeMs = 200;
    periodic.processMemoryKb = 1100;
    m_sut->processMetrics(periodic);

    EXPECT_CALL(*m_client, requestMetricsSample(kClientId, 5, MetricsSampleReason::STATE_TRANSITION));
    m_sut->notifyApplicationStateChanged(ApplicationState::RUNNING, ApplicationState::INACTIVE);
    EXPECT_CALL(*m_client, requestMetricsSample(kClientId, 6, MetricsSampleReason::STATE_TRANSITION));
    m_sut->notifyPlaybackStateChanged(10, PlaybackState::PAUSED, PlaybackState::STOPPED);
    EXPECT_CALL(*m_client, requestMetricsSample(kClientId, 7, MetricsSampleReason::STATE_TRANSITION));
    m_sut->notifyWebAudioPlayerStateChanged(11, WebAudioPlayerState::PAUSED, WebAudioPlayerState::END_OF_STREAM);
    destroyCollector();
}
