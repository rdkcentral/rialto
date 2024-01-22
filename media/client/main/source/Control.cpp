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

namespace
{
firebolt::rialto::IClientLogHandler::Level convertLevel(const RIALTO_DEBUG_LEVEL &level)
{
    switch (level)
    {
    case RIALTO_DEBUG_LEVEL_FATAL:
        return firebolt::rialto::IClientLogHandler::Level::Fatal;
    case RIALTO_DEBUG_LEVEL_ERROR:
        return firebolt::rialto::IClientLogHandler::Level::Error;
    case RIALTO_DEBUG_LEVEL_WARNING:
        return firebolt::rialto::IClientLogHandler::Level::Warning;
    case RIALTO_DEBUG_LEVEL_MILESTONE:
        return firebolt::rialto::IClientLogHandler::Level::Milestone;
    case RIALTO_DEBUG_LEVEL_INFO:
        return firebolt::rialto::IClientLogHandler::Level::Info;
    case RIALTO_DEBUG_LEVEL_DEBUG:
        return firebolt::rialto::IClientLogHandler::Level::Debug;
    case RIALTO_DEBUG_LEVEL_EXTERNAL:
        return firebolt::rialto::IClientLogHandler::Level::External;
    case RIALTO_DEBUG_LEVEL_DEFAULT:
        return firebolt::rialto::IClientLogHandler::Level::Milestone;
    }
    return firebolt::rialto::IClientLogHandler::Level::Debug;
}
} // namespace

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

std::shared_ptr<IControl> ControlFactory::createControl() const
try
{
    return std::make_shared<Control>(IClientControllerAccessor::instance().getClientController());
}
catch (const std::exception &e)
{
    RIALTO_CLIENT_LOG_ERROR("Failed to create the rialto control, reason: %s", e.what());
    return nullptr;
}

Control::Control(IClientController &clientController) : m_clientController(clientController)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
}

Control::~Control()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    for (const auto &client : m_clients)
    {
        m_clientController.unregisterClient(client.get());
    }
    if (m_logHandler)
        cancelLogHandler();
}

bool Control::registerClient(std::weak_ptr<IControlClient> client, ApplicationState &appState)
{
    std::shared_ptr<IControlClient> lockedClient = client.lock();
    if (lockedClient && m_clientController.registerClient(lockedClient.get(), appState))
    {
        m_clients.push_back(lockedClient);
        return true;
    }
    RIALTO_CLIENT_LOG_WARN("Unable to register client");
    return false;
}

bool Control::registerLogHandler(std::shared_ptr<IClientLogHandler> &handler, bool ignoreLogLevels)
{
    m_logHandler = handler;

    if (firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_CLIENT,
                                                 std::bind(&Control::forwardLog, this, RIALTO_COMPONENT_CLIENT,
                                                           std::placeholders::_1, std::placeholders::_2,
                                                           std::placeholders::_3, std::placeholders::_4,
                                                           std::placeholders::_5, std::placeholders::_6),
                                                 ignoreLogLevels) != firebolt::rialto::logging::RIALTO_LOGGING_STATUS_OK ||

        firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_IPC,
                                                 std::bind(&Control::forwardLog, this, RIALTO_COMPONENT_IPC,
                                                           std::placeholders::_1, std::placeholders::_2,
                                                           std::placeholders::_3, std::placeholders::_4,
                                                           std::placeholders::_5, std::placeholders::_6),
                                                 ignoreLogLevels) != firebolt::rialto::logging::RIALTO_LOGGING_STATUS_OK ||

        firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_COMMON,
                                                 std::bind(&Control::forwardLog, this, RIALTO_COMPONENT_COMMON,
                                                           std::placeholders::_1, std::placeholders::_2,
                                                           std::placeholders::_3, std::placeholders::_4,
                                                           std::placeholders::_5, std::placeholders::_6),
                                                 ignoreLogLevels) != firebolt::rialto::logging::RIALTO_LOGGING_STATUS_OK)
    {
        RIALTO_CLIENT_LOG_WARN("Unable to register log handler");
        cancelLogHandler();
        return false;
    }
    return true;
}

void Control::cancelLogHandler()
{
    firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_CLIENT, nullptr, false);
    firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_IPC, nullptr, false);
    firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_COMMON, nullptr, false);
    m_logHandler = nullptr;
}

void Control::forwardLog(RIALTO_COMPONENT component, RIALTO_DEBUG_LEVEL level, const char *file, int line,
                         const char *function, const char *message, std::size_t messageLen) const
{
    if (!m_logHandler)
        return;
    m_logHandler->log(convertLevel(level), std::string(file), line, std::string(function), std::string(message));
}
}; // namespace firebolt::rialto::client
