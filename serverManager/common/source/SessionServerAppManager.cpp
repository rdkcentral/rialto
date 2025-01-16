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
    std::unique_ptr<IHealthcheckServiceFactory> &&healthcheckServiceFactory,
    const std::shared_ptr<firebolt::rialto::common::IEventThreadFactory> &eventThreadFactory)
    : m_ipcController{ipcController}, m_eventThread{eventThreadFactory->createEventThread(
                                          "rialtoservermanager-appmanager")},
      m_sessionServerAppFactory{std::move(sessionServerAppFactory)}, m_stateObserver{stateObserver},
      m_healthcheckService{healthcheckServiceFactory->createHealthcheckService(*this)}
{
}

SessionServerAppManager::~SessionServerAppManager()
{
    m_eventThread->add(&SessionServerAppManager::shutdownAllSessionServers, this);
    m_eventThread->flush();
    m_eventThread.reset();
}

void SessionServerAppManager::preloadSessionServers(unsigned numOfPreloadedServers)
{
    m_eventThread->add(
        [this, numOfPreloadedServers]()
        {
            for (unsigned i = 0; i < numOfPreloadedServers; ++i)
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
    m_eventThread->add([&]() { return p.set_value(handleInitiateApplication(appName, state, appConfig)); });
    return f.get();
}

bool SessionServerAppManager::handleInitiateApplication(const std::string &appName,
                                                        const firebolt::rialto::common::SessionServerState &state,
                                                        const firebolt::rialto::common::AppConfig &appConfig)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager requests to launch %s with initial state: %s", appName.c_str(),
                                   toString(state));
    if (state != firebolt::rialto::common::SessionServerState::NOT_RUNNING && !getServerByAppName(appName))
    {
        auto &preloadedServer{getPreloadedServer()};
        if (preloadedServer)
        {
            return configurePreloadedSessionServer(preloadedServer, appName, state, appConfig);
        }
        return connectSessionServer(launchSessionServer(appName, state, appConfig));
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

void SessionServerAppManager::sendPingEvents(int pingId)
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Queue ping procedure with id: %d", pingId);
    m_eventThread->add(
        [this, pingId]()
        {
            for (const auto &sessionServer : m_sessionServerApps)
            {
                auto serverId{sessionServer->getServerId()};
                if (!m_ipcController->performPing(serverId, pingId))
                {
                    RIALTO_SERVER_MANAGER_LOG_ERROR("Ping with id: %d failed for server: %d", pingId, serverId);
                    m_healthcheckService->onPingFailed(serverId, pingId);
                    continue;
                }
                m_healthcheckService->onPingSent(serverId, pingId);
            }
        });
}

void SessionServerAppManager::onAck(int serverId, int pingId, bool success)
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Queue ack handling for serverId: %d ping id: %d", serverId, pingId);
    m_eventThread->add(&SessionServerAppManager::handleAck, this, serverId, pingId, success);
}

std::string SessionServerAppManager::getAppConnectionInfo(const std::string &appName) const
{
    std::promise<std::string> p;
    std::future<std::string> f{p.get_future()};
    m_eventThread->add(
        [&]()
        {
            const auto &kSessionServer{getServerByAppName(appName)};
            if (kSessionServer)
            {
                return p.set_value(kSessionServer->getSessionManagementSocketName());
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

void SessionServerAppManager::restartServer(int serverId)
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Queue restart server handling for serverId: %d", serverId);
    m_eventThread->add(&SessionServerAppManager::handleRestartServer, this, serverId);
}

void SessionServerAppManager::handleRestartServer(int serverId)
{
    const auto &kSessionServer{getServerById(serverId)};
    if (!kSessionServer)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("Unable to restart server, serverId: %d", serverId);
        return;
    }
    // First, get all needed information from current app
    const std::string kAppName{kSessionServer->getAppName()};
    const firebolt::rialto::common::SessionServerState kState{kSessionServer->getExpectedState()};
    const firebolt::rialto::common::AppConfig kAppConfig{kSessionServer->getSessionManagementSocketName(),
                                                         kSessionServer->getClientDisplayName()};
    if (firebolt::rialto::common::SessionServerState::INACTIVE != kState &&
        firebolt::rialto::common::SessionServerState::ACTIVE != kState)
    {
        RIALTO_SERVER_MANAGER_LOG_DEBUG("Restart server to %s not needed for serverId: %d", toString(kState), serverId);
        return;
    }
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Restarting server with id: %d", serverId);
    // Then kill the app
    kSessionServer->kill();
    handleSessionServerStateChange(serverId, firebolt::rialto::common::SessionServerState::NOT_RUNNING);

    // Finally, spawn the new app with old settings
    handleInitiateApplication(kAppName, kState, kAppConfig);
}

bool SessionServerAppManager::connectSessionServer(const std::unique_ptr<ISessionServerApp> &kSessionServer)
{
    if (!kSessionServer)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Unable to connect Session Server - pointer is null!");
        return false;
    }
    if (!m_ipcController->createClient(kSessionServer->getServerId(), kSessionServer->getAppManagementSocketName()))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to establish RialtoServerManager - RialtoSessionServer connection for "
                                        "session server with id: %d",
                                        kSessionServer->getServerId());
        kSessionServer->kill();
        m_healthcheckService->onServerRemoved(kSessionServer->getServerId());
        m_sessionServerApps.erase(kSessionServer);
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager with id %d successfully connected",
                                   kSessionServer->getServerId());
    return true;
}

