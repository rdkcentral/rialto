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

#ifndef RIALTO_SERVERMANAGER_IPC_CONTROLLER_MOCK_H_
#define RIALTO_SERVERMANAGER_IPC_CONTROLLER_MOCK_H_

#include "IController.h"
#include <gmock/gmock.h>
#include <string>

namespace rialto::servermanager::ipc
{
class ControllerMock : public IController
{
public:
    ControllerMock() = default;
    virtual ~ControllerMock() = default;

    MOCK_METHOD(bool, createClient, (int, int), (override));
    MOCK_METHOD(void, removeClient, (int), (override));
    MOCK_METHOD(bool, performSetConfiguration,
                (int, const firebolt::rialto::common::SessionServerState &, const std::string &, const std::string &,
                 const std::string &, const firebolt::rialto::common::MaxResourceCapabilitites &, const unsigned int,
                 const std::string &, const std::string &, const std::string &),
                (override));
    MOCK_METHOD(bool, performSetConfiguration,
                (int serverId, const firebolt::rialto::common::SessionServerState &initialState, int socketFd,
                 const std::string &clientDisplayName, const std::string &subtitlesDisplayName,
                 const firebolt::rialto::common::MaxResourceCapabilitites &maxResource, const std::string &appName),
                (override));
    MOCK_METHOD(bool, performSetState, (int, const firebolt::rialto::common::SessionServerState &), (override));
    MOCK_METHOD(bool, performPing, (int serverId, int pingId), (override));
    MOCK_METHOD(bool, setLogLevels, (const service::LoggingLevels &), (const, override));
};
} // namespace rialto::servermanager::ipc

#endif // RIALTO_SERVERMANAGER_IPC_CONTROLLER_MOCK_H_
