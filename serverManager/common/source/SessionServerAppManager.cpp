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
#include "RialtoServerManagerLogging.h"
#include "Utils.h"
#include <algorithm>
#include <future>
#include <utility>

namespace
{
const std::unique_ptr<rialto::servermanager::common::ISessionServerApp> kInvalidSessionServer;
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
    m_eventThread->add(&SessionServerAppManager::shutdownAllSessionServers, this);
    m_eventThread->flush();
    m_eventThread.reset();
}

void SessionServerAppManager::preloadSessionServers(int numOfPreloadedServers)
{
    m_eventThread->add(
        [this, numOfPreloadedServers]()
        {
            for (int i = 0; i < numOfPreloadedServers; ++i)
            {
                connectSessionServer(preloadSessionServer());
            }
        });
}

bool SessionServerAppManager::initiateApplication(const std::string &appName,
                                                  const firebolt::rialto::common::SessionServerState &state,
                                                  const firebolt::rialto::common::AppConfig &appConfig)
{
    std::promise<bool> p;
    std::future<bool> f{p.get_future()};
    m_eventThread->add(
        [&]()
        {
            RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager requests to launch %s with initial state: %s",
                                           appName.c_str(), toString(state));
            if (state != firebolt::rialto::common::SessionServerState::NOT_RUNNING && !getServerByAppName(appName))
            {
                auto &preloadedServer{getPreloadedServer()};
                if (preloadedServer)
                {
                    return p.set_value(configurePreloadedSessionServer(preloadedServer, appName, state, appConfig));
                }
                return p.set_value(connectSessionServer(launchSessionServer(appName, state, appConfig)));
            }
            else if (state == firebolt::rialto::common::SessionServerState::NOT_RUNNING)
            {
                RIALTO_SERVER_MANAGER_LOG_ERROR("Initialization of %s failed - wrong state", appName.c_str());
            }
            else
            {
                RIALTO_SERVER_MANAGER_LOG_ERROR("Initialization of %s failed. App is already launched", appName.c_str());
            }
            return p.set_value(false);
        });
    return f.get();
}

bool SessionServerAppManager::setSessionServerState(const std::string &appName,
                                                    const firebolt::rialto::common::SessionServerState &newState)
{
    std::promise<bool> p;
    std::future<bool> f{p.get_future()};
    m_eventThread->add([&]() { return p.set_value(changeSessionServerState(appName, newState)); });
    return f.get();
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
    std::promise<std::string> p;
    std::future<std::string> f{p.get_future()};
    m_eventThread->add(
        [&]()
        {
            const auto &sessionServer{getServerByAppName(appName)};
            if (sessionServer)
            {
                return p.set_value(sessionServer->getSessionManagementSocketName());
            }
            RIALTO_SERVER_MANAGER_LOG_ERROR("App: %s could not be found", appName.c_str());
            return p.set_value("");
        });
    return f.get();
}

bool SessionServerAppManager::setLogLevels(const service::LoggingLevels &logLevels) const
{
    std::promise<bool> p;
    std::future<bool> f{p.get_future()};
    m_eventThread->add(
        [&]()
        {
            setLocalLogLevels(logLevels);
            if (!m_ipcController->setLogLevels(logLevels))
            {
                RIALTO_SERVER_MANAGER_LOG_WARN("Change log levels failed.");
                return p.set_value(false);
            }
            RIALTO_SERVER_MANAGER_LOG_INFO("Change log levels succeeded.");
            return p.set_value(true);
        });
    return f.get();
}

bool SessionServerAppManager::connectSessionServer(const std::unique_ptr<ISessionServerApp> &sessionServer)
{
    if (!sessionServer)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Unable to connect Session Server - pointer is null!");
        return false;
    }
    if (!m_ipcController->createClient(sessionServer->getServerId(), sessionServer->getAppManagementSocketName()))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to establish RialtoServerManager - RialtoSessionServer connection for "
                                        "session server with id: %d",
                                        sessionServer->getServerId());
        sessionServer->kill();
        m_sessionServerApps.erase(sessionServer);
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager with id %d successfully connected", sessionServer->getServerId());
    return true;
}

bool SessionServerAppManager::configureSessionServer(const std::unique_ptr<ISessionServerApp> &sessionServer)
{
    if (!sessionServer)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Configuration of app with id: %d failed - app not found",
                                        sessionServer->getServerId());
        return false;
    }
    const auto initialState{sessionServer->getInitialState()};
    const auto socketName{sessionServer->getSessionManagementSocketName()};
    const firebolt::rialto::common::MaxResourceCapabilitites maxResource{sessionServer->getMaxPlaybackSessions(),
                                                                         sessionServer->getMaxWebAudioPlayers()};
    if (!m_ipcController->performSetConfiguration(sessionServer->getServerId(), initialState, socketName, maxResource))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Configuration of app with id %d failed - ipc error.",
                                        sessionServer->getServerId());
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("Configuration of app with id %d succeeded.", sessionServer->getServerId());
    return true;
}

