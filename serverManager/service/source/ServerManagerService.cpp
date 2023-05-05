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

#include "ServerManagerService.h"
#include "ISessionServerAppManager.h"
#include "RialtoServerManagerLogging.h"

namespace
{
rialto::servermanager::service::ILogHandler::Level convertLevel(const RIALTO_DEBUG_LEVEL &level)
{
    switch (level)
    {
    case RIALTO_DEBUG_LEVEL_FATAL:
        return rialto::servermanager::service::ILogHandler::Level::Fatal;
    case RIALTO_DEBUG_LEVEL_ERROR:
        return rialto::servermanager::service::ILogHandler::Level::Error;
    case RIALTO_DEBUG_LEVEL_WARNING:
        return rialto::servermanager::service::ILogHandler::Level::Warning;
    case RIALTO_DEBUG_LEVEL_MILESTONE:
        return rialto::servermanager::service::ILogHandler::Level::Milestone;
    case RIALTO_DEBUG_LEVEL_INFO:
        return rialto::servermanager::service::ILogHandler::Level::Info;
    case RIALTO_DEBUG_LEVEL_DEBUG:
        return rialto::servermanager::service::ILogHandler::Level::Debug;
    case RIALTO_DEBUG_LEVEL_EXTERNAL:
        return rialto::servermanager::service::ILogHandler::Level::External;
    case RIALTO_DEBUG_LEVEL_DEFAULT:
        return rialto::servermanager::service::ILogHandler::Level::Milestone;
    }
    return rialto::servermanager::service::ILogHandler::Level::Debug;
}
} // namespace

namespace rialto::servermanager::service
{
ServerManagerService::ServerManagerService(std::unique_ptr<IServiceContext> &&context, unsigned numOfPreloadedServers)
    : m_kContext{std::move(context)}
{
    RIALTO_SERVER_MANAGER_LOG_MIL("RialtoServerManager is starting...");
    m_kContext->getSessionServerAppManager().preloadSessionServers(numOfPreloadedServers);
}

ServerManagerService::~ServerManagerService()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager is closing...");
}

bool ServerManagerService::initiateApplication(const std::string &appId,
                                               const firebolt::rialto::common::SessionServerState &state,
                                               const firebolt::rialto::common::AppConfig &appConfig)
{
    return m_kContext->getSessionServerAppManager().initiateApplication(appId, state, appConfig);
}

bool ServerManagerService::changeSessionServerState(const std::string &appId,
                                                    const firebolt::rialto::common::SessionServerState &state)
{
    return m_kContext->getSessionServerAppManager().setSessionServerState(appId, state);
}

std::string ServerManagerService::getAppConnectionInfo(const std::string &appId) const
{
    return m_kContext->getSessionServerAppManager().getAppConnectionInfo(appId);
}

bool ServerManagerService::setLogLevels(const LoggingLevels &logLevels) const
{
    return m_kContext->getSessionServerAppManager().setLogLevels(logLevels);
}

bool ServerManagerService::registerLogHandler(const std::shared_ptr<ILogHandler> &handler)
{
    using namespace std::placeholders;
    if (!handler)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Cannot set custom log handler - ptr is null!");
        return false;
    }
    m_logHandler = handler;
    return firebolt::rialto::logging::RIALTO_LOGGING_STATUS_OK ==
           firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_SERVER_MANAGER,
                                                    std::bind(&ServerManagerService::forwardLog, this, _1, _2, _3, _4,
                                                              _5, _6));
}

void ServerManagerService::forwardLog(RIALTO_DEBUG_LEVEL level, const char *file, int line, const char *function,
                                      const char *message, std::size_t messageLen) const
{
    if (!m_logHandler)
    {
        return;
    }
    m_logHandler->log(convertLevel(level), std::string(file), line, std::string(function), std::string(message));
}
} // namespace rialto::servermanager::service
