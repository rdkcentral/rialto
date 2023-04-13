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

#include "ControlServerInternal.h"
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

namespace firebolt::rialto
{
std::shared_ptr<IControlFactory> IControlFactory::createFactory()
{
    return server::IControlServerInternalFactory::createFactory();
}
} // namespace firebolt::rialto

namespace firebolt::rialto::server
{
std::shared_ptr<IControlServerInternalFactory> IControlServerInternalFactory::createFactory()
try
{
    return std::make_shared<ControlServerInternalFactory>();
}
catch (std::exception &e)
{
    RIALTO_SERVER_LOG_ERROR("ControlServerInternalFactory creation failed");
    return nullptr;
}

std::shared_ptr<IControl> ControlServerInternalFactory::createControl(std::weak_ptr<IControlClient> client) const
{
    RIALTO_SERVER_LOG_ERROR("This function can't be used by rialto server. Please use createControlServerInternal");
    return nullptr;
}

std::shared_ptr<IControlServerInternal>
ControlServerInternalFactory::createControlServerInternal(std::weak_ptr<IControlClientServerInternal> client) const
try
{
    std::shared_ptr<IControlClientServerInternal> controlClient{client.lock()};
    if (!controlClient)
    {
        RIALTO_SERVER_LOG_ERROR("ControlServerInternal creation failed - can't lock client");
        return nullptr;
    }
    return std::make_shared<ControlServerInternal>(controlClient, server::IMainThreadFactory::createFactory());
}
catch (const std::exception &e)
{
    RIALTO_SERVER_LOG_ERROR("ControlServerInternal creation failed");
    return nullptr;
}

ControlServerInternal::ControlServerInternal(const std::shared_ptr<IControlClientServerInternal> &client,
                                             const std::shared_ptr<IMainThreadFactory> &mainThreadFactory)
    : m_client{client}
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    m_mainThread = mainThreadFactory->getMainThread();
    if (!m_mainThread)
    {
        throw std::runtime_error("Failed to get the main thread");
    }
    m_mainThreadClientId = m_mainThread->registerClient();
}

ControlServerInternal::~ControlServerInternal()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    m_mainThread->unregisterClient(m_mainThreadClientId);
}

void ControlServerInternal::ack(uint32_t id)
{
    RIALTO_SERVER_LOG_WARN("Heartbeat functionality not implemented yet.");
}

void ControlServerInternal::setApplicationState(const ApplicationState &state)
{
    RIALTO_SERVER_LOG_INFO("Notify rialto clients about state changed to: %s", convertApplicationState(state));
    auto task = [&]() { m_client->notifyApplicationState(state); };
    m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
}
} // namespace firebolt::rialto::server