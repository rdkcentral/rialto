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

#ifndef RIALTO_SERVERMANAGER_SERVICE_CONFIG_HELPER_H_
#define RIALTO_SERVERMANAGER_SERVICE_CONFIG_HELPER_H_

#include "IConfigReaderFactory.h"
#include "LoggingLevels.h"
#include "SessionServerCommon.h"
#include <chrono>
#include <list>
#include <memory>
#include <string>

namespace rialto::servermanager::service
{
class ConfigHelper
{
public:
    ConfigHelper(std::unique_ptr<IConfigReaderFactory> &&configReaderFactory,
                 const firebolt::rialto::common::ServerManagerConfig &config);
    ~ConfigHelper() = default;

    const std::list<std::string> &getSessionServerEnvVars() const;
    const std::string &getSessionServerPath() const;
    std::chrono::milliseconds getSessionServerStartupTimeout() const;
    std::chrono::seconds getHealthcheckInterval() const;
    firebolt::rialto::common::SocketPermissions getSocketPermissions() const;
    unsigned int getNumOfPreloadedServers() const;
    unsigned int getNumOfFailedPingsBeforeRecovery() const;
    const rialto::servermanager::service::LoggingLevels &getLoggingLevels() const;

private:
    void readConfigFile();

private:
    std::unique_ptr<IConfigReaderFactory> m_configReaderFactory;
    std::list<std::string> m_sessionServerEnvVars;
    std::string m_sessionServerPath;
    std::chrono::milliseconds m_sessionServerStartupTimeout;
    std::chrono::seconds m_healthcheckInterval;
    firebolt::rialto::common::SocketPermissions m_socketPermissions;
    unsigned int m_numOfPreloadedServers;
    unsigned int m_numOfFailedPingsBeforeRecovery;
    rialto::servermanager::service::LoggingLevels m_loggingLevels;
};
} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_CONFIG_HELPER_H_
