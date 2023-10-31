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
#include "ConfigReaderFactoryMock.h"
#include "ConfigReaderMock.h"
#include <gtest/gtest.h>

using firebolt::rialto::common::ServerManagerConfig;
using rialto::servermanager::service::ConfigHelper;
using rialto::servermanager::service::ConfigReaderFactoryMock;
using rialto::servermanager::service::ConfigReaderMock;
using rialto::servermanager::service::LoggingLevel;
using testing::Return;

namespace
{
const ServerManagerConfig kServerManagerConfig{{"env1=var1"},
                                               2,
                                               "sessionServerPath",
                                               std::chrono::milliseconds{3},
                                               std::chrono::seconds{4},
                                               {7, 7, 7},
                                               5};
} // namespace

TEST(ConfigHelperTests, ShouldNotOverrideDefaultValuesWhenFactoryIsNull)
{
    ConfigHelper sut{nullptr, kServerManagerConfig};
    EXPECT_EQ(sut.getSessionServerEnvVars(), kServerManagerConfig.sessionServerEnvVars);
    EXPECT_EQ(sut.getSessionServerPath(), kServerManagerConfig.sessionServerPath);
    EXPECT_EQ(sut.getSessionServerStartupTimeout(), kServerManagerConfig.sessionServerStartupTimeout);
    EXPECT_EQ(sut.getHealthcheckInterval(), kServerManagerConfig.healthcheckInterval);
    EXPECT_EQ(sut.getSocketPermissions().ownerPermissions,
              kServerManagerConfig.sessionManagementSocketPermissions.ownerPermissions);
    EXPECT_EQ(sut.getSocketPermissions().groupPermissions,
              kServerManagerConfig.sessionManagementSocketPermissions.groupPermissions);
    EXPECT_EQ(sut.getSocketPermissions().otherPermissions,
              kServerManagerConfig.sessionManagementSocketPermissions.otherPermissions);
    EXPECT_EQ(sut.getNumOfPreloadedServers(), kServerManagerConfig.numOfPreloadedServers);
    EXPECT_EQ(sut.getNumOfFailedPingsBeforeRecovery(), kServerManagerConfig.numOfFailedPingsBeforeRecovery);
    EXPECT_EQ(sut.getLoggingLevels().defaultLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().clientLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().sessionServerLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().ipcLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().serverManagerLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().commonLoggingLevel, LoggingLevel::UNCHANGED);
}

TEST(ConfigHelperTests, ShouldNotOverrideDefaultValuesWhenConfigReaderIsNull)
{
    std::unique_ptr<ConfigReaderFactoryMock> configReaderFactoryMock{std::make_unique<ConfigReaderFactoryMock>()};
    std::shared_ptr<ConfigReaderMock> configReaderMock{std::make_shared<ConfigReaderMock>()};
    EXPECT_CALL(*configReaderFactoryMock, createConfigReader()).WillOnce(Return(configReaderMock));
    EXPECT_CALL(*configReaderMock, read()).WillOnce(Return(false));
    ConfigHelper sut{std::move(configReaderFactoryMock), kServerManagerConfig};
    EXPECT_EQ(sut.getSessionServerEnvVars(), kServerManagerConfig.sessionServerEnvVars);
    EXPECT_EQ(sut.getSessionServerPath(), kServerManagerConfig.sessionServerPath);
    EXPECT_EQ(sut.getSessionServerStartupTimeout(), kServerManagerConfig.sessionServerStartupTimeout);
    EXPECT_EQ(sut.getHealthcheckInterval(), kServerManagerConfig.healthcheckInterval);
    EXPECT_EQ(sut.getSocketPermissions().ownerPermissions,
              kServerManagerConfig.sessionManagementSocketPermissions.ownerPermissions);
    EXPECT_EQ(sut.getSocketPermissions().groupPermissions,
              kServerManagerConfig.sessionManagementSocketPermissions.groupPermissions);
    EXPECT_EQ(sut.getSocketPermissions().otherPermissions,
              kServerManagerConfig.sessionManagementSocketPermissions.otherPermissions);
    EXPECT_EQ(sut.getNumOfPreloadedServers(), kServerManagerConfig.numOfPreloadedServers);
    EXPECT_EQ(sut.getNumOfFailedPingsBeforeRecovery(), kServerManagerConfig.numOfFailedPingsBeforeRecovery);
    EXPECT_EQ(sut.getLoggingLevels().defaultLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().clientLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().sessionServerLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().ipcLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().serverManagerLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().commonLoggingLevel, LoggingLevel::UNCHANGED);
}

TEST(ConfigHelperTests, ShouldNotOverrideDefaultValuesWhenConfigReaderReturnsNullopts)
{
    std::unique_ptr<ConfigReaderFactoryMock> configReaderFactoryMock{std::make_unique<ConfigReaderFactoryMock>()};
    std::shared_ptr<ConfigReaderMock> configReaderMock{std::make_shared<ConfigReaderMock>()};
    EXPECT_CALL(*configReaderFactoryMock, createConfigReader()).WillOnce(Return(configReaderMock));
    EXPECT_CALL(*configReaderMock, read()).WillOnce(Return(true));
    EXPECT_CALL(*configReaderMock, getEnvironmentVariables()).WillOnce(Return(std::list<std::string>{}));
    EXPECT_CALL(*configReaderMock, getSessionServerPath()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getSessionServerStartupTimeout()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getHealthcheckInterval()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getSocketPermissions()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getNumOfPreloadedServers()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getLoggingLevels()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getNumOfPingsBeforeRecovery()).WillOnce(Return(std::nullopt));

    ConfigHelper sut{std::move(configReaderFactoryMock), kServerManagerConfig};

    EXPECT_EQ(sut.getSessionServerEnvVars(), kServerManagerConfig.sessionServerEnvVars);
    EXPECT_EQ(sut.getSessionServerPath(), kServerManagerConfig.sessionServerPath);
    EXPECT_EQ(sut.getSessionServerStartupTimeout(), kServerManagerConfig.sessionServerStartupTimeout);
    EXPECT_EQ(sut.getHealthcheckInterval(), kServerManagerConfig.healthcheckInterval);
    EXPECT_EQ(sut.getSocketPermissions().ownerPermissions,
              kServerManagerConfig.sessionManagementSocketPermissions.ownerPermissions);
    EXPECT_EQ(sut.getSocketPermissions().groupPermissions,
              kServerManagerConfig.sessionManagementSocketPermissions.groupPermissions);
    EXPECT_EQ(sut.getSocketPermissions().otherPermissions,
              kServerManagerConfig.sessionManagementSocketPermissions.otherPermissions);
    EXPECT_EQ(sut.getNumOfPreloadedServers(), kServerManagerConfig.numOfPreloadedServers);
    EXPECT_EQ(sut.getNumOfFailedPingsBeforeRecovery(), kServerManagerConfig.numOfFailedPingsBeforeRecovery);
    EXPECT_EQ(sut.getLoggingLevels().defaultLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().clientLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().sessionServerLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().ipcLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().serverManagerLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().commonLoggingLevel, LoggingLevel::UNCHANGED);
}
