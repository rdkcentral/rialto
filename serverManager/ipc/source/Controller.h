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

#ifndef RIALTO_SERVERMANAGER_IPC_CONTROLLER_H_
#define RIALTO_SERVERMANAGER_IPC_CONTROLLER_H_

#include "Client.h"
#include "IController.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace rialto::servermanager
{
namespace common
{
class ISessionServerAppManager;
} // namespace common
} // namespace rialto::servermanager

namespace rialto::servermanager::ipc
{
class Controller : public IController
{
public:
    explicit Controller(std::unique_ptr<common::ISessionServerAppManager> &sessionServerAppManager);
    virtual ~Controller() = default;

    Controller(const Controller &) = delete;
    Controller(Controller &&) = delete;
    Controller &operator=(const Controller &) = delete;
    Controller &operator=(Controller &&) = delete;

    bool createClient(int serverId, int appMgmtSocket) override;
    void removeClient(int serverId) override;
    bool performSetConfiguration(int serverId, const firebolt::rialto::common::SessionServerState &initialState,
                                 const std::string &socketName, const std::string &clientDisplayName,
                                 const firebolt::rialto::common::MaxResourceCapabilitites &maxResource,
                                 const unsigned int socketPermissions) override;
    bool performPing(int serverId, int pingId) override;
    bool performSetState(int serverId, const firebolt::rialto::common::SessionServerState &state) override;
    bool setLogLevels(const service::LoggingLevels &logLevels) const override;

private:
    mutable std::mutex m_clientMutex;
    std::unique_ptr<common::ISessionServerAppManager> &m_sessionServerAppManager;
    std::map<int, std::unique_ptr<Client>> m_clients;
};
} // namespace rialto::servermanager::ipc

#endif // RIALTO_SERVERMANAGER_IPC_CONTROLLER_H_
