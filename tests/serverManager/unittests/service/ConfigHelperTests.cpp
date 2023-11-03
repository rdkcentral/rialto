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
const std::string kRialtoConfigOverridesPath{"/rialto-config-overrides.json"};
const ServerManagerConfig kServerManagerConfig{{"env1=var1"},
                                               2,
                                               "sessionServerPath",
                                               std::chrono::milliseconds{3},
                                               std::chrono::seconds{4},
                                               {7, 7, 7, "user1", "group1"},
                                               5};
const std::list<std::string> kEmptyEnvVars{};
const std::list<std::string> kOvwerwrittenEnvVar{"env1=var2"};
const std::list<std::string> kJsonSessionServerEnvVars{"env2=var2"};
constexpr unsigned kJsonNumOfPreloadedServers{10};
const std::string kJsonSessionServerPath{"/usr/bin/RialtoServer"};
constexpr std::chrono::milliseconds kJsonStartupTimeout{0};
constexpr std::chrono::seconds kJsonHealthcheckInterval{5};
const SocketPermissions kJsonSocketPermissions{5, 5, 5, "user2", "group2"};
constexpr unsigned kJsonNumOfFailedPingsBeforeRecovery{3};
const LoggingLevels kJsonLoggingLevels{LoggingLevel::DEBUG, LoggingLevel::DEBUG, LoggingLevel::DEBUG,
                                       LoggingLevel::DEBUG, LoggingLevel::DEBUG, LoggingLevel::DEBUG};
} // namespace

class ConfigHelperTests : public testing::Test
{
public:
    void shouldReturnStructValues(ConfigHelper &sut)
    {
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

    void shouldReturnJsonValues(ConfigHelper &sut)
    {
        const std::list<std::string> kExpectedEnvVars{kServerManagerConfig.sessionServerEnvVars.front(),
                                                      kJsonSessionServerEnvVars.front()};
        EXPECT_EQ(sut.getSessionServerEnvVars(), kExpectedEnvVars);
        EXPECT_EQ(sut.getSessionServerPath(), kJsonSessionServerPath);
        EXPECT_EQ(sut.getSessionServerStartupTimeout(), kJsonStartupTimeout);
        EXPECT_EQ(sut.getHealthcheckInterval(), kJsonHealthcheckInterval);
        EXPECT_EQ(sut.getSocketPermissions().ownerPermissions, kJsonSocketPermissions.ownerPermissions);
        EXPECT_EQ(sut.getSocketPermissions().groupPermissions, kJsonSocketPermissions.groupPermissions);
        EXPECT_EQ(sut.getSocketPermissions().otherPermissions, kJsonSocketPermissions.otherPermissions);
        EXPECT_EQ(sut.getSocketPermissions().owner, kJsonSocketPermissions.owner);
        EXPECT_EQ(sut.getSocketPermissions().group, kJsonSocketPermissions.group);
        EXPECT_EQ(sut.getNumOfPreloadedServers(), kJsonNumOfPreloadedServers);
        EXPECT_EQ(sut.getNumOfFailedPingsBeforeRecovery(), kJsonNumOfFailedPingsBeforeRecovery);
        EXPECT_EQ(sut.getLoggingLevels().defaultLoggingLevel, kJsonLoggingLevels.defaultLoggingLevel);
        EXPECT_EQ(sut.getLoggingLevels().clientLoggingLevel, kJsonLoggingLevels.clientLoggingLevel);
        EXPECT_EQ(sut.getLoggingLevels().sessionServerLoggingLevel, kJsonLoggingLevels.sessionServerLoggingLevel);
        EXPECT_EQ(sut.getLoggingLevels().ipcLoggingLevel, kJsonLoggingLevels.ipcLoggingLevel);
        EXPECT_EQ(sut.getLoggingLevels().serverManagerLoggingLevel, kJsonLoggingLevels.serverManagerLoggingLevel);
        EXPECT_EQ(sut.getLoggingLevels().commonLoggingLevel, kJsonLoggingLevels.commonLoggingLevel);
    }

    void jsonConfigReaderWillFailToReadFile()
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigPath)).WillOnce(Return(m_configReaderMock));
        EXPECT_CALL(*m_configReaderMock, read()).WillOnce(Return(false));
    }

    void jsonConfigReaderWillReturnNulloptsWithEnvVars(const std::list<std::string> &envVars)
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigPath)).WillOnce(Return(m_configReaderMock));
        EXPECT_CALL(*m_configReaderMock, read()).WillOnce(Return(true));
        EXPECT_CALL(*m_configReaderMock, getEnvironmentVariables()).WillOnce(Return(envVars));
        EXPECT_CALL(*m_configReaderMock, getSessionServerPath()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configReaderMock, getSessionServerStartupTimeout()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configReaderMock, getHealthcheckInterval()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configReaderMock, getSocketPermissions()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configReaderMock, getSocketOwner()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configReaderMock, getSocketGroup()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configReaderMock, getNumOfPreloadedServers()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configReaderMock, getLoggingLevels()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configReaderMock, getNumOfPingsBeforeRecovery()).WillOnce(Return(std::nullopt));
    }

    void jsonConfigReaderWillReturnNewValues()
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigPath)).WillOnce(Return(m_configReaderMock));
        EXPECT_CALL(*m_configReaderMock, read()).WillOnce(Return(true));
        EXPECT_CALL(*m_configReaderMock, getEnvironmentVariables()).WillRepeatedly(Return(kJsonSessionServerEnvVars));
        EXPECT_CALL(*m_configReaderMock, getSessionServerPath()).WillRepeatedly(Return(kJsonSessionServerPath));
        EXPECT_CALL(*m_configReaderMock, getSessionServerStartupTimeout()).WillRepeatedly(Return(kJsonStartupTimeout));
        EXPECT_CALL(*m_configReaderMock, getHealthcheckInterval()).WillRepeatedly(Return(kJsonHealthcheckInterval));
        EXPECT_CALL(*m_configReaderMock, getSocketPermissions()).WillRepeatedly(Return(kJsonSocketPermissions));
        EXPECT_CALL(*m_configReaderMock, getSocketOwner()).WillRepeatedly(Return(kJsonSocketPermissions.owner));
        EXPECT_CALL(*m_configReaderMock, getSocketGroup()).WillRepeatedly(Return(kJsonSocketPermissions.group));
        EXPECT_CALL(*m_configReaderMock, getNumOfPreloadedServers()).WillRepeatedly(Return(kJsonNumOfPreloadedServers));
        EXPECT_CALL(*m_configReaderMock, getLoggingLevels()).WillRepeatedly(Return(kJsonLoggingLevels));
        EXPECT_CALL(*m_configReaderMock, getNumOfPingsBeforeRecovery())
            .WillRepeatedly(Return(kJsonNumOfFailedPingsBeforeRecovery));
    }

    void jsonConfigOverridesReaderWillFailToReadFile()
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigOverridesPath))
            .WillOnce(Return(m_configOverridesReaderMock));
        EXPECT_CALL(*m_configOverridesReaderMock, read()).WillOnce(Return(false));
    }

    std::unique_ptr<StrictMock<ConfigReaderFactoryMock>> m_configReaderFactoryMock{
        std::make_unique<StrictMock<ConfigReaderFactoryMock>>()};
    std::shared_ptr<StrictMock<ConfigReaderMock>> m_configReaderMock{std::make_shared<StrictMock<ConfigReaderMock>>()};
    std::shared_ptr<StrictMock<ConfigReaderMock>> m_configOverridesReaderMock{
        std::make_shared<StrictMock<ConfigReaderMock>>()};
};

