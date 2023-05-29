/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "ControlService.h"
#include "RialtoServerLogging.h"

namespace
{
const char *convertApplicationState(const firebolt::rialto::ApplicationState &appState)
{
    switch (appState)
    {
    case firebolt::rialto::ApplicationState::UNKNOWN:
        return "UNKNOWN";
    case firebolt::rialto::ApplicationState::RUNNING:
        return "RUNNING";
    case firebolt::rialto::ApplicationState::INACTIVE:
        return "INACTIVE";
    }
    return "UNKNOWN";
}
} // namespace

namespace firebolt::rialto::server::service
{
ControlService::ControlService() : m_currentState{ApplicationState::UNKNOWN}
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
}

void ControlService::addControl(int controlId, const std::shared_ptr<IControlClientServerInternal> &client)
{
    RIALTO_SERVER_LOG_INFO("Creating new Control with id: %d", controlId);
    std::unique_lock<std::mutex> lock{m_mutex};
    m_clients.emplace(controlId, client);
    client->notifyApplicationState(m_currentState);
}

void ControlService::removeControl(int controlId)
{
    RIALTO_SERVER_LOG_INFO("Removing Control with id: %d", controlId);
    std::unique_lock<std::mutex> lock{m_mutex};
    m_clients.erase(controlId);
}

bool ControlService::ack(int controlId, std::uint32_t id)
{
    std::unique_lock<std::mutex> lock{m_mutex};
    auto controlClientIt = m_clients.find(controlId);
    if (m_clients.end() == controlClientIt)
    {
        RIALTO_SERVER_LOG_ERROR("Control with id: %d not found", controlId);
        return false;
    }
    // Add implementation...
    return true;
}

void ControlService::setApplicationState(const ApplicationState &state)
{
    RIALTO_SERVER_LOG_INFO("Notify rialto client about state changed to: %s", convertApplicationState(state));
    std::unique_lock<std::mutex> lock{m_mutex};
    m_currentState = state;
    for (const auto &client : m_clients)
    {
        client.second->notifyApplicationState(state);
    }
}

bool ControlService::ping(std::int32_t id)
{
    return true;
}
} // namespace firebolt::rialto::server::service
