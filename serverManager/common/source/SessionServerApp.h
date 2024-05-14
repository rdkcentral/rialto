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

#ifndef RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_H_
#define RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_H_

#include "ILinuxWrapper.h"
#include "ISessionServerApp.h"
#include "ITimer.h"
#include "SessionServerAppManager.h"
#include "SessionServerCommon.h"
#include <array>
#include <chrono>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <vector>

namespace rialto::servermanager::common
{
class SessionServerApp : public ISessionServerApp
{
public:
    SessionServerApp(const std::shared_ptr<firebolt::rialto::wrappers::ILinuxWrapper> &linuxWrapper,
                     const std::shared_ptr<firebolt::rialto::common::ITimerFactory> &timerFactory,
                     ISessionServerAppManager &sessionServerAppManager,
                     const std::list<std::string> &environmentVariables, const std::string &sessionServerPath,
                     std::chrono::milliseconds sessionServerStartupTimeout, unsigned int socketPermissions,
                     const std::string &socketOwner, const std::string &socketGroup, bool enableInstantRateChangeSeek);
    SessionServerApp(const std::string &appName, const firebolt::rialto::common::SessionServerState &initialState,
                     const firebolt::rialto::common::AppConfig &appConfig,
                     const std::shared_ptr<firebolt::rialto::wrappers::ILinuxWrapper> &linuxWrapper,
                     const std::shared_ptr<firebolt::rialto::common::ITimerFactory> &timerFactory,
                     ISessionServerAppManager &sessionServerAppManager,
                     const std::list<std::string> &environmentVariables, const std::string &sessionServerPath,
                     std::chrono::milliseconds sessionServerStartupTimeout, unsigned int socketPermissions,
                     const std::string &socketOwner, const std::string &socketGroup, bool enableInstantRateChangeSeek);
    virtual ~SessionServerApp();

    bool launch() override;
    bool isPreloaded() const override;
    bool configure(const std::string &appName, const firebolt::rialto::common::SessionServerState &initialState,
                   const firebolt::rialto::common::AppConfig &appConfig) override;
    bool isConnected() const override;
    std::string getSessionManagementSocketName() const override;
    unsigned int getSessionManagementSocketPermissions() const override;
    std::string getSessionManagementSocketOwner() const override;
    std::string getSessionManagementSocketGroup() const override;
    bool getEnableInstantRateChangeSeek() const override;
    firebolt::rialto::common::SessionServerState getInitialState() const override;
    int getServerId() const override;
    const std::string &getAppName() const override;
    int getAppManagementSocketName() const override;
    std::string getClientDisplayName() const override;
    int getMaxPlaybackSessions() const override;
    int getMaxWebAudioPlayers() const override;
    void cancelStartupTimer() override;
    void kill() const override;
    void setExpectedState(const firebolt::rialto::common::SessionServerState &state) override;
    firebolt::rialto::common::SessionServerState getExpectedState() const override;

private:
    bool initializeSockets();
    void setupStartupTimer();
    bool spawnSessionServer();
    void waitForChildProcess();
    void cancelStartupTimerInternal(); // to avoid calling virtual method in destructor
    std::string addAppSuffixToLogFile(const std::string &envVar) const;

private:
    const int m_kServerId;
    std::string m_appName;
    firebolt::rialto::common::SessionServerState m_initialState;
    std::string m_sessionManagementSocketName;
    std::string m_clientDisplayName;
    std::array<int, 2> m_socks;
    std::shared_ptr<firebolt::rialto::wrappers::ILinuxWrapper> m_linuxWrapper;
    std::shared_ptr<firebolt::rialto::common::ITimerFactory> m_timerFactory;
    ISessionServerAppManager &m_sessionServerAppManager;
    pid_t m_pid;
    bool m_isPreloaded;
    const std::string m_kSessionServerPath;
    const std::chrono::milliseconds m_kSessionServerStartupTimeout;
    const unsigned int m_kSessionManagementSocketPermissions;
    const std::string m_kSessionManagementSocketOwner;
    const std::string m_kSessionManagementSocketGroup;
    const bool m_kEnableInstantRateChangeSeek;
    std::vector<char *> m_environmentVariables;
    mutable std::mutex m_timerMutex;
    std::unique_ptr<firebolt::rialto::common::ITimer> m_startupTimer;
    std::mutex m_processStartupMutex;
    std::condition_variable m_processStartupCv;
    bool m_childInitialized;
    firebolt::rialto::common::SessionServerState m_expectedState;
};
} // namespace rialto::servermanager::common

#endif // RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_H_
