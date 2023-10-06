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
#include "RialtoServerManagerLogging.h"

namespace
{
int generatePingId()
{
    static int id{0};
    return id++;
}
} // namespace

namespace rialto::servermanager::common
{
HealthcheckService::HealthcheckService(ISessionServerAppManager &sessionServerAppManager,
                                       const std::shared_ptr<firebolt::rialto::common::ITimerFactory> &timerFactory,
                                       std::chrono::seconds healthcheckInterval, unsigned numOfFailedPingsBeforeRecovery)
    : m_sessionServerAppManager{sessionServerAppManager},
      m_kNumOfFailedPingsBeforeRecovery{numOfFailedPingsBeforeRecovery}, m_currentPingId{-1}
{
    if (std::chrono::seconds{0} != healthcheckInterval)
    {
        m_healthcheckTimer = timerFactory->createTimer(healthcheckInterval,
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

void HealthcheckService::onPingSent(int serverId, int pingId)
{
    std::unique_lock<std::mutex> lock{m_mutex};
    if (pingId != m_currentPingId)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Something went seriously wrong. Ping sent with wrong id to server: %d, valid "
                                        "ping id: %d, sent pingId: %d",
                                        serverId, m_currentPingId, pingId);
        return;
    }
    m_remainingPings.insert(serverId);
    m_failedPings.try_emplace(serverId, 0);
}

void HealthcheckService::onPingFailed(int serverId, int pingId)
{
    std::unique_lock<std::mutex> lock{m_mutex};
    if (pingId != m_currentPingId)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Something went seriously wrong. Ping sent with wrong id to server: %d, valid "
                                        "ping id: %d, sent pingId: %d",
                                        serverId, m_currentPingId, pingId);
        return;
    }
    if (m_failedPings.end() != m_failedPings.find(serverId))
    {
        handleError(serverId);
    }
    else
    {
        m_sessionServerAppManager.onSessionServerStateChanged(serverId,
                                                              firebolt::rialto::common::SessionServerState::ERROR);
        m_failedPings.emplace(serverId, 1);
    }
}

void HealthcheckService::onAckReceived(int serverId, int pingId, bool success)
{
    std::unique_lock<std::mutex> lock{m_mutex};
    if (pingId != m_currentPingId)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("Unexpected ack received from server id: %d. Current ping id: %d, received ping "
                                       "id: %d",
                                       serverId, m_currentPingId, pingId);
        return;
    }
    m_remainingPings.erase(serverId);
    if (success)
    {
        m_failedPings[serverId] = 0;
    }
    else
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("Ack with error received from server id: %d, ping id: %d", serverId, pingId);
        handleError(serverId);
    }
}

void HealthcheckService::onServerRemoved(int serverId)
{
    std::unique_lock<std::mutex> lock{m_mutex};
    m_remainingPings.erase(serverId);
    m_failedPings.erase(serverId);
}

void HealthcheckService::sendPing()
{
    std::unique_lock<std::mutex> lock{m_mutex};
    for (int serverId : m_remainingPings)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("Ping (id: %d) timeout for server id: %d", m_currentPingId, serverId);
        handleError(serverId);
    }
    m_remainingPings.clear();
    m_currentPingId = generatePingId();
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Start ping procedure with id: %d", m_currentPingId);
    m_sessionServerAppManager.sendPingEvents(m_currentPingId);
}

void HealthcheckService::handleError(int serverId)
{
    m_sessionServerAppManager.onSessionServerStateChanged(serverId, firebolt::rialto::common::SessionServerState::ERROR);
    unsigned &failedPingsNum{m_failedPings[serverId]};
    if (++failedPingsNum >= m_kNumOfFailedPingsBeforeRecovery)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN(
            "Max num of failed pings reached for server with id: %d. Starting recovery action", serverId);
        failedPingsNum = 0;
        m_sessionServerAppManager.restartServer(serverId);
    }
}
} // namespace rialto::servermanager::common
