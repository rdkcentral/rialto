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

#ifndef RIALTO_SERVERMANAGER_COMMON_I_SESSION_SERVER_APP_H_
#define RIALTO_SERVERMANAGER_COMMON_I_SESSION_SERVER_APP_H_

#include "SessionServerCommon.h"
#include <string>

namespace rialto::servermanager::common
{
class ISessionServerApp
{
public:
    ISessionServerApp() = default;
    virtual ~ISessionServerApp() = default;

    ISessionServerApp(const ISessionServerApp &) = delete;
    ISessionServerApp &operator=(const ISessionServerApp &) = delete;
    ISessionServerApp(ISessionServerApp &&) = delete;
    ISessionServerApp &operator=(ISessionServerApp &&) = delete;

    virtual bool launch() = 0;
    virtual bool isPreloaded() const = 0;
    virtual bool configure(const std::string &appName, const firebolt::rialto::common::SessionServerState &initialState,
                           const firebolt::rialto::common::AppConfig &appConfig) = 0;
    virtual bool isConnected() const = 0;
    virtual std::string getSessionManagementSocketName() const = 0;
    virtual firebolt::rialto::common::SessionServerState getInitialState() const = 0;
    virtual int getServerId() const = 0;
    virtual const std::string &getAppName() const = 0;
    virtual int getAppManagementSocketName() const = 0;
    virtual int getMaxPlaybackSessions() const = 0;
    virtual int getMaxWebAudioPlayers() const = 0;
    virtual void cancelStartupTimer() = 0;
    virtual void kill() const = 0;
};
} // namespace rialto::servermanager::common

#endif // RIALTO_SERVERMANAGER_COMMON_I_SESSION_SERVER_APP_H_
