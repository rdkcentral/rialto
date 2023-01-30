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

#include "ISessionServerApp.h"
#include "ITimer.h"
#include "SessionServerAppManager.h"
#include "SessionServerState.h"
#include <array>
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
    SessionServerApp(const std::string &appId, const service::SessionServerState &initialState,
                     SessionServerAppManager &sessionServerAppManager,
                     const std::list<std::string> &environmentVariables);
    virtual ~SessionServerApp();

    bool launch() override;
    std::string getSessionManagementSocketName() const override;
    service::SessionServerState getInitialState() const override;
    int getAppManagementSocketName() const override;
    int getMaxPlaybackSessions() const override;
    int getMaxWebAudioPlayers() const override;
    void cancelStartupTimer() override;
    void kill() const override;

private:
    bool initializeSockets();
    void setupStartupTimer();
    bool spawnSessionServer();
    void cancelStartupTimerInternal(); // to avoid calling virtual method in destructor

private:
    const std::string m_kAppId;
    const service::SessionServerState m_kInitialState;
    const std::string m_kSessionManagementSocketName;
    std::array<int, 2> m_socks;
    SessionServerAppManager &m_sessionServerAppManager;
    pid_t m_pid;
    std::vector<char *> m_environmentVariables;
    std::mutex m_timerMutex;
    std::unique_ptr<firebolt::rialto::common::ITimer> m_startupTimer;
};
} // namespace rialto::servermanager::common

#endif // RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_H_