TEST_F(ConfigHelperTests, ShouldNotUseMainJsonValuesWhenFactoryIsNull)
{
    ConfigHelper sut{nullptr, kServerManagerConfig};
    shouldReturnStructValues(sut);
}

TEST_F(ConfigHelperTests, ShouldNotUseMainJsonValuesWhenConfigReaderIsNull)
{
    jsonConfigReaderWillFailToReadFile();
    jsonConfigOverridesReaderWillFailToReadFile();
    ConfigHelper sut{std::move(m_configReaderFactoryMock), kServerManagerConfig};
    shouldReturnStructValues(sut);
}

TEST_F(ConfigHelperTests, ShouldNotUseMainJsonValuesWhenConfigReaderReturnsNullopts)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kEmptyEnvVars);
    jsonConfigOverridesReaderWillFailToReadFile();
    ConfigHelper sut{std::move(m_configReaderFactoryMock), kServerManagerConfig};
    shouldReturnStructValues(sut);
}

TEST_F(ConfigHelperTests, ShouldUseMainJsonValues)
{
    jsonConfigReaderWillReturnNewValues();
    jsonConfigOverridesReaderWillFailToReadFile();
    ConfigHelper sut{std::move(m_configReaderFactoryMock), kServerManagerConfig};
    shouldReturnJsonValues(sut);
}

TEST_F(ConfigHelperTests, ShouldNotOverrideEnvVariable)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kOvwerwrittenEnvVar);
    jsonConfigOverridesReaderWillFailToReadFile();
    ConfigHelper sut{std::move(m_configReaderFactoryMock), kServerManagerConfig};
    shouldReturnStructValues(sut);
}