bool SessionServerAppManager::configureSessionServer(const std::unique_ptr<ISessionServerApp> &kSessionServer)
{
    if (!kSessionServer)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Unable to configure Session Server - pointer is null!");
        return false;
    }
    if (kSessionServer->isNamedSocketInitialized())
    {
        return configureSessionServerWithSocketFd(kSessionServer);
    }
    return configureSessionServerWithSocketName(kSessionServer);
}

bool SessionServerAppManager::configurePreloadedSessionServer(const std::unique_ptr<ISessionServerApp> &kSessionServer,
                                                              const std::string &appName,
                                                              const firebolt::rialto::common::SessionServerState &state,
                                                              const firebolt::rialto::common::AppConfig &appConfig)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Configuration of preloaded session server with id: %d for %s app",
                                   kSessionServer->getServerId(), appName.c_str());
    if (kSessionServer->configure(appName, state, appConfig) && configureSessionServer(kSessionServer))
    {
        // Schedule adding new preloaded session server (as we've just used one) and return immediately
        m_eventThread->add([this]() { connectSessionServer(preloadSessionServer()); });
        return true;
    }
    // Configuration failed, kill server and return error
    handleSessionServerStateChange(kSessionServer->getServerId(), firebolt::rialto::common::SessionServerState::ERROR);
    kSessionServer->kill();
    handleSessionServerStateChange(kSessionServer->getServerId(),
                                   firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    // Schedule adding new preloaded session server
    m_eventThread->add([this]() { connectSessionServer(preloadSessionServer()); });
    return false;
}

bool SessionServerAppManager::changeSessionServerState(const std::string &appName,
                                                       const firebolt::rialto::common::SessionServerState &newState)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager requests to change state of %s to %s", appName.c_str(),
                                   toString(newState));
    const auto &kSessionServer{getServerByAppName(appName)};
    if (!kSessionServer)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Change state of %s to %s failed - session server not found.", appName.c_str(),
                                        toString(newState));
        return false;
    }
    kSessionServer->setExpectedState(newState);
    if (!m_ipcController->performSetState(kSessionServer->getServerId(), newState))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Change state of %s to %s failed.", appName.c_str(), toString(newState));
        handleStateChangeFailure(kSessionServer, newState);
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("Change state of %s to %s succeeded.", appName.c_str(), toString(newState));
    return true;
}

void SessionServerAppManager::handleSessionServerStateChange(int serverId,
                                                             firebolt::rialto::common::SessionServerState newState)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("SessionServer with id: %d changed state to %s", serverId, toString(newState));
    const auto &kSessionServer{getServerById(serverId)};
    if (!kSessionServer)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("SessionServer with id: %d not found", serverId);
        return;
    }
    std::string appName{kSessionServer->getAppName()};
    if (!appName.empty() && m_stateObserver) // empty app name is when SessionServer is preloaded
    {
        m_stateObserver->stateChanged(appName, newState);
    }
    if (firebolt::rialto::common::SessionServerState::UNINITIALIZED == newState)
    {
        kSessionServer->cancelStartupTimer();
        if (!kSessionServer->isPreloaded() && !configureSessionServer(kSessionServer))
        {
            handleSessionServerStateChange(serverId, firebolt::rialto::common::SessionServerState::ERROR);
            kSessionServer->kill();
            handleSessionServerStateChange(serverId, firebolt::rialto::common::SessionServerState::NOT_RUNNING);
        }
    }
    else if (newState == firebolt::rialto::common::SessionServerState::ERROR && kSessionServer->isPreloaded())
    {
        m_ipcController->removeClient(serverId);
        kSessionServer->kill();
        m_healthcheckService->onServerRemoved(kSessionServer->getServerId());
        m_sessionServerApps.erase(kSessionServer);
        connectSessionServer(preloadSessionServer());
    }
    else if (newState == firebolt::rialto::common::SessionServerState::NOT_RUNNING)
    {
        m_ipcController->removeClient(serverId);
        m_healthcheckService->onServerRemoved(kSessionServer->getServerId());
        m_sessionServerApps.erase(kSessionServer);
    }
}

