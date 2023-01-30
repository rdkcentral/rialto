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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_SESSION_SERVER_MANAGER_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_SESSION_SERVER_MANAGER_MOCK_H_

#include "ISessionServerManager.h"
#include <gmock/gmock.h>
#include <string>

namespace firebolt::rialto::server::service
{
class SessionServerManagerMock : public ISessionServerManager
{
public:
    MOCK_METHOD(bool, initialize, (int argc, char *argv[]), (override));
    MOCK_METHOD(void, startService, (), (override));
    MOCK_METHOD(bool, setConfiguration,
                (const std::string &socketName, const SessionServerState &state,
                 const MaxResourceCapabilitites &maxResource),
                (override));
    MOCK_METHOD(bool, setState, (const SessionServerState &state), (override));
    MOCK_METHOD(void, setLogLevels,
                (RIALTO_DEBUG_LEVEL defaultLogLevels, RIALTO_DEBUG_LEVEL clientLogLevels,
                 RIALTO_DEBUG_LEVEL sessionServerLogLevels, RIALTO_DEBUG_LEVEL ipcLogLevels,
                 RIALTO_DEBUG_LEVEL serverManagerLogLevels, RIALTO_DEBUG_LEVEL commonLogLevels),
                (override));
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_SESSION_SERVER_MANAGER_MOCK_H_
