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

#ifndef RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_MANAGER_MOCK_H_
#define RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_MANAGER_MOCK_H_

#include "ISessionServerAppManager.h"
#include <gmock/gmock.h>
#include <string>

namespace rialto::servermanager::common
{
class SessionServerAppManagerMock : public ISessionServerAppManager
{
public:
    SessionServerAppManagerMock() = default;
    virtual ~SessionServerAppManagerMock() = default;

    MOCK_METHOD(void, preloadSessionServers, (unsigned numOfPreloadedServers), (override));
    MOCK_METHOD(bool, initiateApplication,
                (const std::string &appName, const firebolt::rialto::common::SessionServerState &state,
                 const firebolt::rialto::common::AppConfig &appConfig),
                (override));
    MOCK_METHOD(bool, setSessionServerState,
                (const std::string &, const firebolt::rialto::common::SessionServerState &), (override));
    MOCK_METHOD(void, onSessionServerStateChanged, (int, const firebolt::rialto::common::SessionServerState &),
                (override));
    MOCK_METHOD(void, sendPingEvents, (int pingId), (override));
    MOCK_METHOD(void, onAck, (int serverId, int pingId, bool success), (override));
    MOCK_METHOD(std::string, getAppConnectionInfo, (const std::string &), (const, override));
    MOCK_METHOD(bool, setLogLevels, (const service::LoggingLevels &), (const, override));
    MOCK_METHOD(bool, restartServer, (int serverId), (override));
};
} // namespace rialto::servermanager::common

#endif // RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_MANAGER_MOCK_H_
