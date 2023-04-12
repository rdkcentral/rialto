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

#include "Control.h"
#include "IControlIpc.h"
#include "RialtoClientLogging.h"
#include "SharedMemoryManager.h"

namespace firebolt::rialto
{
std::shared_ptr<IControlFactory> IControlFactory::createFactory()
{
    return client::ControlFactory::createFactory();
}
}; // namespace firebolt::rialto

namespace firebolt::rialto::client
{
std::shared_ptr<ControlFactory> ControlFactory::createFactory()
{
    std::shared_ptr<ControlFactory> factory;
    try
    {
        factory = std::make_shared<client::ControlFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the rialto control factory, reason: %s", e.what());
    }

    return factory;
}

std::shared_ptr<IControl> ControlFactory::createControl(std::weak_ptr<IControlClient> client) const
try
{
    return std::make_shared<Control>(client, SharedMemoryManager::instance());
}
catch (const std::exception &e)
{
    RIALTO_CLIENT_LOG_ERROR("Failed to create the rialto control, reason: %s", e.what());
    return nullptr;
}

Control::Control(std::weak_ptr<IControlClient> client, ISharedMemoryManager &sharedMemoryManager)
    : m_currentState(ApplicationState::UNKNOWN), m_client(client), m_sharedMemoryManager(sharedMemoryManager)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    if (!init())
    {
        throw std::runtime_error("Cound not initalise rialto control");
    }
}

Control::~Control()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    term();
}

void Control::ack(uint32_t id)
{
    RIALTO_CLIENT_LOG_WARN("Not implemented");
}

void Control::notifyApplicationState(ApplicationState state)
{
    std::lock_guard<std::mutex> lock{m_stateMutex};

    if (ApplicationState::UNKNOWN == m_currentState)
    {
        RIALTO_CLIENT_LOG_ERROR("Rialto control not initalised");
        return;
    }
    if (m_currentState == state)
    {
        RIALTO_CLIENT_LOG_WARN("Rialto application state already set, %s", stateToString(m_currentState).c_str());
        return;
    }

    switch (state)
    {
    case ApplicationState::RUNNING:
    {
        if (!m_sharedMemoryManager.initSharedMemory())
        {
            RIALTO_CLIENT_LOG_ERROR("Could not initalise the shared memory in the running state");
            return;
        }
        break;
    }
    case ApplicationState::INACTIVE:
    {
        m_sharedMemoryManager.termSharedMemory();
        break;
    }
    default:
    {
        RIALTO_CLIENT_LOG_ERROR("Invalid application state, %s", stateToString(state).c_str());
        return;
    }
    }

    RIALTO_CLIENT_LOG_INFO("Rialto application state changed from %s to %s", stateToString(m_currentState).c_str(),
                           stateToString(state).c_str());

    m_currentState = state;

    std::shared_ptr<IControlClient> client = m_client.lock();
    if (client)
    {
        client->notifyApplicationState(state);
    }
}

void Control::ping(uint32_t id)
{
    RIALTO_CLIENT_LOG_WARN("Not implemented");
}

bool Control::init()
{
    std::lock_guard<std::mutex> lock{m_stateMutex};

    m_currentState = ApplicationState::INACTIVE;

    return true;
}

void Control::term()
{
    std::lock_guard<std::mutex> lock{m_stateMutex};

    m_currentState = ApplicationState::UNKNOWN;
}

std::string Control::stateToString(ApplicationState state)
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
    {
        return "UNKNOWN";
    }
    default:
    {
        return "";
    }
    }
}

}; // namespace firebolt::rialto::client
