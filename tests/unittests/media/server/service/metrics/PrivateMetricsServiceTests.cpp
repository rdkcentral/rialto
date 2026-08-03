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

#include "MetricsCollectorClientMock.h"
#include "MetricsCollectorMock.h"
#include "PrivateMetricsService.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::server::service;
using testing::_;
using testing::ByMove;
using testing::Return;
using testing::StrictMock;

class PrivateMetricsServiceTests : public testing::Test
{
protected:
    static constexpr int kClientId{4};

    void addCollector()
    {
        auto collector{std::make_unique<StrictMock<MetricsCollectorMock>>()};
        m_collector = collector.get();
        EXPECT_CALL(*m_factory, create(kClientId, m_client, ApplicationState::UNKNOWN))
            .WillOnce(Return(ByMove(std::move(collector))));
        m_sut.clientReady(kClientId, m_client);
    }

    std::shared_ptr<StrictMock<MetricsCollectorFactoryMock>> m_factory{
        std::make_shared<StrictMock<MetricsCollectorFactoryMock>>()};
    std::shared_ptr<IMetricsCollectorClient> m_client{std::make_shared<StrictMock<MetricsCollectorClientMock>>()};
    PrivateMetricsService m_sut{m_factory};
    StrictMock<MetricsCollectorMock> *m_collector{nullptr};
};

TEST_F(PrivateMetricsServiceTests, routesMetricsAndStateChangesToCollector)
{
    addCollector();
    ClientMetricsData metrics;
    metrics.sampleId = 17;

    EXPECT_CALL(*m_collector, processMetrics(testing::Ref(metrics)));
    m_sut.reportMetrics(kClientId, metrics);
    EXPECT_CALL(*m_collector,
                notifyPlaybackStateChanged(1, PlaybackState::PLAYING, PlaybackState::PAUSED));
    m_sut.notifyPlaybackStateChanged(1, PlaybackState::PLAYING, PlaybackState::PAUSED);
    EXPECT_CALL(*m_collector,
                notifyWebAudioPlayerStateChanged(2, WebAudioPlayerState::PLAYING, WebAudioPlayerState::PAUSED));
    m_sut.notifyWebAudioPlayerStateChanged(2, WebAudioPlayerState::PLAYING, WebAudioPlayerState::PAUSED);
    EXPECT_CALL(*m_collector,
                notifyApplicationStateChanged(ApplicationState::UNKNOWN, ApplicationState::RUNNING));
    m_sut.notifyApplicationStateChanged(ApplicationState::UNKNOWN, ApplicationState::RUNNING);
}

TEST_F(PrivateMetricsServiceTests, disconnectRemovesCollector)
{
    addCollector();
    m_sut.clientDisconnected(kClientId);
    m_collector = nullptr;

    m_sut.reportMetrics(kClientId, ClientMetricsData{});
    m_sut.notifyPlaybackStateChanged(1, PlaybackState::UNKNOWN, PlaybackState::PLAYING);
}

TEST_F(PrivateMetricsServiceTests, collectorInheritsCurrentApplicationStateWhenClientConnects)
{
    m_sut.notifyApplicationStateChanged(ApplicationState::UNKNOWN, ApplicationState::INACTIVE);
    m_sut.notifyApplicationStateChanged(ApplicationState::INACTIVE, ApplicationState::RUNNING);

    auto collector{std::make_unique<StrictMock<MetricsCollectorMock>>()};
    m_collector = collector.get();
    EXPECT_CALL(*m_factory, create(kClientId, m_client, ApplicationState::RUNNING))
        .WillOnce(Return(ByMove(std::move(collector))));
    m_sut.clientReady(kClientId, m_client);
}

TEST_F(PrivateMetricsServiceTests, ignoresFactoryFailureAndUnknownDisconnect)
{
    EXPECT_CALL(*m_factory, create(kClientId, m_client, ApplicationState::UNKNOWN))
        .WillOnce(Return(ByMove(std::unique_ptr<IMetricsCollector>{})));
    m_sut.clientReady(kClientId, m_client);
    m_sut.clientDisconnected(kClientId);
}
