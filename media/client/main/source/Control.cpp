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
    return std::make_shared<Control>(client, ISharedMemoryManagerAccessor::instance().getSharedMemoryManager());
}
catch (const std::exception &e)
{
    RIALTO_CLIENT_LOG_ERROR("Failed to create the rialto control, reason: %s", e.what());
    return nullptr;
}

Control::Control(std::weak_ptr<IControlClient> client, ISharedMemoryManager &sharedMemoryManager)
    : m_client(client), m_sharedMemoryManager(sharedMemoryManager)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    std::shared_ptr<IControlClient> lockedClient = m_client.lock();
    if (lockedClient)
    {
        m_sharedMemoryManager.registerClient(lockedClient.get());
    }
    else
    {
        RIALTO_CLIENT_LOG_WARN("Unable to register client");
    }
}

Control::~Control()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    std::shared_ptr<IControlClient> client = m_client.lock();
    if (client)
    {
        m_sharedMemoryManager.unregisterClient(client.get());
    }
    else
    {
        RIALTO_CLIENT_LOG_WARN("Unable to unregister client");
    }
}
}; // namespace firebolt::rialto::client
