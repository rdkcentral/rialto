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
#include <json/json.h>
#include <fstream>

namespace rialto::servermanager::service
{
    ConfigReader::ConfigReader(const std::string &filePath, std::unique_ptr<IJsonCppWrapper> &&jsonWrapper, std::unique_ptr<IFileReader> &&fileReader)
    : m_jsonWrapper(std::move(jsonWrapper)), m_fileReader(std::move(fileReader)), m_filePath(filePath)
    {

    }

    bool ConfigReader::read()
    {
        std::ifstream jsonFile(m_filePath.c_str());
        if (!m_fileReader->isOpen())
        {
            RIALTO_SERVER_MANAGER_LOG_WARN("Could not open '%s' config file", m_filePath.c_str());
            return false;
        }
        std::unique_ptr<IJsonValueWrapper> root;
        Json::CharReaderBuilder builder;
        if (!m_jsonWrapper->parseFromStream(builder, jsonFile, root, nullptr))
        {
            RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to parse '%s' config file", m_filePath.c_str());
            return false;
        }

        if (root->isMember("ENVIRONMENT_VARIABLES") && (*root)["ENVIRONMENT_VARIABLES"]->isArray())
        {
            std::unique_ptr<IJsonValueWrapper> envVarsJson = (*root)["ENVIRONMENT_VARIABLES"];
            Json::ArrayIndex size = envVarsJson->size();
            for (Json::ArrayIndex index = 0; index < size; ++index)
            {
                if((*envVarsJson)[index]->isString())
                {
                    m_envVars.emplace_back((*envVarsJson)[index]->asString());
                }
            }
        }

        if (root->isMember("SESSION_SERVER_PATH") && (*root)["SESSION_SERVER_PATH"]->isString())
        {
            m_sessionServerPath = (*root)["SESSION_SERVER_PATH"]->asString();
        }

        if (root->isMember("STARTUP_TIMEOUT_MS") && (*root)["STARTUP_TIMEOUT_MS"]->isUInt())
        {
            m_sessionServerStartupTimeout = std::optional<std::chrono::milliseconds>((*root)["STARTUP_TIMEOUT_MS"]->asUInt());
        }

        if (root->isMember("HEALTHCHECK_INTERVAL") && (*root)["HEALTHCHECK_INTERVAL"]->isUInt())
        {
            m_healthcheckInterval = std::optional<std::chrono::seconds>((*root)["HEALTHCHECK_INTERVAL"]->asUInt());
        }

        if (root->isMember("SOCKET_PERMISSIONS") && (*root)["SOCKET_PERMISSIONS"]->isUInt())
        {
            m_socketPermissions = (*root)["SOCKET_PERMISSIONS"]->asUInt();
        }

        if (root->isMember("NUM_OF_PRELOADED_SERVERS") && (*root)["NUM_OF_PRELOADED_SERVERS"]->isUInt())
        {
            m_numOfPreloadedServers = (*root)["NUM_OF_PRELOADED_SERVERS"]->asUInt();
        }

        return true;
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

    std::optional<unsigned int> ConfigReader::getSocketPermissions()
    {
        return m_socketPermissions;
    }

    std::optional<unsigned int> ConfigReader::getNumOfPreloadedServers()
    {
        return m_numOfPreloadedServers;
    }
} // namespace rialto::servermanager::service
