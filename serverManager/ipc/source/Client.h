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

#ifndef RIALTO_SERVERMANAGER_IPC_CLIENT_H_
#define RIALTO_SERVERMANAGER_IPC_CLIENT_H_

#include "IIpcChannel.h"
#include "ISessionServerAppManager.h"
#include "IpcLoop.h"
#include "LoggingLevels.h"
#include <memory>
#include <string>

namespace rialto
{
class ServerManagerModule_Stub;
class StateChangedEvent;
} // namespace rialto

namespace rialto::servermanager::ipc
{
class Client
{
public:
    Client(std::unique_ptr<common::ISessionServerAppManager> &sessionServerAppManager, const std::string &appId,
           int socket);
    ~Client();
    Client(const Client &) = delete;
    Client(Client &&) = delete;
    Client &operator=(const Client &) = delete;
    Client &operator=(Client &&) = delete;

    bool connect();
    bool performSetState(const firebolt::rialto::common::SessionServerState &state);
    bool performSetConfiguration(const firebolt::rialto::common::SessionServerState &initialState, const std::string &socketName,
                                 const firebolt::rialto::common::MaxResourceCapabilitites &maxResource) const;
    bool setLogLevels(const service::LoggingLevels &logLevels) const;
    void onDisconnected() const;

private:
    void onStateChangedEvent(const std::shared_ptr<rialto::StateChangedEvent> &event) const;

private:
    std::string m_appId;
    std::unique_ptr<common::ISessionServerAppManager> &m_sessionServerAppManager;
    int m_socket;
    std::shared_ptr<::firebolt::rialto::ipc::IChannel> m_channel;
    std::shared_ptr<IpcLoop> m_ipcLoop;
    std::unique_ptr<::rialto::ServerManagerModule_Stub> m_serviceStub;
};
} // namespace rialto::servermanager::ipc

#endif // RIALTO_SERVERMANAGER_IPC_CLIENT_H_
