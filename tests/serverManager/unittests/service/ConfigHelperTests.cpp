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
using firebolt::rialto::common::SocketPermissions;
using rialto::servermanager::service::ConfigHelper;
using rialto::servermanager::service::ConfigReaderFactoryMock;
using rialto::servermanager::service::ConfigReaderMock;
using rialto::servermanager::service::LoggingLevel;
using rialto::servermanager::service::LoggingLevels;
using testing::Return;
using testing::StrictMock;

namespace
{
const std::string kRialtoConfigPath{"/rialto-config.json"};
const ServerManagerConfig kServerManagerConfig{{"env1=var1"},
                                               2,
                                               "sessionServerPath",
                                               std::chrono::milliseconds{3},
                                               std::chrono::seconds{4},
                                               {7, 7, 7, "user1", "group1"},
                                               5};
const std::list<std::string> kAdditionalSessionServerEnvVars{"env2=var2"};
constexpr unsigned kOverridenNumOfPreloadedServers{10};
const std::string kOverridenSessionServerPath{"/usr/bin/RialtoServer"};
constexpr std::chrono::milliseconds kOverridenStartupTimeout{0};
constexpr std::chrono::seconds kOverridenHealthcheckInterval{5};
const SocketPermissions kOverridenSocketPermissions{5, 5, 5, "user2", "group2"};
constexpr unsigned kOverridenNumOfFailedPingsBeforeRecovery{3};
const LoggingLevels kOverridenLoggingLevels{LoggingLevel::DEBUG, LoggingLevel::DEBUG, LoggingLevel::DEBUG,
                                            LoggingLevel::DEBUG, LoggingLevel::DEBUG, LoggingLevel::DEBUG};
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
    EXPECT_EQ(sut.getSocketPermissions().owner, kServerManagerConfig.sessionManagementSocketPermissions.owner);
    EXPECT_EQ(sut.getSocketPermissions().group, kServerManagerConfig.sessionManagementSocketPermissions.group);
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
    std::unique_ptr<StrictMock<ConfigReaderFactoryMock>> configReaderFactoryMock{
        std::make_unique<StrictMock<ConfigReaderFactoryMock>>()};
    std::shared_ptr<StrictMock<ConfigReaderMock>> configReaderMock{std::make_shared<StrictMock<ConfigReaderMock>>()};
    EXPECT_CALL(*configReaderFactoryMock, createConfigReader(kRialtoConfigPath)).WillOnce(Return(configReaderMock));
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
    EXPECT_EQ(sut.getSocketPermissions().owner, kServerManagerConfig.sessionManagementSocketPermissions.owner);
    EXPECT_EQ(sut.getSocketPermissions().group, kServerManagerConfig.sessionManagementSocketPermissions.group);
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
    std::unique_ptr<StrictMock<ConfigReaderFactoryMock>> configReaderFactoryMock{
        std::make_unique<StrictMock<ConfigReaderFactoryMock>>()};
    std::shared_ptr<StrictMock<ConfigReaderMock>> configReaderMock{std::make_shared<StrictMock<ConfigReaderMock>>()};
    EXPECT_CALL(*configReaderFactoryMock, createConfigReader(kRialtoConfigPath)).WillOnce(Return(configReaderMock));
    EXPECT_CALL(*configReaderMock, read()).WillOnce(Return(true));
    EXPECT_CALL(*configReaderMock, getEnvironmentVariables()).WillOnce(Return(std::list<std::string>{}));
    EXPECT_CALL(*configReaderMock, getSessionServerPath()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getSessionServerStartupTimeout()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getHealthcheckInterval()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getSocketPermissions()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getSocketOwner()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getSocketGroup()).WillOnce(Return(std::nullopt));
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
    EXPECT_EQ(sut.getSocketPermissions().owner, kServerManagerConfig.sessionManagementSocketPermissions.owner);
    EXPECT_EQ(sut.getSocketPermissions().group, kServerManagerConfig.sessionManagementSocketPermissions.group);
    EXPECT_EQ(sut.getNumOfPreloadedServers(), kServerManagerConfig.numOfPreloadedServers);
    EXPECT_EQ(sut.getNumOfFailedPingsBeforeRecovery(), kServerManagerConfig.numOfFailedPingsBeforeRecovery);
    EXPECT_EQ(sut.getLoggingLevels().defaultLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().clientLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().sessionServerLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().ipcLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().serverManagerLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().commonLoggingLevel, LoggingLevel::UNCHANGED);
}

