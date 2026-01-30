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

#include "Controller.h"
#include "RialtoServerManagerLogging.h"
#include <algorithm>
#include <string>
#include <utility>

namespace rialto::servermanager::ipc
{
Controller::Controller(std::unique_ptr<common::ISessionServerAppManager> &sessionServerAppManager)
    : m_sessionServerAppManager{sessionServerAppManager}
{
}

bool Controller::createClient(int serverId, int appMgmtSocket)
{
    std::unique_lock<std::mutex> lock{m_clientMutex};
    if (m_clients.find(serverId) == m_clients.end())
    {
        auto newClient = std::make_unique<Client>(m_sessionServerAppManager, serverId, appMgmtSocket);
        if (newClient->connect())
        {
            m_clients.emplace(std::make_pair(serverId, std::move(newClient)));
            return true;
        }
        return false;
    }
    return false;
}

void Controller::removeClient(int serverId)
{
    std::unique_lock<std::mutex> lock{m_clientMutex};
    m_clients.erase(serverId);
}

bool Controller::performSetState(int serverId, const firebolt::rialto::common::SessionServerState &state)
{
    std::unique_lock<std::mutex> lock{m_clientMutex};
    auto client = m_clients.find(serverId);
    if (client != m_clients.end())
    {
        return client->second->performSetState(state);
    }
    return false;
}

bool Controller::performSetConfiguration(int serverId, const firebolt::rialto::common::SessionServerState &initialState,
                                         const std::string &socketName, const std::string &clientDisplayName,
                                         const std::string &subtitlesDisplayName,
                                         const firebolt::rialto::common::MaxResourceCapabilitites &maxResource,
                                         const unsigned int socketPermissions, const std::string &socketOwner,
                                         const std::string &socketGroup, const std::string &appName)
{
    std::unique_lock<std::mutex> lock{m_clientMutex};
    auto client = m_clients.find(serverId);
    if (client != m_clients.end())
    {
        return client->second->performSetConfiguration(initialState, socketName, clientDisplayName,
                                                       subtitlesDisplayName, maxResource,
                                                       socketPermissions, socketOwner, socketGroup, appName);
    }
    return false;
}

bool Controller::performSetConfiguration(int serverId, const firebolt::rialto::common::SessionServerState &initialState,
                                         int socketFd, const std::string &clientDisplayName,
                                         const std::string &subtitlesDisplayName,
                                         const firebolt::rialto::common::MaxResourceCapabilitites &maxResource,
                                         const std::string &appName)
{
    std::unique_lock<std::mutex> lock{m_clientMutex};
    auto client = m_clients.find(serverId);
    if (client != m_clients.end())
    {
        return client->second->performSetConfiguration(initialState, socketFd, clientDisplayName,
                                                       subtitlesDisplayName, maxResource, appName);
    }
    return false;
}

bool Controller::performPing(int serverId, int pingId)
{
    std::unique_lock<std::mutex> lock{m_clientMutex};
    auto client = m_clients.find(serverId);
    if (client != m_clients.end())
    {
        return client->second->performPing(pingId);
    }
    return false;
}

bool Controller::setLogLevels(const service::LoggingLevels &logLevels) const
{
    std::unique_lock<std::mutex> lock{m_clientMutex};
    return std::all_of(m_clients.begin(), m_clients.end(),
                       [&](const auto &client) { return client.second->setLogLevels(logLevels); });
}
} // namespace rialto::servermanager::ipc
