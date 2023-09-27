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

#include "ClientController.h"
#include "RialtoClientLogging.h"
#include "SharedMemoryHandle.h"
#include <cstring>
#include <sys/mman.h>
#include <sys/un.h>
#include <unistd.h>

namespace firebolt::rialto::client
{
IClientControllerAccessor &IClientControllerAccessor::instance()
{
    static ClientControllerAccessor factory;
    return factory;
}

IClientController &ClientControllerAccessor::getClientController() const
{
    static ClientController ClientController{IControlIpcFactory::createFactory()};
    return ClientController;
}

ClientController::ClientController(const std::shared_ptr<IControlIpcFactory> &ControlIpcFactory)
    : m_currentState{ApplicationState::UNKNOWN}
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    const char commitID[] = COMMIT_ID;

    if (std::strlen(commitID) > 0)
    {
        RIALTO_CLIENT_LOG_MIL("Commit ID: %s", commitID);
    }
    else
    {
        RIALTO_CLIENT_LOG_WARN("Failed to get git commit ID.");
    }

    m_controlIpc = ControlIpcFactory->getControlIpc(this);
    if (nullptr == m_controlIpc)
    {
        throw std::runtime_error("Failed to create the ControlIpc object");
    }
}

ClientController::~ClientController()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    termSharedMemory();
}

std::shared_ptr<ISharedMemoryHandle> ClientController::getSharedMemoryHandle()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_shmHandle;
}

bool ClientController::registerClient(IControlClient *client, ApplicationState &appState)
{
    if (nullptr == client)
    {
        RIALTO_CLIENT_LOG_ERROR("Client ptr is null");
        return false;
    }

    std::lock_guard<std::mutex> lock{m_mutex};
    if (ApplicationState::UNKNOWN == m_currentState)
    {
        if (!m_controlIpc->registerClient())
        {
            RIALTO_CLIENT_LOG_ERROR("Failed to register client");
            return false;
        }
    }
    m_clientVec.insert(client);
    appState = m_currentState;

    return true;
}

bool ClientController::unregisterClient(IControlClient *client)
{
    if (nullptr == client)
    {
        RIALTO_CLIENT_LOG_ERROR("Client ptr is null");
        return false;
    }

    std::lock_guard<std::mutex> lock{m_mutex};

    auto numDeleted = m_clientVec.erase(client);
    if (0 == numDeleted)
    {
        RIALTO_CLIENT_LOG_ERROR("No client unregistered");
        return false;
    }

    return true;
}

bool ClientController::initSharedMemory()
try
{
    std::lock_guard<std::mutex> lock{m_mutex};
    int32_t shmFd{-1};
    uint32_t shmBufferLen{0U};
    if (!m_controlIpc->getSharedMemory(shmFd, shmBufferLen))
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to get the shared memory");
        return false;
    }
    m_shmHandle = std::make_shared<SharedMemoryHandle>(shmFd, shmBufferLen);

    RIALTO_CLIENT_LOG_INFO("Shared buffer was successfully initialised");
    return true;
}
catch (const std::exception &e)
{
    RIALTO_CLIENT_LOG_ERROR("Failed to initialise shared memory: %s", e.what());
    return false;
}

void ClientController::termSharedMemory()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    m_shmHandle.reset();
}

void ClientController::notifyApplicationState(ApplicationState state)
{
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_currentState == state)
        {
            RIALTO_CLIENT_LOG_WARN("Rialto application state already set, %s", stateToString(m_currentState).c_str());
            return;
        }
    }

    switch (state)
    {
    case ApplicationState::RUNNING:
    {
        if (!initSharedMemory())
        {
            RIALTO_CLIENT_LOG_ERROR("Could not initalise the shared memory");
            return;
        }
        // Inform clients after memory initialisation
        changeStateAndNotifyClients(state);
        break;
    }
    case ApplicationState::INACTIVE:
    case ApplicationState::UNKNOWN:
    {
        // Inform clients before memory termination
        changeStateAndNotifyClients(state);
        termSharedMemory();
        break;
    }
    }
}

std::string ClientController::stateToString(ApplicationState state)
{
    switch (state)
    {
    case ApplicationState::RUNNING:
    {
        return "RUNNING";
    }
    case ApplicationState::INACTIVE:
    {
        return "INACTIVE";
    }
    case ApplicationState::UNKNOWN:
    default:
    {
        return "UNKNOWN";
    }
    }
}

void ClientController::changeStateAndNotifyClients(ApplicationState state)
{
    std::set<IControlClient *> currentClients;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        RIALTO_CLIENT_LOG_INFO("Rialto application state changed from %s to %s", stateToString(m_currentState).c_str(),
                               stateToString(state).c_str());
        m_currentState = state;
        currentClients = m_clientVec;
    }
    for (const auto &client : currentClients)
    {
        client->notifyApplicationState(state);
    }
}
} // namespace firebolt::rialto::client
