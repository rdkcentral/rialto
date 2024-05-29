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

#ifndef RIALTO_SERVERMANAGER_IPC_I_CONTROLLER_H_
#define RIALTO_SERVERMANAGER_IPC_I_CONTROLLER_H_

#include "LoggingLevels.h"
#include "SessionServerCommon.h"
#include <memory>
#include <string>

namespace rialto::servermanager::ipc
{
class IController
{
public:
    IController() = default;
    virtual ~IController() = default;

    IController(const IController &) = delete;
    IController(IController &&) = delete;
    IController &operator=(const IController &) = delete;
    IController &operator=(IController &&) = delete;

    virtual bool createClient(int serverId, int appMgmtSocket) = 0;
    virtual void removeClient(int serverId) = 0;
    virtual bool performSetConfiguration(int serverId, const firebolt::rialto::common::SessionServerState &initialState,
                                         const std::string &socketName, const std::string &clientDisplayName,
                                         const firebolt::rialto::common::MaxResourceCapabilitites &maxResource,
                                         const unsigned int socketPermissions, const std::string &socketOwner,
                                         const std::string &socketGroup) = 0;
    virtual bool performPing(int serverId, int pingId) = 0;
    virtual bool performSetState(int serverId, const firebolt::rialto::common::SessionServerState &state) = 0;
    virtual bool setLogLevels(const service::LoggingLevels &logLevels) const = 0;
};
} // namespace rialto::servermanager::ipc

#endif // RIALTO_SERVERMANAGER_IPC_I_CONTROLLER_H_
