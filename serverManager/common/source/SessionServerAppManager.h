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
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace rialto::servermanager::common
{
class SessionServerAppManager : public ISessionServerAppManager
{
public:
    SessionServerAppManager(std::unique_ptr<ipc::IController> &ipcController,
                            const std::shared_ptr<service::IStateObserver> &stateObserver,
                            std::unique_ptr<ISessionServerAppFactory> &&sessionServerAppFactory,
                            const std::shared_ptr<firebolt::rialto::common::IEventThreadFactory> &eventThreadFactory);
    virtual ~SessionServerAppManager();
    SessionServerAppManager(const SessionServerAppManager &) = delete;
    SessionServerAppManager(SessionServerAppManager &&) = delete;
    SessionServerAppManager &operator=(const SessionServerAppManager &) = delete;
    SessionServerAppManager &operator=(SessionServerAppManager &&) = delete;

    bool setSessionServerState(const std::string &appId, const service::SessionServerState &newState) override;
    void onSessionServerStateChanged(const std::string &appId, const service::SessionServerState &newState) override;
    std::string getAppConnectionInfo(const std::string &appId) const override;
    bool setLogLevels(const service::LoggingLevels &logLevels) const override;

private:
    bool addSessionServer(const std::string &appId, const service::SessionServerState &initialState);
    bool launchSessionServer(const std::string &appId, const service::SessionServerState &initialState);
    void removeSessionServer(const std::string &appId, bool killApp = false);
    bool configureSessionServer(const std::string &appId);
    bool changeSessionServerState(const std::string &appId, const service::SessionServerState &newState);
    bool isSessionServerLaunched(const std::string &appId) const;
    void cancelSessionServerStartupTimer(const std::string &appId) const;
    int getAppManagementSocketName(const std::string &appId) const;
    void handleSessionServerStateChange(std::string appId, service::SessionServerState newState);
    void shutdownAllSessionServers();

private:
    mutable std::mutex m_sessionServerAppsMutex;
    std::unique_ptr<ipc::IController> &m_ipcController;
    std::unique_ptr<firebolt::rialto::common::IEventThread> m_eventThread;
    std::map<std::string, std::unique_ptr<ISessionServerApp>> m_sessionServerApps;
    std::unique_ptr<ISessionServerAppFactory> m_sessionServerAppFactory;
    std::shared_ptr<service::IStateObserver> m_stateObserver;
};
} // namespace rialto::servermanager::common

#endif // RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_MANAGER_H_
