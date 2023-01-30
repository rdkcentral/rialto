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

bool Controller::createClient(const std::string &appId, int appMgmtSocket)
{
    std::unique_lock<std::mutex> lock{m_clientMutex};
    if (m_clients.find(appId) == m_clients.end())
    {
        auto newClient = std::make_unique<Client>(m_sessionServerAppManager, appId, appMgmtSocket);
        if (newClient->connect())
        {
            m_clients.emplace(std::make_pair(appId, std::move(newClient)));
            return true;
        }
        return false;
    }
    return false;
}

void Controller::removeClient(const std::string &appId)
{
    std::unique_lock<std::mutex> lock{m_clientMutex};
    m_clients.erase(appId);
}

bool Controller::performSetState(const std::string &appId, const service::SessionServerState &state)
{
    std::unique_lock<std::mutex> lock{m_clientMutex};
    auto client = m_clients.find(appId);
    if (client != m_clients.end())
    {
        return client->second->performSetState(state);
    }
    return false;
}

bool Controller::performSetConfiguration(const std::string &appId, const service::SessionServerState &initialState,
                                         const std::string &socketName,
                                         const service::MaxResourceCapabilitites &maxResource)
{
    std::unique_lock<std::mutex> lock{m_clientMutex};
    auto client = m_clients.find(appId);
    if (client != m_clients.end())
    {
        return client->second->performSetConfiguration(initialState, socketName, maxResource);
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
