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

#include "WebAudioPlayerMetricsClient.h"

namespace firebolt::rialto::server::service
{
WebAudioPlayerMetricsClient::WebAudioPlayerMetricsClient(int handle,
                                                         const std::shared_ptr<IWebAudioPlayerClient> &client,
                                                         IPrivateMetricsService &metricsService)
    : m_handle{handle}, m_client{client}, m_metricsService{metricsService}
{
}

void WebAudioPlayerMetricsClient::notifyState(WebAudioPlayerState state)
{
    m_metricsService.notifyWebAudioPlayerStateChanged(m_handle, m_currentState, state);
    m_currentState = state;
    m_client->notifyState(state);
}
} // namespace firebolt::rialto::server::service
