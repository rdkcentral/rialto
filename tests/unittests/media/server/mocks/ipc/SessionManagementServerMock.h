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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_SESSION_MANAGEMENT_SERVER_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_IPC_SESSION_MANAGEMENT_SERVER_MOCK_H_

#include "ISessionManagementServer.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>

namespace firebolt::rialto::server::ipc
{
class SessionManagementServerMock : public ISessionManagementServer
{
public:
    MOCK_METHOD(bool, initialize,
                (const std::string &socketName, unsigned int socketPermissions, const std::string &socketOwner,
                 const std::string &socketGroup),
                (override));
    MOCK_METHOD(bool, initialize, (int32_t socketFd), (override));
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, setLogLevels,
                (RIALTO_DEBUG_LEVEL defaultLogLevels, RIALTO_DEBUG_LEVEL clientLogLevels,
                 RIALTO_DEBUG_LEVEL ipcLogLevels, RIALTO_DEBUG_LEVEL commonLogLevels),
                (override));
};
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_SESSION_MANAGEMENT_SERVER_MOCK_H_
