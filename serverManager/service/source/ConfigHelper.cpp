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

#include "ConfigHelper.h"
#include "IConfigReader.h"
#include "RialtoServerManagerLogging.h"
#include <algorithm>
#include <memory>

namespace rialto::servermanager::service
{
ConfigHelper::ConfigHelper(std::unique_ptr<IConfigReaderFactory> &&configReaderFactory,
                           const firebolt::rialto::common::ServerManagerConfig &config)
    : m_configReaderFactory{std::move(configReaderFactory)}, m_sessionServerEnvVars{config.sessionServerEnvVars},
      m_sessionServerPath{config.sessionServerPath}, m_sessionServerStartupTimeout{config.sessionServerStartupTimeout},
      m_healthcheckInterval{config.healthcheckInterval}, m_socketPermissions{config.sessionManagementSocketPermissions},
      m_numOfPreloadedServers{config.numOfPreloadedServers},
      m_numOfFailedPingsBeforeRecovery{config.numOfFailedPingsBeforeRecovery}, m_loggingLevels{}
{
    readConfigFile();
}

const std::list<std::string> &ConfigHelper::getSessionServerEnvVars() const
{
    return m_sessionServerEnvVars;
}

const std::string &ConfigHelper::getSessionServerPath() const
{
    return m_sessionServerPath;
}

std::chrono::milliseconds ConfigHelper::getSessionServerStartupTimeout() const
{
    return m_sessionServerStartupTimeout;
}

std::chrono::seconds ConfigHelper::getHealthcheckInterval() const
{
    return m_healthcheckInterval;
}

firebolt::rialto::common::SocketPermissions ConfigHelper::getSocketPermissions() const
{
    return m_socketPermissions;
}

unsigned int ConfigHelper::getNumOfPreloadedServers() const
{
    return m_numOfPreloadedServers;
}

unsigned int ConfigHelper::getNumOfFailedPingsBeforeRecovery() const
{
    return m_numOfFailedPingsBeforeRecovery;
}

const rialto::servermanager::service::LoggingLevels &ConfigHelper::getLoggingLevels() const
{
    return m_loggingLevels;
}

void ConfigHelper::readConfigFile()
{
#ifdef RIALTO_ENABLE_CONFIG_FILE
    std::shared_ptr<IConfigReader> configReader = m_configReaderFactory->createConfigReader();
    if (!configReader || !configReader->read())
    {
        RIALTO_SERVER_MANAGER_LOG_DEBUG("Config file not present");
        return;
    }

    for (const auto &envVar : configReader->getEnvironmentVariables())
    {
        // If environment variable exists in ServerManagerConfig, do not overwrite it
        if (m_sessionServerEnvVars.end() ==
            std::find(m_sessionServerEnvVars.begin(), m_sessionServerEnvVars.end(), envVar))
        {
            m_sessionServerEnvVars.push_back(envVar);
        }
    }

    if (configReader->getSessionServerPath())
        m_sessionServerPath = configReader->getSessionServerPath().value();

    if (configReader->getSessionServerStartupTimeout())
        m_sessionServerStartupTimeout = configReader->getSessionServerStartupTimeout().value();

    if (configReader->getHealthcheckInterval())
        m_healthcheckInterval = configReader->getHealthcheckInterval().value();

    if (configReader->getSocketPermissions())
        m_socketPermissions = configReader->getSocketPermissions().value();

    if (configReader->getNumOfPreloadedServers())
        m_numOfPreloadedServers = configReader->getNumOfPreloadedServers().value();

    if (configReader->getNumOfPingsBeforeRecovery())
        m_numOfFailedPingsBeforeRecovery = configReader->getNumOfPingsBeforeRecovery().value();

    if (configReader->getLoggingLevels())
        m_loggingLevels = configReader->getLoggingLevels().value();
#endif
}
} // namespace rialto::servermanager::service
