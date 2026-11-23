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
#include <algorithm>
#include <fstream>
#include <json/json.h>
#include <set>

namespace
{
constexpr const char *kProfilingSectionKey{"profiling"};
constexpr const char *kProfilingEnabledKey{"enabled"};
constexpr const char *kProfilingScopeModeKey{"scopeMode"};
constexpr const char *kProfilingInterfacesKey{"interfaces"};

constexpr const char *kProfilingEnvEnabled{"RIALTO_SERVER_PROFILING_ENABLED"};
constexpr const char *kProfilingEnvScopeMode{"RIALTO_SERVER_PROFILING_SCOPE_MODE"};
constexpr const char *kProfilingEnvInterfaces{"RIALTO_SERVER_PROFILING_INTERFACES"};

const std::set<std::string> kSupportedProfilingInterfaces{"IMediaPipeline", "IMediaKeys", "IWebAudioPlayer",
                                                           "IMediaPipelineCapabilities", "IMediaKeysCapabilities"};

bool isSupportedScopeMode(const std::string &scopeMode)
{
    return scopeMode == "all" || scopeMode == "interface";
}
} // namespace

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
    parseExtraEnvVariables(root);
    parseSessionServerPath(root);
    parseSessionServerStartupTimeout(root);
    parseHealthcheckInterval(root);
    parseSocketPermissions(root);
    parseSocketOwner(root);
    parseSocketGroup(root);
    parseNumOfPreloadedServers(root);
    parseLogLevel(root);
    parseNumOfPingsBeforeRecovery(root);
    parseProfiling(root);

    return true;
}

void ConfigReader::parseEnvironmentVariables(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    m_envVars = getListOfStrings(root, "environmentVariables");
}

void ConfigReader::parseExtraEnvVariables(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    m_extraEnvVars = getListOfStrings(root, "extraEnvVariables");
}

void ConfigReader::parseSessionServerPath(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    m_sessionServerPath = getString(root, "sessionServerPath");
}

void ConfigReader::parseSessionServerStartupTimeout(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    auto timeout{getUInt(root, "startupTimeoutMs")};
    if (timeout.has_value())
    {
        m_sessionServerStartupTimeout = std::chrono::milliseconds{timeout.value()};
    }
}

void ConfigReader::parseHealthcheckInterval(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    auto interval{getUInt(root, "healthcheckIntervalInSeconds")};
    if (interval.has_value())
    {
        m_healthcheckInterval = std::chrono::seconds{*interval};
    }
}

void ConfigReader::parseSocketPermissions(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    auto permissions{getUInt(root, "socketPermissions")};
    if (permissions.has_value())
    {
        firebolt::rialto::common::SocketPermissions socketPermissions;
        socketPermissions.ownerPermissions = (*permissions / 100) % 10;
        socketPermissions.groupPermissions = (*permissions / 10) % 10;
        socketPermissions.otherPermissions = (*permissions) % 10;
        m_socketPermissions = socketPermissions;
    }
}

void ConfigReader::parseSocketOwner(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    m_socketOwner = getString(root, "socketOwner");
}

void ConfigReader::parseSocketGroup(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    m_socketGroup = getString(root, "socketGroup");
}

void ConfigReader::parseNumOfPreloadedServers(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    m_numOfPreloadedServers = getUInt(root, "numOfPreloadedServers");
}

