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

namespace
{
constexpr int kInvalidId{-1};
} // namespace

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

void SessionServerAppManager::preloadSessionServers(int numOfPreloadedServers)
{
    for (int i = 0; i < numOfPreloadedServers; ++i)
    {
        addPreloadedSessionServer();
    }
}

bool SessionServerAppManager::initiateApplication(const std::string &appName,
                                                  const firebolt::rialto::common::SessionServerState &state,
                                                  const firebolt::rialto::common::AppConfig &appConfig)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager requests to launch %s with initial state: %s", appName.c_str(),
                                   toString(state));
    if (state != firebolt::rialto::common::SessionServerState::NOT_RUNNING && !isSessionServerLaunched(appName))
    {
        int preloadedServerId{getPreloadedServerId()};
        if (kInvalidId != preloadedServerId)
        {
            return configurePreloadedSessionServer(preloadedServerId, appName, state, appConfig);
        }
        return addSessionServer(appName, state, appConfig);
    }
    else if (state == firebolt::rialto::common::SessionServerState::NOT_RUNNING)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Initialization of %s failed - wrong state", appName.c_str());
    }
    else
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Initialization of %s failed. App is already launched", appName.c_str());
    }
    return false;
}

bool SessionServerAppManager::setSessionServerState(const std::string &appName,
                                                    const firebolt::rialto::common::SessionServerState &newState)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager requests to change state of %s to %s", appName.c_str(),
                                   toString(newState));
    if (isSessionServerLaunched(appName))
    {
        return changeSessionServerState(appName, newState);
    }
    RIALTO_SERVER_MANAGER_LOG_ERROR("Change state of %s to %s failed. App is not launched", appName.c_str(),
                                    toString(newState));
    return false;
}

void SessionServerAppManager::onSessionServerStateChanged(int serverId,
                                                          const firebolt::rialto::common::SessionServerState &newState)
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Queue state change of serverId: %d to %s", serverId, toString(newState));
    // Event loop needed here, as function caller may be deleted as a result of this call
    m_eventThread->add(&SessionServerAppManager::handleSessionServerStateChange, this, serverId, newState);
}

std::string SessionServerAppManager::getAppConnectionInfo(const std::string &appName) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                            [&](const auto &app) { return app->getAppName() == appName; });
    if (app != m_sessionServerApps.end())
    {
        return (*app)->getSessionManagementSocketName();
    }
    RIALTO_SERVER_MANAGER_LOG_ERROR("App: %s could not be found", appName.c_str());
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

bool SessionServerAppManager::addSessionServer(const std::string &appName,
                                               const firebolt::rialto::common::SessionServerState &initialState,
                                               const firebolt::rialto::common::AppConfig &appConfig)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager tries to launch %s", appName.c_str());
    int serverId{launchSessionServer(appName, initialState, appConfig)};
    if (kInvalidId == serverId)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("RialtoServerManager unable to launch %s", appName.c_str());
        return false;
    }
    if (!m_ipcController->createClient(serverId, getAppManagementSocketName(serverId)))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR(
            "Failed to establish RialtoServerManager - RialtoSessionServer connection for %s", appName.c_str());
        removeSessionServer(serverId, true);
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager %s successfully launched", appName.c_str());
    return true;
}

void SessionServerAppManager::addPreloadedSessionServer()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager tries to launch preloaded session server");
    int serverId{preloadSessionServer()};
    if (kInvalidId == serverId)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("RialtoServerManager unable to launch preloaded session server");
        return;
    }
    if (!m_ipcController->createClient(serverId, getAppManagementSocketName(serverId)))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to establish RialtoServerManager - RialtoSessionServer connection for "
                                        "session server with id: %d",
                                        serverId);
        removeSessionServer(serverId, true);
        return;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager with id %d successfully launched", serverId);
}

void SessionServerAppManager::removeSessionServer(int serverId, bool killApp)
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                            [&](const auto &app) { return app->getId() == serverId; });
    if (app != m_sessionServerApps.end())
    {
        if (killApp)
        {
            (*app)->kill();
        }
        m_sessionServerApps.erase(app);
    }
}

bool SessionServerAppManager::isSessionServerLaunched(const std::string &appName) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    return std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                        [&](const auto &app) { return app->getAppName() == appName; }) != m_sessionServerApps.end();
}

void SessionServerAppManager::cancelSessionServerStartupTimer(int serverId) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                            [&](const auto &app) { return app->getId() == serverId; });
    if (app != m_sessionServerApps.end())
    {
        (*app)->cancelStartupTimer();
    }
}

bool SessionServerAppManager::isSessionServerPreloaded(int serverId) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                            [&](const auto &app) { return app->getId() == serverId && app->isPreloaded(); });
    return app != m_sessionServerApps.end();
}

int SessionServerAppManager::launchSessionServer(const std::string &appName,
                                                 const firebolt::rialto::common::SessionServerState &initialState,
                                                 const firebolt::rialto::common::AppConfig &appConfig)
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = m_sessionServerAppFactory->create(appName, initialState, appConfig, *this);
    if (app->launch())
    {
        int serverId = app->getId();
        m_sessionServerApps.emplace_back(std::move(app));
        return serverId;
    }
    return kInvalidId;
}