void SessionServerAppManager::handleAck(int serverId, int pingId, bool success)
{
    if (success)
    {
        RIALTO_SERVER_MANAGER_LOG_DEBUG("Ping with id: %d succeeded for server: %d", pingId, serverId);
    }
    else
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Ping with id: %d failed for server: %d", pingId, serverId);
    }
    m_healthcheckService->onAckReceived(serverId, pingId, success);
}

void SessionServerAppManager::shutdownAllSessionServers()
{
    for (const auto &kSessionServer : m_sessionServerApps)
    {
        kSessionServer->kill();
    }
    m_sessionServerApps.clear();
}

const std::unique_ptr<ISessionServerApp> &
SessionServerAppManager::launchSessionServer(const std::string &appName,
                                             const firebolt::rialto::common::SessionServerState &kInitialState,
                                             const firebolt::rialto::common::AppConfig &appConfig)
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Launching Rialto Session Server for %s", appName.c_str());
    auto app = m_sessionServerAppFactory->create(appName, kInitialState, appConfig, *this);
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

void SessionServerAppManager::handleStateChangeFailure(const std::unique_ptr<ISessionServerApp> &kSessionServer,
                                                       const firebolt::rialto::common::SessionServerState &state)
{
    if (state == firebolt::rialto::common::SessionServerState::NOT_RUNNING)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("Force change of %s to NotRunning.", kSessionServer->getAppName().c_str());
        kSessionServer->kill();
        handleSessionServerStateChange(kSessionServer->getServerId(), state);
    }
    else
    {
        handleSessionServerStateChange(kSessionServer->getServerId(),
                                       firebolt::rialto::common::SessionServerState::ERROR);
    }
}

bool SessionServerAppManager::configureSessionServerWithSocketName(const std::unique_ptr<ISessionServerApp> &kSessionServer)
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Configuring Session Server using socket name");
    const auto kInitialState{kSessionServer->getInitialState()};
    const auto kSocketName{kSessionServer->getSessionManagementSocketName()};
    const auto kClientDisplayName{kSessionServer->getClientDisplayName()};
    const auto kSocketPermissions{kSessionServer->getSessionManagementSocketPermissions()};
    const auto kSocketOwner{kSessionServer->getSessionManagementSocketOwner()};
    const auto kSocketGroup{kSessionServer->getSessionManagementSocketGroup()};
    const auto kAppName{kSessionServer->getAppName()};

    const firebolt::rialto::common::MaxResourceCapabilitites kMaxResource{kSessionServer->getMaxPlaybackSessions(),
                                                                          kSessionServer->getMaxWebAudioPlayers()};
    if (!m_ipcController->performSetConfiguration(kSessionServer->getServerId(), kInitialState, kSocketName,
                                                  kClientDisplayName, kMaxResource, kSocketPermissions, kSocketOwner,
                                                  kSocketGroup, kAppName))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Configuration of server with id %d failed - ipc error.",
                                        kSessionServer->getServerId());
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("Configuration of server with id %d succeeded.", kSessionServer->getServerId());
    return true;
}

bool SessionServerAppManager::configureSessionServerWithSocketFd(const std::unique_ptr<ISessionServerApp> &kSessionServer)
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Configuring Session Server using socket fd");
    const auto kInitialState{kSessionServer->getInitialState()};
    const auto kSocketFd{kSessionServer->getSessionManagementSocketFd()};
    const auto kClientDisplayName{kSessionServer->getClientDisplayName()};
    const auto kAppName{kSessionServer->getAppName()};

    const firebolt::rialto::common::MaxResourceCapabilitites kMaxResource{kSessionServer->getMaxPlaybackSessions(),
                                                                          kSessionServer->getMaxWebAudioPlayers()};
    if (!m_ipcController->performSetConfiguration(kSessionServer->getServerId(), kInitialState, kSocketFd,
                                                  kClientDisplayName, kMaxResource, kAppName))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Configuration of server with id %d failed - ipc error.",
                                        kSessionServer->getServerId());
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_INFO("Configuration of server with id %d succeeded.", kSessionServer->getServerId());
    return true;
}
} // namespace rialto::servermanager::common
