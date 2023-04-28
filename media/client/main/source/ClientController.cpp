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
    : m_currentState{ApplicationState::UNKNOWN}, m_shmFd(-1), m_shmBuffer(nullptr), m_shmBufferLen(0U)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    m_controlIpc = ControlIpcFactory->getControlIpc(this);
    if (nullptr == m_controlIpc)
    {
        throw std::runtime_error("Failed to create the ControlIpc object");
    }

    if (!m_controlIpc->registerClient())
    {
        throw std::runtime_error("Failed to register client");
    }
}

ClientController::~ClientController()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    termSharedMemory();
}

uint8_t *ClientController::getSharedMemoryBuffer()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_shmBuffer;
}

bool ClientController::registerClient(IControlClient *client, ApplicationState &appState)
{
    if (nullptr == client)
    {
        RIALTO_CLIENT_LOG_ERROR("Client ptr is null");
        return false;
    }

    std::lock_guard<std::mutex> lock{m_mutex};
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
{
    std::lock_guard<std::mutex> lock{m_mutex};
    if (!m_controlIpc->getSharedMemory(m_shmFd, m_shmBufferLen))
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to get the shared memory");
        return false;
    }

    if ((-1 == m_shmFd) || (0U == m_shmBufferLen))
    {
        RIALTO_CLIENT_LOG_ERROR("Shared buffer invalid");
        return false;
    }

    m_shmBuffer = reinterpret_cast<uint8_t *>(mmap(NULL, m_shmBufferLen, PROT_READ | PROT_WRITE, MAP_SHARED, m_shmFd, 0));
    if (MAP_FAILED == m_shmBuffer)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to map databuffer: %s", strerror(errno));
        close(m_shmFd);
        m_shmFd = -1;
        m_shmBuffer = nullptr;
        m_shmBufferLen = 0U;
        return false;
    }

    RIALTO_CLIENT_LOG_INFO("Shared buffer was successfully initialised");
    return true;
}

void ClientController::termSharedMemory()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    if (-1 == m_shmFd)
    {
        RIALTO_CLIENT_LOG_WARN("Shared memory not initalised");
        return;
    }

    int32_t ret = munmap(m_shmBuffer, m_shmBufferLen);
    if (-1 == ret)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to unmap databuffer: %s", strerror(errno));
    }
    else
    {
        RIALTO_CLIENT_LOG_INFO("Shared buffer was successfully terminated");
    }

    close(m_shmFd);
    m_shmBuffer = nullptr;
    m_shmFd = -1;
    m_shmBufferLen = 0U;
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
    {
        // Inform clients before memory termination
        changeStateAndNotifyClients(state);
        termSharedMemory();
        break;
    }
    default:
    {
        RIALTO_CLIENT_LOG_ERROR("Invalid application state, %s", stateToString(state).c_str());
        return;
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
