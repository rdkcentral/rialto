/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "HealthcheckService.h"

namespace rialto::servermanager::common
{
HealthcheckService::HealthcheckService(ISessionServerAppManager &sessionServerAppManager,
                                       const std::shared_ptr<firebolt::rialto::common::ITimerFactory> &timerFactory,
                                       std::chrono::seconds healthcheckFrequency)
    : m_sessionServerAppManager{sessionServerAppManager}
{
    if (std::chrono::seconds{0} != healthcheckFrequency)
    {
        m_healthcheckTimer = timerFactory->createTimer(healthcheckFrequency,
                                                       std::bind(&HealthcheckService::sendPing, this),
                                                       firebolt::rialto::common::TimerType::PERIODIC);
    }
}

HealthcheckService::~HealthcheckService()
{
    if (m_healthcheckTimer && m_healthcheckTimer->isActive())
    {
        m_healthcheckTimer->cancel();
        m_healthcheckTimer.reset();
    }
}

void HealthcheckService::onPingSent(int serverId, int pingId) {}

void HealthcheckService::onAckReceived(int serverId, int pingId, bool success) {}

void HealthcheckService::onServerRemoved(int serverId) {}

void HealthcheckService::sendPing() {}
} // namespace rialto::servermanager::common
