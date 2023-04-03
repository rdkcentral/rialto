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

namespace rialto::servermanager::service
{
ServerManagerService::ServerManagerService(std::unique_ptr<IServiceContext> &&context) : m_kContext{std::move(context)}
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager is starting...");
}

ServerManagerService::~ServerManagerService()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("RialtoServerManager is closing...");

#ifdef RIALTO_SERVER_LOG_FATAL_ENABLED
    RIALTO_SERVER_MANAGER_LOG_FATAL("TEST_FATAL");
#endif
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
} // namespace rialto::servermanager::service
