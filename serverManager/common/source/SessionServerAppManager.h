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

#ifndef RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_MANAGER_H_
#define RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_MANAGER_H_

#include "IController.h"
#include "IEventThread.h"
#include "ISessionServerApp.h"
#include "ISessionServerAppManager.h"
#include "IStateObserver.h"
#include "SessionServerAppFactory.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace rialto::servermanager::common
{
class SessionServerAppManager : public ISessionServerAppManager
{
public:
    SessionServerAppManager(std::unique_ptr<ipc::IController> &ipcController,
                            const std::shared_ptr<service::IStateObserver> &stateObserver,
                            std::unique_ptr<ISessionServerAppFactory> &&sessionServerAppFactory,
                            const std::shared_ptr<firebolt::rialto::common::IEventThreadFactory> &eventThreadFactory,
                            int numberOfPreloadedServers);
    virtual ~SessionServerAppManager();
    SessionServerAppManager(const SessionServerAppManager &) = delete;
    SessionServerAppManager(SessionServerAppManager &&) = delete;
    SessionServerAppManager &operator=(const SessionServerAppManager &) = delete;
    SessionServerAppManager &operator=(SessionServerAppManager &&) = delete;

    bool initiateApplication(const std::string &appName, const firebolt::rialto::common::SessionServerState &state,
                             const firebolt::rialto::common::AppConfig &appConfig) override;
    bool setSessionServerState(const std::string &appName,
                               const firebolt::rialto::common::SessionServerState &newState) override;
    void onSessionServerStateChanged(int appId, const firebolt::rialto::common::SessionServerState &newState) override;
    std::string getAppConnectionInfo(const std::string &appName) const override;
    bool setLogLevels(const service::LoggingLevels &logLevels) const override;

private:
    bool addSessionServer(const std::string &appName, const firebolt::rialto::common::SessionServerState &initialState,
                          const firebolt::rialto::common::AppConfig &appConfig);
    void addPreloadedSessionServer();
    int launchSessionServer(const std::string &appName, const firebolt::rialto::common::SessionServerState &initialState,
                            const firebolt::rialto::common::AppConfig &appConfig);
    int preloadSessionServer();
    void removeSessionServer(int appId, bool killApp = false);
    bool configureSessionServer(int appId);
    bool configurePreloadedSessionServer(int appId, const std::string &appName,
                                         const firebolt::rialto::common::SessionServerState &state,
                                         const firebolt::rialto::common::AppConfig &appConfig);
    bool addSessionServerConfiguration(int appId, const std::string &appName,
                                       const firebolt::rialto::common::SessionServerState &state,
                                       const firebolt::rialto::common::AppConfig &appConfig);
    bool changeSessionServerState(const std::string &appName,
                                  const firebolt::rialto::common::SessionServerState &newState);
    bool isSessionServerLaunched(const std::string &appName) const;
    void cancelSessionServerStartupTimer(int appId) const;
    bool isSessionServerPreloaded(int appId) const;
    int getAppId(const std::string &appName) const;
    std::string getAppName(int appId) const;
    int getAppManagementSocketName(int appId) const;
    void handleSessionServerStateChange(int appId, firebolt::rialto::common::SessionServerState newState);
    void shutdownAllSessionServers();
    int getPreloadedServerId() const;

private:
    mutable std::mutex m_sessionServerAppsMutex;
    std::unique_ptr<ipc::IController> &m_ipcController;
    std::unique_ptr<firebolt::rialto::common::IEventThread> m_eventThread;
    std::vector<std::unique_ptr<ISessionServerApp>> m_sessionServerApps;
    std::unique_ptr<ISessionServerAppFactory> m_sessionServerAppFactory;
    std::shared_ptr<service::IStateObserver> m_stateObserver;
};
} // namespace rialto::servermanager::common

#endif // RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_MANAGER_H_
