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

#ifndef RIALTO_SERVERMANAGER_SERVICE_CONFIG_READER_MOCK_H_
#define RIALTO_SERVERMANAGER_SERVICE_CONFIG_READER_MOCK_H_

#include "IConfigReader.h"

namespace rialto::servermanager::service
{
class ConfigReaderMock : public IConfigReader
{
public:
    MOCK_METHOD(bool, read, (), (override));
    MOCK_METHOD(std::list<std::string>, getEnvironmentVariables, (), (override));
    MOCK_METHOD(std::optional<std::string>, getSessionServerPath, (), (override));
    MOCK_METHOD(std::optional<std::chrono::milliseconds>, getSessionServerStartupTimeout, (), (override));
    MOCK_METHOD(std::optional<std::chrono::seconds>, getHealthcheckInterval, (), (override));
    MOCK_METHOD(std::optional<firebolt::rialto::common::SocketPermissions>, getSocketPermissions, (), (override));
    MOCK_METHOD(std::optional<unsigned int>, getNumOfPreloadedServers, (), (override));
    MOCK_METHOD(std::optional<rialto::servermanager::service::LoggingLevels>, getLoggingLevels, (), (override));
    MOCK_METHOD(std::optional<unsigned int>, getNumOfPingsBeforeRecovery, (), (override));
};
} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_CONFIG_READER_MOCK_H_
