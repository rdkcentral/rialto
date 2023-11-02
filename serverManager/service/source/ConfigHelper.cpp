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
#include <list>
#include <map>
#include <memory>

namespace
{
std::map<std::string, std::string> convertToMap(const std::list<std::string> &envVariablesList)
{
    std::map<std::string, std::string> result{};
    for (const auto &var : envVariablesList)
    {
        const auto kSplitPos{var.find("=")};
        if (std::string::npos == kSplitPos || kSplitPos + 1 >= var.size())
        {
            continue;
        }
        std::string name = var.substr(0, kSplitPos);
        std::string value = var.substr(kSplitPos + 1);
        result.emplace(name, value);
    }
    return result;
}

std::list<std::string> convertToList(const std::map<std::string, std::string> &envVariablesMap)
{
    std::list<std::string> result{};
    for (const auto &[name, value] : envVariablesMap)
    {
        result.emplace_back(name + "=" + value);
    }
    return result;
}
} // namespace

namespace rialto::servermanager::service
{
ConfigHelper::ConfigHelper(std::unique_ptr<IConfigReaderFactory> &&configReaderFactory,
                           const firebolt::rialto::common::ServerManagerConfig &config)
    : m_configReaderFactory{std::move(configReaderFactory)}, m_sessionServerEnvVars{convertToMap(
                                                                 config.sessionServerEnvVars)},
      m_sessionServerPath{config.sessionServerPath}, m_sessionServerStartupTimeout{config.sessionServerStartupTimeout},
      m_healthcheckInterval{config.healthcheckInterval}, m_socketPermissions{config.sessionManagementSocketPermissions},
      m_numOfPreloadedServers{config.numOfPreloadedServers},
      m_numOfFailedPingsBeforeRecovery{config.numOfFailedPingsBeforeRecovery}, m_loggingLevels{}
{
    readConfigFile();
}

std::list<std::string> ConfigHelper::getSessionServerEnvVars() const
{
    return convertToList(m_sessionServerEnvVars);
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
    if (!m_configReaderFactory)
    {
        RIALTO_SERVER_MANAGER_LOG_DEBUG("Config reader factory not present");
        return;
    }
    std::shared_ptr<IConfigReader> configReader = m_configReaderFactory->createConfigReader();
    if (!configReader || !configReader->read())
    {
        RIALTO_SERVER_MANAGER_LOG_DEBUG("Config file not present");
        return;
    }

    std::map<std::string, std::string> envVariables{convertToMap(configReader->getEnvironmentVariables())};
    for (const auto &[name, value] : envVariables)
    {
        // If environment variable exists in ServerManagerConfig, do not overwrite it
        if (m_sessionServerEnvVars.end() == m_sessionServerEnvVars.find(name))
        {
            m_sessionServerEnvVars.emplace(name, value);
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

    if (configReader->getSocketOwner())
        m_socketPermissions.owner = configReader->getSocketOwner().value();

    if (configReader->getSocketGroup())
        m_socketPermissions.group = configReader->getSocketGroup().value();

    if (configReader->getNumOfPreloadedServers())
        m_numOfPreloadedServers = configReader->getNumOfPreloadedServers().value();

    if (configReader->getNumOfPingsBeforeRecovery())
        m_numOfFailedPingsBeforeRecovery = configReader->getNumOfPingsBeforeRecovery().value();

    if (configReader->getLoggingLevels())
        m_loggingLevels = configReader->getLoggingLevels().value();
#endif
}
} // namespace rialto::servermanager::service
