/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "ServerProfiling.h"

#include "RialtoServerLogging.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>

namespace firebolt::rialto::server::service::profiling
{
namespace
{
constexpr const char *kEnabledEnvVar{"RIALTO_SERVER_PROFILING_ENABLED"};
constexpr const char *kScopeModeEnvVar{"RIALTO_SERVER_PROFILING_SCOPE_MODE"};
constexpr const char *kInterfacesEnvVar{"RIALTO_SERVER_PROFILING_INTERFACES"};

constexpr const char *kScopeModeAll{"all"};
constexpr const char *kScopeModeInterface{"interface"};

struct ProfileConfig
{
    bool enabled{false};
    bool allInterfaces{false};
    std::set<ProfileInterface> enabledInterfaces;
};

std::once_flag g_configInitFlag;
ProfileConfig g_profileConfig;

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::string trim(std::string value)
{
    auto isNotSpace = [](unsigned char character) { return !std::isspace(character); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), isNotSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
    return value;
}

std::string toString(ProfileInterface profileInterface)
{
    switch (profileInterface)
    {
    case ProfileInterface::MediaPipeline:
        return "IMediaPipeline";
    case ProfileInterface::MediaKeys:
        return "IMediaKeys";
    case ProfileInterface::WebAudioPlayer:
        return "IWebAudioPlayer";
    case ProfileInterface::MediaPipelineCapabilities:
        return "IMediaPipelineCapabilities";
    case ProfileInterface::MediaKeysCapabilities:
        return "IMediaKeysCapabilities";
    }

    return "UnknownInterface";
}

std::optional<ProfileInterface> toProfileInterface(const std::string &interfaceName)
{
    if (interfaceName == "IMediaPipeline")
    {
        return ProfileInterface::MediaPipeline;
    }
    if (interfaceName == "IMediaKeys")
    {
        return ProfileInterface::MediaKeys;
    }
    if (interfaceName == "IWebAudioPlayer")
    {
        return ProfileInterface::WebAudioPlayer;
    }
    if (interfaceName == "IMediaPipelineCapabilities")
    {
        return ProfileInterface::MediaPipelineCapabilities;
    }
    if (interfaceName == "IMediaKeysCapabilities")
    {
        return ProfileInterface::MediaKeysCapabilities;
    }

    return std::nullopt;
}

bool parseEnabledEnvValue(const char *enabledEnvValue)
{
    if (enabledEnvValue == nullptr)
    {
        return false;
    }

    const std::string normalized = toLower(trim(enabledEnvValue));
    return normalized == "1" || normalized == "true" || normalized == "yes";
}

std::set<ProfileInterface> parseInterfaces(const char *interfacesEnvValue)
{
    std::set<ProfileInterface> enabledInterfaces;
    if (interfacesEnvValue == nullptr)
    {
        return enabledInterfaces;
    }

    std::stringstream valueStream{interfacesEnvValue};
    std::string token;
    while (std::getline(valueStream, token, ','))
    {
        token = trim(token);
        if (token.empty())
        {
            continue;
        }

        const auto interfaceValue = toProfileInterface(token);
        if (!interfaceValue.has_value())
        {
            RIALTO_SERVER_LOG_WARN("Unsupported profiling interface '%s' ignored", token.c_str());
            continue;
        }

        enabledInterfaces.emplace(interfaceValue.value());
    }

    return enabledInterfaces;
}

void initializeProfileConfig()
{
    const char *enabledEnvValue = std::getenv(kEnabledEnvVar);
    g_profileConfig.enabled = parseEnabledEnvValue(enabledEnvValue);
    if (!g_profileConfig.enabled)
    {
        RIALTO_SERVER_LOG_DEBUG("Server profiling disabled via %s", kEnabledEnvVar);
        return;
    }

    const char *scopeModeEnvValue = std::getenv(kScopeModeEnvVar);
    const std::string scopeMode = toLower(trim(scopeModeEnvValue != nullptr ? scopeModeEnvValue : "all"));

    if (scopeMode == kScopeModeAll)
    {
        g_profileConfig.allInterfaces = true;
        RIALTO_SERVER_LOG_INFO("Server profiling enabled for all interfaces");
        return;
    }

    if (scopeMode == kScopeModeInterface)
    {
        g_profileConfig.enabledInterfaces = parseInterfaces(std::getenv(kInterfacesEnvVar));
        RIALTO_SERVER_LOG_INFO("Server profiling enabled for %d selected interfaces",
                               static_cast<int>(g_profileConfig.enabledInterfaces.size()));
        return;
    }

    RIALTO_SERVER_LOG_WARN("Unsupported profiling scope mode '%s'; profiling disabled", scopeMode.c_str());
    g_profileConfig.enabled = false;
}

const ProfileConfig &getProfileConfig()
{
    std::call_once(g_configInitFlag, initializeProfileConfig);
    return g_profileConfig;
}

} // namespace

namespace Internal
{
bool isEnabledForInterface(ProfileInterface profileInterface)
{
    const auto &config = getProfileConfig();
    if (!config.enabled)
    {
        return false;
    }

    if (config.allInterfaces)
    {
        return true;
    }

    return config.enabledInterfaces.find(profileInterface) != config.enabledInterfaces.end();
}

std::string createScopeName(ProfileInterface profileInterface, const char *operationName)
{
    std::string scopeName{"RialtoServer::"};
    scopeName += toString(profileInterface);
    scopeName += "::";
    scopeName += (operationName != nullptr) ? operationName : "unknown";
    return scopeName;
}

} // namespace Internal
} // namespace firebolt::rialto::server::service::profiling
