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

#include "SessionServerAppManager.h"
#include "IController.h"
#include "IEventThread.h"
#include "IStateObserver.h"
#include "RialtoServerManagerLogging.h"
#include "Utils.h"
#include <algorithm>
#include <string>
#include <unistd.h>
#include <utility>

namespace rialto::servermanager::common
{
SessionServerAppManager::SessionServerAppManager(
    std::unique_ptr<ipc::IController> &ipcController, const std::shared_ptr<service::IStateObserver> &stateObserver,
    std::unique_ptr<ISessionServerAppFactory> &&sessionServerAppFactory,
    const std::shared_ptr<firebolt::rialto::common::IEventThreadFactory> &eventThreadFactory)
    : m_ipcController{ipcController}, m_eventThread{eventThreadFactory->createEventThread(
                                          "rialtoservermanager-appmanager")},
      m_sessionServerAppFactory{std::move(sessionServerAppFactory)}, m_stateObserver{stateObserver}
{
}

SessionServerAppManager::~SessionServerAppManager()
{
    shutdownAllSessionServers();
    m_eventThread.reset();
}

bool SessionServerAppManager::initiateApplication(const std::string &appId,
                                                  const firebolt::rialto::common::SessionServerState &state,
                                                  const firebolt::rialto::common::AppConfig &appConfig)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager requests to launch %s with initial state: %s", appId.c_str(),
                                   toString(state));
    if (state != firebolt::rialto::common::SessionServerState::NOT_RUNNING && !isSessionServerLaunched(appId))
    {
        return addSessionServer(appId, state);
    }
    RIALTO_SERVER_MANAGER_LOG_ERROR("Initialization of %s failed.", appId.c_str());
    return false;
}

bool SessionServerAppManager::setSessionServerState(const std::string &appId,
                                                    const firebolt::rialto::common::SessionServerState &newState)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager requests to change state of %s to %s", appId.c_str(),
                                   toString(newState));
    if (isSessionServerLaunched(appId))
    {
        return changeSessionServerState(appId, newState);
    }
    RIALTO_SERVER_MANAGER_LOG_ERROR("Change state of %s to %s failed. App is not launched", appId.c_str(),
                                    toString(newState));
    return false;
}

void SessionServerAppManager::onSessionServerStateChanged(const std::string &appId,
                                                          const firebolt::rialto::common::SessionServerState &newState)
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Queue state change of %s to %s", appId.c_str(), toString(newState));
    // Event loop needed here, as function caller may be deleted as a result of this call
    m_eventThread->add(&SessionServerAppManager::handleSessionServerStateChange, this, appId, newState);
}

std::string SessionServerAppManager::getAppConnectionInfo(const std::string &appId) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = m_sessionServerApps.find(appId);
    if (app != m_sessionServerApps.end())
    {
        return app->second->getSessionManagementSocketName();
    }
    RIALTO_SERVER_MANAGER_LOG_ERROR("%s: App: %s could not be found", __func__, appId.c_str());
    return "";
}

bool SessionServerAppManager::setLogLevels(const service::LoggingLevels &logLevels) const
{
    setLocalLogLevels(logLevels);
    if (!m_ipcController->setLogLevels(logLevels))
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("Change log levels failed.");
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("Change log levels succeeded.");
    return true;
}

bool SessionServerAppManager::addSessionServer(const std::string &appId,
                                               const firebolt::rialto::common::SessionServerState &initialState)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager tries to launch %s", appId.c_str());
    if (!launchSessionServer(appId, initialState))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("RialtoServerManager unable to launch %s", appId.c_str());
        return false;
    }
    if (!m_ipcController->createClient(appId, getAppManagementSocketName(appId)))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR(
            "Failed to establish RialtoServerManager - RialtoSessionServer connection for %s", appId.c_str());
        removeSessionServer(appId, true);
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager %s successfully launched", appId.c_str());
    return true;
}

void SessionServerAppManager::removeSessionServer(const std::string &appId, bool killApp)
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = m_sessionServerApps.find(appId);
    if (app != m_sessionServerApps.end())
    {
        if (killApp)
        {
            app->second->kill();
        }
        m_sessionServerApps.erase(app);
    }
}

bool SessionServerAppManager::isSessionServerLaunched(const std::string &appId) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    return m_sessionServerApps.find(appId) != m_sessionServerApps.end();
}

void SessionServerAppManager::cancelSessionServerStartupTimer(const std::string &appId) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = m_sessionServerApps.find(appId);
    if (app != m_sessionServerApps.end())
    {
        app->second->cancelStartupTimer();
    }
}

bool SessionServerAppManager::launchSessionServer(const std::string &appId,
                                                  const firebolt::rialto::common::SessionServerState &initialState)
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = m_sessionServerAppFactory->create(appId, initialState, *this);
    if (app->launch())
    {
        m_sessionServerApps.insert(std::make_pair(appId, std::move(app)));
        return true;
    }
    return false;
}

bool SessionServerAppManager::configureSessionServer(const std::string &appId)
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = m_sessionServerApps.find(appId);
    if (app == m_sessionServerApps.end())
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Configuration of %s failed - app not found", appId.c_str());
        return false;
    }
    const auto initialState{app->second->getInitialState()};
    const auto socketName{app->second->getSessionManagementSocketName()};
    const firebolt::rialto::common::MaxResourceCapabilitites maxResource{app->second->getMaxPlaybackSessions(),
                                                                         app->second->getMaxWebAudioPlayers()};
    if (!m_ipcController->performSetConfiguration(appId, initialState, socketName, maxResource))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Configuration of %s failed - ipc error.", appId.c_str());
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("Configuration of %s succeeded.", appId.c_str());
    return true;
}

bool SessionServerAppManager::changeSessionServerState(const std::string &appId,
                                                       const firebolt::rialto::common::SessionServerState &newState)
{
    if (!m_ipcController->performSetState(appId, newState))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Change state of %s to %s failed.", appId.c_str(), toString(newState));
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("Change state of %s to %s succeeded.", appId.c_str(), toString(newState));
    return true;
}

int SessionServerAppManager::getAppManagementSocketName(const std::string &appId) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = m_sessionServerApps.find(appId);
    if (app != m_sessionServerApps.end())
    {
        return app->second->getAppManagementSocketName();
    }
    return -1;
}

void SessionServerAppManager::handleSessionServerStateChange(std::string appId,
                                                             firebolt::rialto::common::SessionServerState newState)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("%s changed state to %s", appId.c_str(), toString(newState));
    if (firebolt::rialto::common::SessionServerState::UNINITIALIZED == newState)
    {
        cancelSessionServerStartupTimer(appId);
        if (!configureSessionServer(appId))
        {
            return handleSessionServerStateChange(appId, firebolt::rialto::common::SessionServerState::ERROR);
        }
    }
    else if (newState == firebolt::rialto::common::SessionServerState::ERROR ||
             newState == firebolt::rialto::common::SessionServerState::NOT_RUNNING)
    {
        m_ipcController->removeClient(appId);
        removeSessionServer(appId);
    }
    if (m_stateObserver)
    {
        m_stateObserver->stateChanged(appId, newState);
    }
}

void SessionServerAppManager::shutdownAllSessionServers()
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    for (const auto &sessionServer : m_sessionServerApps)
    {
        sessionServer.second->kill();
    }
    m_sessionServerApps.clear();
}
} // namespace rialto::servermanager::common
