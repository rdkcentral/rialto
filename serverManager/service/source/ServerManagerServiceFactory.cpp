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

#include "ServerManagerServiceFactory.h"
#include "RialtoServerManagerLogging.h"
#include "ServerManagerService.h"
#include "ServiceContext.h"
#include <memory>

#ifdef RIALTO_ENABLE_CONFIG_FILE
#include "ConfigReaderFactory.h"
#endif

namespace
{
unsigned int convertSocketPermissions(firebolt::rialto::common::SocketPermissions permissions) // copy param intentionally
{
    constexpr unsigned int kDefaultPermissions{0666};
    if (permissions.ownerPermissions > 7 || permissions.groupPermissions > 7 || permissions.otherPermissions > 7)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("One of permissions values is out of bounds. Default permissions will be used");
        return kDefaultPermissions;
    }
    permissions.ownerPermissions <<= 6;
    permissions.groupPermissions <<= 3;
    return (permissions.ownerPermissions | permissions.groupPermissions | permissions.otherPermissions);
}
} // namespace

namespace rialto::servermanager::service
{
std::unique_ptr<IServerManagerService> create(const std::shared_ptr<IStateObserver> &stateObserver)
{
    firebolt::rialto::common::ServerManagerConfig config;
    return create(stateObserver, config);
}

std::unique_ptr<IServerManagerService> create(const std::shared_ptr<IStateObserver> &stateObserver,
                                              const firebolt::rialto::common::ServerManagerConfig &config)
{
    std::list<std::string> sessionServerEnvVars = config.sessionServerEnvVars;
    std::string sessionServerPath = config.sessionServerPath;
    std::chrono::milliseconds sessionServerStartupTimeout = config.sessionServerStartupTimeout;
    std::chrono::seconds healthcheckInterval = config.healthcheckInterval;
    firebolt::rialto::common::SocketPermissions socketPermissions = config.sessionManagementSocketPermissions;
    std::string socketOwner = config.sessionManagementSocketPermissions.owner;
    std::string socketGroup = config.sessionManagementSocketPermissions.group;
    unsigned int numOfPreloadedServers = config.numOfPreloadedServers;
    unsigned int numOfFailedPingsBeforeRecovery = config.numOfFailedPingsBeforeRecovery;

#ifdef RIALTO_ENABLE_CONFIG_FILE
    std::unique_ptr<IConfigReaderFactory> configReaderFactory = std::make_unique<ConfigReaderFactory>();
    std::shared_ptr<IConfigReader> configReader = configReaderFactory->createConfigReader();
    configReader->read();

    for (const auto &envVar : configReader->getEnvironmentVariables())
    {
        // If environment variable exists in ServerManagerConfig, do not overwrite it
        if (sessionServerEnvVars.end() == std::find(sessionServerEnvVars.begin(), sessionServerEnvVars.end(), envVar))
        {
            sessionServerEnvVars.push_back(envVar);
        }
    }

    if (configReader->getSessionServerPath())
        sessionServerPath = configReader->getSessionServerPath().value();

    if (configReader->getSessionServerStartupTimeout())
        sessionServerStartupTimeout = configReader->getSessionServerStartupTimeout().value();

    if (configReader->getHealthcheckInterval())
        healthcheckInterval = configReader->getHealthcheckInterval().value();

    if (configReader->getSocketPermissions())
        socketPermissions = configReader->getSocketPermissions().value();

    if (configReader->getSocketOwner())
        socketOwner = configReader->getSocketOwner().value();

    if (configReader->getSocketGroup())
        socketGroup = configReader->getSocketGroup().value();

    if (configReader->getNumOfPreloadedServers())
        numOfPreloadedServers = configReader->getNumOfPreloadedServers().value();

    if (configReader->getNumOfPingsBeforeRecovery())
        numOfFailedPingsBeforeRecovery = configReader->getNumOfPingsBeforeRecovery().value();
#endif

    std::unique_ptr<IServerManagerService> service =
        std::make_unique<ServerManagerService>(std::make_unique<ServiceContext>(stateObserver, sessionServerEnvVars,
                                                                                sessionServerPath,
                                                                                sessionServerStartupTimeout,
                                                                                healthcheckInterval,
                                                                                numOfFailedPingsBeforeRecovery,
                                                                                convertSocketPermissions(socketPermissions),
                                                                                socketOwner, socketGroup),
                                               numOfPreloadedServers);

#ifdef RIALTO_ENABLE_CONFIG_FILE
    if (configReader->getLoggingLevels())
        service->setLogLevels(configReader->getLoggingLevels().value());
#endif
    return service;
}
} // namespace rialto::servermanager::service
