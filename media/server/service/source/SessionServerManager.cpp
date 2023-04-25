/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "SessionServerManager.h"
#include "IApplicationManagementServer.h"
#include "IIpcFactory.h"
#include "ISessionManagementServer.h"
#include "RialtoServerLogging.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
inline bool isNumber(const std::string &text)
{
    return std::find_if(text.begin(), text.end(), [](const auto &letter) { return !std::isdigit(letter); }) == text.end();
}
} // namespace

namespace firebolt::rialto::server::service
{
SessionServerManager::SessionServerManager(const ipc::IIpcFactory &ipcFactory, IPlaybackService &playbackService,
                                           ICdmService &cdmService)
    : m_playbackService{playbackService}, m_cdmService{cdmService},
      m_applicationManagementServer{ipcFactory.createApplicationManagementServer(*this)},
      m_sessionManagementServer{ipcFactory.createSessionManagementServer(playbackService, cdmService)},
      m_isServiceRunning{true}, m_currentState{common::SessionServerState::UNINITIALIZED}
{
    RIALTO_SERVER_LOG_INFO("Starting Rialto Server Service");
}

SessionServerManager::~SessionServerManager()
{
    RIALTO_SERVER_LOG_INFO("Stopping Rialto Server Service");
    std::unique_lock<std::mutex> lock{m_serviceMutex};
    m_isServiceRunning = false;
    m_serviceCv.notify_one();
}

bool SessionServerManager::initialize(int argc, char *argv[])
try
{
    if (argc != 2)
    {
        RIALTO_SERVER_LOG_ERROR("Wrong number of arguments. Rialto Server Service will close now.");
        return false;
    }
    std::string socketStr{argv[1]};
    if (!isNumber(socketStr))
    {
        RIALTO_SERVER_LOG_ERROR("Rialto App Management socket is not a number.");
        return false;
    }
    if (!m_applicationManagementServer->initialize(std::stoi(socketStr)))
    {
        RIALTO_SERVER_LOG_ERROR("Initialization of Application Management server failed.");
        return false;
    }
    m_applicationManagementServer->start();
    return m_applicationManagementServer->sendStateChangedEvent(common::SessionServerState::UNINITIALIZED);
}
catch (const std::exception &e)
{
    RIALTO_SERVER_LOG_ERROR("Exception caught during service initialization: %s", e.what());
    return false;
}

void SessionServerManager::startService()
{
    std::unique_lock<std::mutex> lock{m_serviceMutex};
    m_serviceCv.wait(lock, [this]() { return !m_isServiceRunning; });
}

void SessionServerManager::stopService()
{
    std::unique_lock<std::mutex> lock{m_serviceMutex};
    m_isServiceRunning = false;
    m_serviceCv.notify_one();
}

bool SessionServerManager::setConfiguration(const std::string &socketName, const common::SessionServerState &state,
                                            const common::MaxResourceCapabilitites &maxResource)
{
    if (!m_sessionManagementServer->initialize(socketName))
    {
        RIALTO_SERVER_LOG_ERROR("SetConfiguration failed - SessionManagementServer failed to initialize");
        return false;
    }
    m_sessionManagementServer->start();
    m_playbackService.setMaxPlaybacks(maxResource.maxPlaybacks);
    m_playbackService.setMaxWebAudioPlayers(maxResource.maxWebAudioPlayers);
    return setState(state);
}

bool SessionServerManager::setState(const common::SessionServerState &state)
{
    switch (state)
    {
    case common::SessionServerState::ACTIVE:
    {
        return switchToActive();
    }
    case common::SessionServerState::INACTIVE:
    {
        return switchToInactive();
    }
    case common::SessionServerState::NOT_RUNNING:
    {
        return switchToNotRunning();
    }
    default:
    {
        RIALTO_SERVER_LOG_ERROR("SetState failed - unsupported state");
        m_applicationManagementServer->sendStateChangedEvent(common::SessionServerState::ERROR);
    }
    }
    return false;
}

void SessionServerManager::setLogLevels(RIALTO_DEBUG_LEVEL defaultLogLevels, RIALTO_DEBUG_LEVEL clientLogLevels,
                                        RIALTO_DEBUG_LEVEL sessionServerLogLevels, RIALTO_DEBUG_LEVEL ipcLogLevels,
                                        RIALTO_DEBUG_LEVEL serverManagerLogLevels, RIALTO_DEBUG_LEVEL commonLogLevels)
{
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_DEFAULT, defaultLogLevels);
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_CLIENT, clientLogLevels);
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_SERVER, sessionServerLogLevels);
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_IPC, ipcLogLevels);
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_SERVER_MANAGER, serverManagerLogLevels);
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_COMMON, commonLogLevels);
    m_sessionManagementServer->setLogLevels(defaultLogLevels, clientLogLevels, ipcLogLevels, commonLogLevels);
}

bool SessionServerManager::switchToActive()
{
    if (m_currentState.load() == common::SessionServerState::ACTIVE)
    {
        RIALTO_SERVER_LOG_DEBUG("Session server already in Active state.");
        return true;
    }
    if (!m_playbackService.switchToActive())
    {
        RIALTO_SERVER_LOG_ERROR("Player service failed to switch to active state");
        return false;
    }
    if (!m_cdmService.switchToActive())
    {
        RIALTO_SERVER_LOG_ERROR("Cdm service failed to switch to active state");
        m_playbackService.switchToInactive();
        return false;
    }
    if (m_applicationManagementServer->sendStateChangedEvent(common::SessionServerState::ACTIVE))
    {
        m_currentState.store(common::SessionServerState::ACTIVE);
        return true;
    }
    m_playbackService.switchToInactive();
    m_cdmService.switchToInactive();
    return false;
}

bool SessionServerManager::switchToInactive()
{
    if (m_currentState.load() == common::SessionServerState::INACTIVE)
    {
        RIALTO_SERVER_LOG_DEBUG("Session server already in Inactive state.");
        return true;
    }
    m_playbackService.switchToInactive();
    m_cdmService.switchToInactive();
    if (m_applicationManagementServer->sendStateChangedEvent(common::SessionServerState::INACTIVE))
    {
        m_currentState.store(common::SessionServerState::INACTIVE);
        return true;
    }
    if (m_currentState.load() == common::SessionServerState::ACTIVE)
    {
        if (!m_playbackService.switchToActive())
        {
            RIALTO_SERVER_LOG_WARN("Player service failed to switch to active state");
        }
        if (!m_cdmService.switchToActive())
        {
            RIALTO_SERVER_LOG_WARN("Cdm service failed to switch to active state");
        }
    }
    return false;
}

bool SessionServerManager::switchToNotRunning()
{
    if (m_currentState.load() == common::SessionServerState::NOT_RUNNING)
    {
        RIALTO_SERVER_LOG_DEBUG("Session server already in NotRunning state.");
        return true;
    }
    // Free resources before sending notification to ServerManager
    m_playbackService.switchToInactive();
    m_cdmService.switchToInactive();
    stopService();
    if (m_applicationManagementServer->sendStateChangedEvent(common::SessionServerState::NOT_RUNNING))
    {
        m_currentState.store(common::SessionServerState::NOT_RUNNING);
        return true;
    }
    return false;
}
} // namespace firebolt::rialto::server::service