int SessionServerAppManager::preloadSessionServer()
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = m_sessionServerAppFactory->create(*this);
    if (app->launch())
    {
        int serverId = app->getId();
        m_sessionServerApps.emplace_back(std::move(app));
        return serverId;
    }
    return kInvalidId;
}

bool SessionServerAppManager::configureSessionServer(int serverId)
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                            [&](const auto &app) { return app->getId() == serverId; });
    if (app == m_sessionServerApps.end())
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Configuration of app with id: %d failed - app not found", serverId);
        return false;
    }
    const auto initialState{(*app)->getInitialState()};
    const auto socketName{(*app)->getSessionManagementSocketName()};
    const firebolt::rialto::common::MaxResourceCapabilitites maxResource{(*app)->getMaxPlaybackSessions(),
                                                                         (*app)->getMaxWebAudioPlayers()};
    if (!m_ipcController->performSetConfiguration((*app)->getId(), initialState, socketName, maxResource))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Configuration of app with id %d failed - ipc error.", serverId);
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("Configuration of app with id %d succeeded.", serverId);
    return true;
}

bool SessionServerAppManager::configurePreloadedSessionServer(int serverId, const std::string &appName,
                                                              const firebolt::rialto::common::SessionServerState &state,
                                                              const firebolt::rialto::common::AppConfig &appConfig)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Configuration of preloaded app with id: %d for %s app", serverId, appName.c_str());
    if (addSessionServerConfiguration(serverId, appName, state, appConfig) && configureSessionServer(serverId))
    {
        // Schedule adding new preloaded session server (as we've just used one) and return immediately
        m_eventThread->add(&SessionServerAppManager::addPreloadedSessionServer, this);
        return true;
    }
    // Configuration failed, kill server and return error
    handleSessionServerStateChange(serverId, firebolt::rialto::common::SessionServerState::ERROR);
    // Schedule adding new preloaded session server
    m_eventThread->add(&SessionServerAppManager::addPreloadedSessionServer, this);
    return false;
}

bool SessionServerAppManager::addSessionServerConfiguration(int serverId, const std::string &appName,
                                                            const firebolt::rialto::common::SessionServerState &state,
                                                            const firebolt::rialto::common::AppConfig &appConfig)
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                            [&](const auto &app) { return app->getId() == serverId; });
    if (app == m_sessionServerApps.end())
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Configuration of app with id: %d failed - app not found", serverId);
        return false;
    }
    return (*app)->configure(appName, state, appConfig);
}

bool SessionServerAppManager::changeSessionServerState(const std::string &appName,
                                                       const firebolt::rialto::common::SessionServerState &newState)
{
    if (!m_ipcController->performSetState(getserverId(appName), newState))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Change state of %s to %s failed.", appName.c_str(), toString(newState));
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("Change state of %s to %s succeeded.", appName.c_str(), toString(newState));
    return true;
}

int SessionServerAppManager::getserverId(const std::string &appName) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                            [&](const auto &app) { return app->getAppName() == appName; });
    if (app != m_sessionServerApps.end())
    {
        return (*app)->getId();
    }
    return kInvalidId;
}

std::string SessionServerAppManager::getAppName(int serverId) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                            [&](const auto &app) { return app->getId() == serverId; });
    if (app != m_sessionServerApps.end())
    {
        return (*app)->getAppName();
    }
    return "";
}

int SessionServerAppManager::getAppManagementSocketName(int serverId) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                            [&](const auto &app) { return app->getId() == serverId; });
    if (app != m_sessionServerApps.end())
    {
        return (*app)->getAppManagementSocketName();
    }
    return -1;
}

void SessionServerAppManager::handleSessionServerStateChange(int serverId,
                                                             firebolt::rialto::common::SessionServerState newState)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("SessionServer with id: %d changed state to %s", serverId, toString(newState));
    std::string appName{getAppName(serverId)};
    if (firebolt::rialto::common::SessionServerState::UNINITIALIZED == newState)
    {
        cancelSessionServerStartupTimer(serverId);
        if (!isSessionServerPreloaded(serverId) && !configureSessionServer(serverId))
        {
            return handleSessionServerStateChange(serverId, firebolt::rialto::common::SessionServerState::ERROR);
        }
    }
    else if (newState == firebolt::rialto::common::SessionServerState::ERROR ||
             newState == firebolt::rialto::common::SessionServerState::NOT_RUNNING)
    {
        m_ipcController->removeClient(serverId);
        removeSessionServer(serverId, newState == firebolt::rialto::common::SessionServerState::ERROR);
    }
    if (!appName.empty() && m_stateObserver) // empty app name is when SessionServer is preloaded
    {
        m_stateObserver->stateChanged(appName, newState);
    }
}

void SessionServerAppManager::shutdownAllSessionServers()
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    for (const auto &sessionServer : m_sessionServerApps)
    {
        sessionServer->kill();
    }
    m_sessionServerApps.clear();
}

int SessionServerAppManager::getPreloadedServerId() const
{
    std::unique_lock<std::mutex> lock{m_sessionServerAppsMutex};
    auto app = std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                            [](const auto &app) { return app->isPreloaded() && app->isConnected(); });
    if (app != m_sessionServerApps.end())
    {
        return (*app)->getId();
    }
    return kInvalidId;
}
} // namespace rialto::servermanager::common
