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

#include "ConfigReader.h"
#include "RialtoServerManagerLogging.h"
#include <fstream>
#include <json/json.h>

namespace rialto::servermanager::service
{
ConfigReader::ConfigReader(std::shared_ptr<firebolt::rialto::wrappers::IJsonCppWrapper> jsonWrapper,
                           std::shared_ptr<IFileReader> fileReader)
    : m_jsonWrapper(jsonWrapper), m_fileReader(fileReader)
{
}

bool ConfigReader::read()
{
    if (!m_fileReader->isOpen())
    {
        return false;
    }

    std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root;
    Json::CharReaderBuilder builder;
    if (!m_jsonWrapper->parseFromStream(builder, m_fileReader->get(), root, nullptr))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to parse config file");
        return false;
    }

    parseEnvironmentVariables(root);
    parseSessionServerPath(root);
    parseSessionServerStartupTimeout(root);
    parseHealthcheckInterval(root);
    parseSocketPermissions(root);
    parseSocketOwner(root);
    parseSocketGroup(root);
    parseNumOfPreloadedServers(root);
    parseLogLevel(root);
    parseNumOfPingsBeforeRecovery(root);

    return true;
}

void ConfigReader::parseEnvironmentVariables(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    if (root->isMember("environment_variables") && root->at("environment_variables")->isArray())
    {
        std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> envVarsJson = root->at("environment_variables");
        Json::ArrayIndex size = envVarsJson->size();
        for (Json::ArrayIndex index = 0; index < size; ++index)
        {
            if (envVarsJson->at(index)->isString())
            {
                m_envVars.emplace_back(envVarsJson->at(index)->asString());
            }
        }
    }
}

void ConfigReader::parseSessionServerPath(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    if (root->isMember("session_server_path") && root->at("session_server_path")->isString())
    {
        m_sessionServerPath = root->at("session_server_path")->asString();
    }
}

void ConfigReader::parseSessionServerStartupTimeout(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    if (root->isMember("startup_timeout_ms") && root->at("startup_timeout_ms")->isUInt())
    {
        m_sessionServerStartupTimeout = std::chrono::milliseconds(root->at("startup_timeout_ms")->asUInt());
    }
}

void ConfigReader::parseHealthcheckInterval(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    if (root->isMember("healthcheck_interval_s") && root->at("healthcheck_interval_s")->isUInt())
    {
        m_healthcheckInterval = std::chrono::seconds(root->at("healthcheck_interval_s")->asUInt());
    }
}

void ConfigReader::parseSocketPermissions(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    if (root->isMember("socket_permissions") && root->at("socket_permissions")->isUInt())
    {
        unsigned permissions = root->at("socket_permissions")->asUInt();

        firebolt::rialto::common::SocketPermissions socketPermissions;
        socketPermissions.ownerPermissions = (permissions / 100) % 10;
        socketPermissions.groupPermissions = (permissions / 10) % 10;
        socketPermissions.otherPermissions = (permissions) % 10;
        m_socketPermissions = socketPermissions;
    }
}

void ConfigReader::parseSocketOwner(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    const char *kFieldString = "socket_owner";
    if (root->isMember(kFieldString) && root->at(kFieldString)->isString())
    {
        m_socketOwner = root->at(kFieldString)->asString();
    }
}

void ConfigReader::parseSocketGroup(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    const char *kFieldString = "socket_group";
    if (root->isMember(kFieldString) && root->at(kFieldString)->isString())
    {
        m_socketGroup = root->at(kFieldString)->asString();
    }
}

void ConfigReader::parseNumOfPreloadedServers(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    if (root->isMember("num_of_preloaded_servers") && root->at("num_of_preloaded_servers")->isUInt())
    {
        m_numOfPreloadedServers = root->at("num_of_preloaded_servers")->asUInt();
    }
}

void ConfigReader::parseLogLevel(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    if (root->isMember("log_level") && root->at("log_level")->isUInt())
    {
        unsigned loggingLevel = root->at("log_level")->asUInt();
        rialto::servermanager::service::LoggingLevel newLevel{rialto::servermanager::service::LoggingLevel::UNCHANGED};

        switch (loggingLevel)
        {
        case 0:
            newLevel = rialto::servermanager::service::LoggingLevel::FATAL;
            break;
        case 1:
            newLevel = rialto::servermanager::service::LoggingLevel::ERROR;
            break;
        case 2:
            newLevel = rialto::servermanager::service::LoggingLevel::WARNING;
            break;
        case 3:
            newLevel = rialto::servermanager::service::LoggingLevel::MILESTONE;
            break;
        case 4:
            newLevel = rialto::servermanager::service::LoggingLevel::INFO;
            break;
        case 5:
            newLevel = rialto::servermanager::service::LoggingLevel::DEBUG;
            break;
        default:
            newLevel = rialto::servermanager::service::LoggingLevel::UNCHANGED;
            break;
        }
        m_loggingLevels = {newLevel, newLevel, newLevel, newLevel, newLevel, newLevel};
    }
}

void ConfigReader::parseNumOfPingsBeforeRecovery(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    if (root->isMember("num_of_pings_before_recovery") && root->at("num_of_pings_before_recovery")->isUInt())
    {
        m_numOfPingsBeforeRecovery = root->at("num_of_pings_before_recovery")->asUInt();
    }
}

std::list<std::string> ConfigReader::getEnvironmentVariables()
{
    return m_envVars;
}

std::optional<std::string> ConfigReader::getSessionServerPath()
{
    return m_sessionServerPath;
}

std::optional<std::chrono::milliseconds> ConfigReader::getSessionServerStartupTimeout()
{
    return m_sessionServerStartupTimeout;
}

std::optional<std::chrono::seconds> ConfigReader::getHealthcheckInterval()
{
    return m_healthcheckInterval;
}

std::optional<firebolt::rialto::common::SocketPermissions> ConfigReader::getSocketPermissions()
{
    return m_socketPermissions;
}

std::optional<std::string> ConfigReader::getSocketOwner()
{
    return m_socketOwner;
}

std::optional<std::string> ConfigReader::getSocketGroup()
{
    return m_socketGroup;
}

std::optional<unsigned int> ConfigReader::getNumOfPreloadedServers()
{
    return m_numOfPreloadedServers;
}

std::optional<rialto::servermanager::service::LoggingLevels> ConfigReader::getLoggingLevels()
{
    return m_loggingLevels;
}

std::optional<unsigned int> ConfigReader::getNumOfPingsBeforeRecovery()
{
    return m_numOfPingsBeforeRecovery;
}

} // namespace rialto::servermanager::service