TEST(ConfigHelperTests, ShouldOverrideDefaultValues)
{
    std::unique_ptr<StrictMock<ConfigReaderFactoryMock>> configReaderFactoryMock{
        std::make_unique<StrictMock<ConfigReaderFactoryMock>>()};
    std::shared_ptr<StrictMock<ConfigReaderMock>> configReaderMock{std::make_shared<StrictMock<ConfigReaderMock>>()};
    EXPECT_CALL(*configReaderFactoryMock, createConfigReader(kRialtoConfigPath)).WillOnce(Return(configReaderMock));
    EXPECT_CALL(*configReaderMock, read()).WillOnce(Return(true));
    EXPECT_CALL(*configReaderMock, getEnvironmentVariables()).WillRepeatedly(Return(kAdditionalSessionServerEnvVars));
    EXPECT_CALL(*configReaderMock, getSessionServerPath()).WillRepeatedly(Return(kOverridenSessionServerPath));
    EXPECT_CALL(*configReaderMock, getSessionServerStartupTimeout()).WillRepeatedly(Return(kOverridenStartupTimeout));
    EXPECT_CALL(*configReaderMock, getHealthcheckInterval()).WillRepeatedly(Return(kOverridenHealthcheckInterval));
    EXPECT_CALL(*configReaderMock, getSocketPermissions()).WillRepeatedly(Return(kOverridenSocketPermissions));
    EXPECT_CALL(*configReaderMock, getSocketOwner()).WillRepeatedly(Return(kOverridenSocketPermissions.owner));
    EXPECT_CALL(*configReaderMock, getSocketGroup()).WillRepeatedly(Return(kOverridenSocketPermissions.group));
    EXPECT_CALL(*configReaderMock, getNumOfPreloadedServers()).WillRepeatedly(Return(kOverridenNumOfPreloadedServers));
    EXPECT_CALL(*configReaderMock, getLoggingLevels()).WillRepeatedly(Return(kOverridenLoggingLevels));
    EXPECT_CALL(*configReaderMock, getNumOfPingsBeforeRecovery())
        .WillRepeatedly(Return(kOverridenNumOfFailedPingsBeforeRecovery));

    ConfigHelper sut{std::move(configReaderFactoryMock), kServerManagerConfig};

    const std::list<std::string> kExpectedEnvVars{kServerManagerConfig.sessionServerEnvVars.front(),
                                                  kAdditionalSessionServerEnvVars.front()};
    EXPECT_EQ(sut.getSessionServerEnvVars(), kExpectedEnvVars);
    EXPECT_EQ(sut.getSessionServerPath(), kOverridenSessionServerPath);
    EXPECT_EQ(sut.getSessionServerStartupTimeout(), kOverridenStartupTimeout);
    EXPECT_EQ(sut.getHealthcheckInterval(), kOverridenHealthcheckInterval);
    EXPECT_EQ(sut.getSocketPermissions().ownerPermissions, kOverridenSocketPermissions.ownerPermissions);
    EXPECT_EQ(sut.getSocketPermissions().groupPermissions, kOverridenSocketPermissions.groupPermissions);
    EXPECT_EQ(sut.getSocketPermissions().otherPermissions, kOverridenSocketPermissions.otherPermissions);
    EXPECT_EQ(sut.getSocketPermissions().owner, kOverridenSocketPermissions.owner);
    EXPECT_EQ(sut.getSocketPermissions().group, kOverridenSocketPermissions.group);
    EXPECT_EQ(sut.getNumOfPreloadedServers(), kOverridenNumOfPreloadedServers);
    EXPECT_EQ(sut.getNumOfFailedPingsBeforeRecovery(), kOverridenNumOfFailedPingsBeforeRecovery);
    EXPECT_EQ(sut.getLoggingLevels().defaultLoggingLevel, kOverridenLoggingLevels.defaultLoggingLevel);
    EXPECT_EQ(sut.getLoggingLevels().clientLoggingLevel, kOverridenLoggingLevels.clientLoggingLevel);
    EXPECT_EQ(sut.getLoggingLevels().sessionServerLoggingLevel, kOverridenLoggingLevels.sessionServerLoggingLevel);
    EXPECT_EQ(sut.getLoggingLevels().ipcLoggingLevel, kOverridenLoggingLevels.ipcLoggingLevel);
    EXPECT_EQ(sut.getLoggingLevels().serverManagerLoggingLevel, kOverridenLoggingLevels.serverManagerLoggingLevel);
    EXPECT_EQ(sut.getLoggingLevels().commonLoggingLevel, kOverridenLoggingLevels.commonLoggingLevel);
}

TEST(ConfigHelperTests, ShouldNotOverrideEnvVariable)
{
    std::unique_ptr<StrictMock<ConfigReaderFactoryMock>> configReaderFactoryMock{
        std::make_unique<StrictMock<ConfigReaderFactoryMock>>()};
    std::shared_ptr<StrictMock<ConfigReaderMock>> configReaderMock{std::make_shared<StrictMock<ConfigReaderMock>>()};
    EXPECT_CALL(*configReaderFactoryMock, createConfigReader(kRialtoConfigPath)).WillOnce(Return(configReaderMock));
    EXPECT_CALL(*configReaderMock, read()).WillOnce(Return(true));
    EXPECT_CALL(*configReaderMock, getEnvironmentVariables()).WillOnce(Return(std::list<std::string>{"env1=var2"}));
    EXPECT_CALL(*configReaderMock, getSessionServerPath()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getSessionServerStartupTimeout()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getHealthcheckInterval()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getSocketPermissions()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getSocketOwner()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*configReaderMock, getSocketGroup()).WillOnce(Return(std::nullopt));
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
    EXPECT_EQ(sut.getSocketPermissions().owner, kServerManagerConfig.sessionManagementSocketPermissions.owner);
    EXPECT_EQ(sut.getSocketPermissions().group, kServerManagerConfig.sessionManagementSocketPermissions.group);
    EXPECT_EQ(sut.getNumOfPreloadedServers(), kServerManagerConfig.numOfPreloadedServers);
    EXPECT_EQ(sut.getNumOfFailedPingsBeforeRecovery(), kServerManagerConfig.numOfFailedPingsBeforeRecovery);
    EXPECT_EQ(sut.getLoggingLevels().defaultLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().clientLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().sessionServerLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().ipcLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().serverManagerLoggingLevel, LoggingLevel::UNCHANGED);
    EXPECT_EQ(sut.getLoggingLevels().commonLoggingLevel, LoggingLevel::UNCHANGED);
}
