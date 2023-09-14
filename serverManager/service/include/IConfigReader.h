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

#ifndef RIALTO_SERVERMANAGER_SERVICE_ICONFIG_READER_H_
#define RIALTO_SERVERMANAGER_SERVICE_ICONFIG_READER_H_
#include <chrono>
#include <string>
#include <optional>
#include <list>
#include <string>

namespace rialto::servermanager::service
{
class IConfigReader
{
public:
    virtual ~IConfigReader() = default;
    virtual bool read() = 0;

    virtual std::list<std::string> getEnvironmentVariables() = 0;
    virtual std::optional<std::string> getSessionServerPath() = 0;
    virtual std::optional<std::chrono::milliseconds> getSessionServerStartupTimeout() = 0;
    virtual std::optional<std::chrono::seconds> getHealthcheckInterval() = 0;
    virtual std::optional<unsigned int> getSocketPermissions() = 0;
    virtual std::optional<unsigned int> getNumOfPreloadedServers() = 0;
};

} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_ICONFIG_READER_H_
