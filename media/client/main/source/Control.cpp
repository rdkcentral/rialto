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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "Control.h"
#include "IControlIpc.h"
#include "RialtoClientLogging.h"
#include "ShmCommon.h"

namespace firebolt::rialto
{
std::shared_ptr<IControlFactory> IControlFactory::createFactory()
{
    return client::ControlFactory::createFactory();
}
}; // namespace firebolt::rialto

namespace firebolt::rialto::client
{
std::weak_ptr<Control> ControlFactory::m_control;
std::mutex ControlFactory::m_creationMutex;

std::shared_ptr<ISharedMemoryManagerFactory> ISharedMemoryManagerFactory::createFactory()
{
    return ControlFactory::createFactory();
}

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
{
    return getGeneric();
}

std::shared_ptr<ISharedMemoryManager> ControlFactory::getSharedMemoryManager() const
{
    return getGeneric();
}

std::shared_ptr<Control> ControlFactory::getGeneric() const
{
    std::lock_guard<std::mutex> lock{m_creationMutex};

    std::shared_ptr<Control> control = m_control.lock();
    if (!control)
    {
        try
        {
            control = std::make_shared<Control>(IControlIpcFactory::createFactory());
        }
        catch (const std::exception &e)
        {
            RIALTO_CLIENT_LOG_ERROR("Failed to create the rialto control, reason: %s", e.what());
        }

        m_control = control;
    }

    return control;
}

Control::Control(const std::shared_ptr<IControlIpcFactory> &ControlIpcFactory)
    : m_currentState(ApplicationState::UNKNOWN), m_shmFd(-1), m_shmBuffer(nullptr), m_shmBufferLen(0U)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");

    m_controlIpc = ControlIpcFactory->getControlIpc();
    if (nullptr == m_controlIpc)
    {
        throw std::runtime_error("Failed to create the ControlIpc object");
    }

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

// TODO(marcin.wojciechowski): To be rewritten
// bool Control::setControlClient(std::weak_ptr<IControlClient> client, ApplicationState &state)
// {
//     std::lock_guard<std::mutex> lock{m_stateMutex};

//     if (ApplicationState::UNKNOWN == m_currentState)
//     {
//         RIALTO_CLIENT_LOG_ERROR("Rialto control not initalised");
//         return false;
//     }

//     if (m_currentState == state)
//     {
//         RIALTO_CLIENT_LOG_WARN("Rialto application state already set, %s", stateToString(m_currentState).c_str());
//         return true;
//     }

//     switch (state)
//     {
//     case ApplicationState::RUNNING:
//     {
//         if (!initSharedMemory())
//         {
//             RIALTO_CLIENT_LOG_ERROR("Could not initalise the shared memory in the running state");
//             return false;
//         }
//         break;
//     }
//     case ApplicationState::INACTIVE:
//     {
//         termSharedMemory();
//         break;
//     }
//     default:
//     {
//         RIALTO_CLIENT_LOG_ERROR("Invalid application state, %s", stateToString(state).c_str());
//         return false;
//     }
//     }

//     RIALTO_CLIENT_LOG_INFO("Rialto application state changed from %s to %s", stateToString(m_currentState).c_str(),
//                            stateToString(state).c_str());

//     m_currentState = state;

//     return true;
// }

void Control::ack(uint32_t id)
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

    if (ApplicationState::RUNNING == m_currentState)
    {
        termSharedMemory();
    }

    m_currentState = ApplicationState::UNKNOWN;
}

uint8_t *Control::getSharedMemoryBuffer()
{
    std::lock_guard<std::mutex> lock{m_shmMutex};
    return m_shmBuffer;
}

bool Control::registerClient(ISharedMemoryManagerClient *client)
{
    if (nullptr == client)
    {
        RIALTO_CLIENT_LOG_ERROR("Client ptr is null");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock{m_clientVecMutex};

        m_clientVec.insert(client);
    }

    return true;
}

bool Control::unregisterClient(ISharedMemoryManagerClient *client)
{
    if (nullptr == client)
    {
        RIALTO_CLIENT_LOG_ERROR("Client ptr is null");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock{m_clientVecMutex};

        auto numDeleted = m_clientVec.erase(client);
        if (0 == numDeleted)
        {
            RIALTO_CLIENT_LOG_ERROR("No client unregistered");
            return false;
        }
    }

    return true;
}

bool Control::initSharedMemory()
{
    {
        std::lock_guard<std::mutex> lock{m_shmMutex};

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

        m_shmBuffer =
            reinterpret_cast<uint8_t *>(mmap(NULL, m_shmBufferLen, PROT_READ | PROT_WRITE, MAP_SHARED, m_shmFd, 0));
        if (MAP_FAILED == m_shmBuffer)
        {
            RIALTO_CLIENT_LOG_ERROR("Failed to map databuffer: %s", strerror(errno));
            close(m_shmFd);
            m_shmFd = -1;
            m_shmBuffer = nullptr;
            m_shmBufferLen = 0U;
            return false;
        }
    }

    RIALTO_CLIENT_LOG_INFO("Shared buffer was successfully initialised");
    return true;
}

void Control::termSharedMemory()
{
    // Invalidate the stored shm so that getSharedMemoryBuffer returns nullptr
    uint8_t *shmBuffer = m_shmBuffer;
    {
        std::lock_guard<std::mutex> lock{m_shmMutex};

        m_shmBuffer = nullptr;
    }

    // Notify clients that the shared memory is about to be terminated
    // Client should finish any reading/writing from the shared buffer before returning
    {
        std::lock_guard<std::mutex> lock{m_clientVecMutex};

        for (auto it = m_clientVec.begin(); it != m_clientVec.end(); it++)
        {
            (*it)->notifyBufferTerm();
        }
    }

    // Unmap the shared memory
    {
        std::lock_guard<std::mutex> lock{m_shmMutex};

        if (-1 == m_shmFd)
        {
            RIALTO_CLIENT_LOG_WARN("Shared memory not initalised");
        }
        else
        {
            int32_t ret = munmap(shmBuffer, m_shmBufferLen);
            if (-1 == ret)
            {
                RIALTO_CLIENT_LOG_ERROR("Failed to unmap databuffer: %s", strerror(errno));
            }
            else
            {
                RIALTO_CLIENT_LOG_INFO("Shared buffer was successfully terminated");
            }

            close(m_shmFd);
            m_shmFd = -1;
            m_shmBufferLen = 0U;
        }
    }
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
