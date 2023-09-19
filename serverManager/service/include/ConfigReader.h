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

#ifndef RIALTO_SERVERMANAGER_SERVICE_CONFIG_READER_H_
#define RIALTO_SERVERMANAGER_SERVICE_CONFIG_READER_H_
#include "IConfigReader.h"
#include "IJsonCppWrapper.h"
#include <IFileReader.h>
#include <chrono>
#include <list>
#include <memory>
#include <optional>
#include <string>

namespace rialto::servermanager::service
{
class ConfigReader : public IConfigReader
{
public:
    ConfigReader(std::shared_ptr<IJsonCppWrapper> jsonWrapper, std::shared_ptr<IFileReader> fileReader);
    bool read() override;

    std::list<std::string> getEnvironmentVariables() override;
    std::optional<std::string> getSessionServerPath() override;
    std::optional<std::chrono::milliseconds> getSessionServerStartupTimeout() override;
    std::optional<std::chrono::seconds> getHealthcheckInterval() override;
    std::optional<unsigned int> getSocketPermissions() override;
    std::optional<unsigned int> getNumOfPreloadedServers() override;

private:
    std::shared_ptr<IJsonCppWrapper> m_jsonWrapper;
    std::shared_ptr<IFileReader> m_fileReader;

    std::list<std::string> m_envVars;
    std::optional<std::string> m_sessionServerPath;
    std::optional<std::chrono::milliseconds> m_sessionServerStartupTimeout;
    std::optional<std::chrono::seconds> m_healthcheckInterval;
    std::optional<unsigned int> m_socketPermissions;
    std::optional<unsigned int> m_numOfPreloadedServers;
};

} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_CONFIG_READER_H_
