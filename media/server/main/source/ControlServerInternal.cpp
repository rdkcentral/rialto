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

namespace firebolt::rialto
{
IControlAccessor &IControlAccessor::instance()
{
    return server::IControlServerInternalAccessor::instance();
}
} // namespace firebolt::rialto

namespace firebolt::rialto::server
{
IControlServerInternalAccessor &IControlServerInternalAccessor::instance()
{
    static ControlServerInternalAccessor accessor;
    return accessor;
}

IControlServerInternal &ControlServerInternalAccessor::getControlServerInternal() const
{
    static ControlServerInternal controlServerInternal{server::IMainThreadFactory::createFactory()};
    return controlServerInternal;
}

IControl &ControlServerInternalAccessor::getControl() const
{
    return server::IControlServerInternalAccessor::instance().getControlServerInternal();
}

ControlServerInternal::ControlServerInternal(const std::shared_ptr<IMainThreadFactory> &mainThreadFactory)
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

bool ControlServerInternal::registerClient(IControlClient *client, ApplicationState &appState)
{
    return false;
}

bool ControlServerInternal::unregisterClient(IControlClient *client)
{
    return false;
}
} // namespace firebolt::rialto::server
