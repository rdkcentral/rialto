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

#ifndef RIALTO_SERVERMANAGER_COMMON_HEALTHCHECK_SERVICE_H_
#define RIALTO_SERVERMANAGER_COMMON_HEALTHCHECK_SERVICE_H_

#include "IHealthcheckService.h"
#include "ISessionServerAppManager.h"
#include "ITimer.h"
#include <map>
#include <memory>
#include <mutex>
#include <set>

namespace rialto::servermanager::common
{
class HealthcheckService : public IHealthcheckService
{
public:
    HealthcheckService(ISessionServerAppManager &sessionServerAppManager,
                       const std::shared_ptr<firebolt::rialto::common::ITimerFactory> &timerFactory,
                       std::chrono::seconds healthcheckInterval, unsigned numOfFailedPingsBeforeRecovery);
    ~HealthcheckService() override;
    void onPingSent(int serverId, int pingId) override;
    void onAckReceived(int serverId, int pingId, bool success) override;
    void onServerRemoved(int serverId) override;

private:
    void sendPing();
    void handleError(int serverId);

private:
    ISessionServerAppManager &m_sessionServerAppManager;
    const unsigned m_kNumOfFailedPingsBeforeRecovery;
    std::unique_ptr<firebolt::rialto::common::ITimer> m_healthcheckTimer;
    std::mutex m_mutex;
    int m_currentPingId;
    std::set<int> m_remainingPings;
    std::map<int, unsigned> m_failedPings;
};
} // namespace rialto::servermanager::common

#endif // RIALTO_SERVERMANAGER_COMMON_HEALTHCHECK_SERVICE_H_
