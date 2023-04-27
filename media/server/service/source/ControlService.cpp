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
int generateControlId()
{
    static int id{0};
    return id++;
}
} // namespace

namespace firebolt::rialto::server::service
{
ControlService::ControlService(IControlServerInternal &controlServerInternal)
    : m_currentState{ApplicationState::UNKNOWN}, m_controlServerInternal{controlServerInternal}
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
}

int ControlService::registerClient(const std::shared_ptr<IControlClientServerInternal> &client)
{
    int id{generateControlId()};
    RIALTO_SERVER_LOG_INFO("Creating new Control with id: %d", id);
    m_controlServerInternal.registerClient(client.get(), m_currentState);
    m_clients.emplace(id, client);
    return id;
}

void ControlService::unregisterClient(int controlId)
{
    RIALTO_SERVER_LOG_INFO("Removing Control with id: %d", controlId);
    m_clients.erase(controlId);
}

bool ControlService::ack(std::uint32_t id)
{
    m_controlServerInternal.ack(id);
    return true;
}

void ControlService::setApplicationState(const ApplicationState &state)
{
    m_currentState = state;
    for (const auto &controlClient : m_clients)
    {
        controlClient.second->notifyApplicationState(state);
    }
}
} // namespace firebolt::rialto::server::service