void ConfigReader::parseLogLevel(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    std::optional<unsigned> loggingLevel{getUInt(root, "logLevel")};

    if (loggingLevel)
    {
        rialto::servermanager::service::LoggingLevel newLevel{rialto::servermanager::service::LoggingLevel::UNCHANGED};

        switch (*loggingLevel)
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
    m_numOfPingsBeforeRecovery = getUInt(root, "numOfPingsBeforeRecovery");
}

void ConfigReader::parseProfiling(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root)
{
    if (!root->isMember(kProfilingSectionKey))
    {
        return;
    }

    const auto profiling = root->at(kProfilingSectionKey);
    if (!profiling)
    {
        return;
    }

    if (const auto enabled = getBool(profiling, kProfilingEnabledKey); enabled.has_value())
    {
        m_extraEnvVars.emplace_back(std::string{kProfilingEnvEnabled} + "=" + (enabled.value() ? "1" : "0"));
    }

    if (const auto scopeMode = getString(profiling, kProfilingScopeModeKey); scopeMode.has_value())
    {
        if (!isSupportedScopeMode(scopeMode.value()))
        {
            RIALTO_SERVER_MANAGER_LOG_WARN("Unsupported profiling scope mode '%s' ignored", scopeMode->c_str());
        }
        else
        {
            m_extraEnvVars.emplace_back(std::string{kProfilingEnvScopeMode} + "=" + scopeMode.value());
        }
    }

    if (!profiling->isMember(kProfilingInterfacesKey))
    {
        return;
    }

    const auto interfacesValue = profiling->at(kProfilingInterfacesKey);
    if (!interfacesValue || !interfacesValue->isArray())
    {
        return;
    }

    const auto allInterfaces = getListOfStrings(interfacesValue);
    std::list<std::string> supportedInterfaces;
    for (const auto &interfaceName : allInterfaces)
    {
        if (kSupportedProfilingInterfaces.find(interfaceName) == kSupportedProfilingInterfaces.end())
        {
            RIALTO_SERVER_MANAGER_LOG_WARN("Unsupported profiling interface '%s' ignored", interfaceName.c_str());
            continue;
        }
        supportedInterfaces.emplace_back(interfaceName);
    }

    if (!supportedInterfaces.empty())
    {
        m_extraEnvVars.emplace_back(std::string{kProfilingEnvInterfaces} + "=" + joinList(supportedInterfaces, ","));
    }
}

std::list<std::string> ConfigReader::getEnvironmentVariables()
{
    return m_envVars;
}

std::list<std::string> ConfigReader::getExtraEnvVariables()
{
    return m_extraEnvVars;
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

std::list<std::string> ConfigReader::getListOfStrings(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root,
                                                      const std::string &valueName) const
{
    std::list<std::string> result;
    if (root->isMember(valueName) && root->at(valueName)->isArray())
    {
        std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> envVarsJson = root->at(valueName);
        Json::ArrayIndex size = envVarsJson->size();
        for (Json::ArrayIndex index = 0; index < size; ++index)
        {
            if (envVarsJson->at(index)->isString())
            {
                result.emplace_back(envVarsJson->at(index)->asString());
            }
        }
    }
    return result;
}

std::list<std::string> ConfigReader::getListOfStrings(
    std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root) const
{
    std::list<std::string> result;
    if (!root || !root->isArray())
    {
        return result;
    }

    const Json::ArrayIndex size = root->size();
    for (Json::ArrayIndex index = 0; index < size; ++index)
    {
        const auto value = root->at(index);
        if (value && value->isString())
        {
            result.emplace_back(value->asString());
        }
    }
    return result;
}

std::optional<std::string> ConfigReader::getString(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root,
                                                   const std::string &valueName) const
{
    if (root->isMember(valueName) && root->at(valueName)->isString())
    {
        return root->at(valueName)->asString();
    }
    return std::nullopt;
}

std::optional<std::string> ConfigReader::getString(
    std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> value) const
{
    if (value && value->isString())
    {
        return value->asString();
    }
    return std::nullopt;
}

std::optional<unsigned int> ConfigReader::getUInt(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root,
                                                  const std::string &valueName) const
{
    if (root->isMember(valueName) && root->at(valueName)->isUInt())
    {
        return root->at(valueName)->asUInt();
    }
    return std::nullopt;
}

std::optional<bool> ConfigReader::getBool(std::shared_ptr<firebolt::rialto::wrappers::IJsonValueWrapper> root,
                                          const std::string &valueName) const
{
    if (root->isMember(valueName) && root->at(valueName)->isBool())
    {
        return root->at(valueName)->asBool();
    }
    return std::nullopt;
}

std::string ConfigReader::joinList(const std::list<std::string> &values, const std::string &delimiter) const
{
    std::string result;
    for (auto it = values.begin(); it != values.end(); ++it)
    {
        if (!result.empty())
        {
            result += delimiter;
        }
        result += *it;
    }
    return result;
}

} // namespace rialto::servermanager::service