bool SessionServerAppManager::configurePreloadedSessionServer(const std::unique_ptr<ISessionServerApp> &sessionServer,
                                                              const std::string &appName,
                                                              const firebolt::rialto::common::SessionServerState &state,
                                                              const firebolt::rialto::common::AppConfig &appConfig)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Configuration of preloaded session server with id: %d for %s app",
                                   sessionServer->getServerId(), appName.c_str());
    if (sessionServer->configure(appName, state, appConfig) && configureSessionServer(sessionServer))
    {
        // Schedule adding new preloaded session server (as we've just used one) and return immediately
        m_eventThread->add([this]() { connectSessionServer(preloadSessionServer()); });
        return true;
    }
    // Configuration failed, kill server and return error
    handleSessionServerStateChange(sessionServer->getServerId(), firebolt::rialto::common::SessionServerState::ERROR);
    // Schedule adding new preloaded session server
    m_eventThread->add([this]() { connectSessionServer(preloadSessionServer()); });
    return false;
}

bool SessionServerAppManager::changeSessionServerState(const std::string &appName,
                                                       const firebolt::rialto::common::SessionServerState &newState)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager requests to change state of %s to %s", appName.c_str(),
                                   toString(newState));
    const auto &sessionServer{getServerByAppName(appName)};
    if (!sessionServer)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Change state of %s to %s failed - session server not found.", appName.c_str(),
                                        toString(newState));
        return false;
    }
    if (!m_ipcController->performSetState(sessionServer->getServerId(), newState))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Change state of %s to %s failed.", appName.c_str(), toString(newState));
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("Change state of %s to %s succeeded.", appName.c_str(), toString(newState));
    return true;
}

void SessionServerAppManager::handleSessionServerStateChange(int serverId,
                                                             firebolt::rialto::common::SessionServerState newState)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("SessionServer with id: %d changed state to %s", serverId, toString(newState));
    const auto &sessionServer{getServerById(serverId)};
    std::string appName{sessionServer->getAppName()};
    if (firebolt::rialto::common::SessionServerState::UNINITIALIZED == newState)
    {
        sessionServer->cancelStartupTimer();
        if (!sessionServer->isPreloaded() && !configureSessionServer(sessionServer))
        {
            return handleSessionServerStateChange(serverId, firebolt::rialto::common::SessionServerState::ERROR);
        }
    }
    else if (newState == firebolt::rialto::common::SessionServerState::ERROR ||
             newState == firebolt::rialto::common::SessionServerState::NOT_RUNNING)
    {
        m_ipcController->removeClient(serverId);
        if (newState == firebolt::rialto::common::SessionServerState::ERROR)
        {
            sessionServer->kill();
        }
        m_sessionServerApps.erase(sessionServer);
    }
    if (!appName.empty() && m_stateObserver) // empty app name is when SessionServer is preloaded
    {
        m_stateObserver->stateChanged(appName, newState);
    }
}

void SessionServerAppManager::shutdownAllSessionServers()
{
    for (const auto &sessionServer : m_sessionServerApps)
    {
        sessionServer->kill();
    }
    m_sessionServerApps.clear();
}

const std::unique_ptr<ISessionServerApp> &
SessionServerAppManager::launchSessionServer(const std::string &appName,
                                             const firebolt::rialto::common::SessionServerState &initialState,
                                             const firebolt::rialto::common::AppConfig &appConfig)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Launching Rialto Session Server for %s", appName.c_str());
    auto app = m_sessionServerAppFactory->create(appName, initialState, appConfig, *this);
    if (app->launch())
    {
        auto result = m_sessionServerApps.emplace(std::move(app));
        if (result.second)
        {
            return *result.first;
        }
    }
    return kInvalidSessionServer;
}

const std::unique_ptr<ISessionServerApp> &SessionServerAppManager::preloadSessionServer()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Preloading new Rialto Session Server");
    auto app = m_sessionServerAppFactory->create(*this);
    if (app->launch())
    {
        auto result = m_sessionServerApps.emplace(std::move(app));
        if (result.second)
        {
            return *result.first;
        }
    }
    return kInvalidSessionServer;
}

const std::unique_ptr<ISessionServerApp> &SessionServerAppManager::getPreloadedServer() const
{
    auto iter = std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                             [](const auto &srv) { return srv->isPreloaded() && srv->isConnected(); });
    if (m_sessionServerApps.end() != iter)
    {
        return *iter;
    }
    return kInvalidSessionServer;
}

const std::unique_ptr<ISessionServerApp> &SessionServerAppManager::getServerByAppName(const std::string &appName) const
{
    auto iter{std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                           [&](const auto &srv) { return srv->getAppName() == appName; })};
    if (m_sessionServerApps.end() != iter)
    {
        return *iter;
    }
    return kInvalidSessionServer;
}

const std::unique_ptr<ISessionServerApp> &SessionServerAppManager::getServerById(int serverId) const
{
    auto iter{std::find_if(m_sessionServerApps.begin(), m_sessionServerApps.end(),
                           [&](const auto &srv) { return srv->getServerId() == serverId; })};
    if (m_sessionServerApps.end() != iter)
    {
        return *iter;
    }
    return kInvalidSessionServer;
}
} // namespace rialto::servermanager::common
