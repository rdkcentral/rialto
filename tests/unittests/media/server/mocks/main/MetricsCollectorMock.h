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

#ifndef FIREBOLT_RIALTO_SERVER_METRICS_COLLECTOR_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_COLLECTOR_MOCK_H_

#include "IMetricsCollector.h"
#include <memory>
#include <gmock/gmock.h>

namespace firebolt::rialto::server
{
class MetricsCollectorMock : public IMetricsCollector
{
public:
    MOCK_METHOD(void, processMetrics, (const ClientMetricsData &metrics), (override));
    MOCK_METHOD(void, notifyPlaybackStateChanged, (int sessionId, PlaybackState oldState, PlaybackState newState),
                (override));
    MOCK_METHOD(void, notifyWebAudioPlayerStateChanged,
                (int handle, WebAudioPlayerState oldState, WebAudioPlayerState newState), (override));
    MOCK_METHOD(void, notifyApplicationStateChanged, (ApplicationState oldState, ApplicationState newState), (override));
};

class MetricsCollectorFactoryMock : public IMetricsCollectorFactory
{
public:
    MOCK_METHOD(std::unique_ptr<IMetricsCollector>, create,
                (int clientId, const std::shared_ptr<IMetricsCollectorClient> &client,
                 ApplicationState initialApplicationState),
                (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_METRICS_COLLECTOR_MOCK_H_
