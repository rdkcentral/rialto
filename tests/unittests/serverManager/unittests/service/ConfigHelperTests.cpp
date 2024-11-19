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
const std::string kRialtoConfigPath{"/etc/rialto-config.json"};
const std::string kRialtoConfigOverridesPath{"/opt/persistent/sky/rialto-config-overrides.json"};
const ServerManagerConfig kServerManagerConfig{{"env1=var1"},
                                               2,
                                               "sessionServerPath",
                                               std::chrono::milliseconds{3},
                                               std::chrono::seconds{4},
                                               {7, 7, 7, "user1", "group1"},
                                               5};
const std::list<std::string> kEmptyEnvVars{};
const std::list<std::string> kOverwrittenEnvVar{"env1=var2"};
const std::list<std::string> kJsonSessionServerEnvVars{"env2=var2"};
constexpr unsigned kJsonNumOfPreloadedServers{10};
const std::string kJsonSessionServerPath{"/usr/bin/RialtoServer"};
constexpr std::chrono::milliseconds kJsonStartupTimeout{0};
constexpr std::chrono::seconds kJsonHealthcheckInterval{5};
const SocketPermissions kJsonSocketPermissions{5, 5, 5, "user2", "group2"};
constexpr unsigned kJsonNumOfFailedPingsBeforeRecovery{3};
const LoggingLevels kJsonLoggingLevels{LoggingLevel::DEBUG, LoggingLevel::DEBUG, LoggingLevel::DEBUG,
                                       LoggingLevel::DEBUG, LoggingLevel::DEBUG, LoggingLevel::DEBUG};
const std::list<std::string> kJsonOverrideSessionServerEnvVars{"env3=var3"};
constexpr unsigned kJsonOverrideNumOfPreloadedServers{13};
const std::string kJsonOverrideSessionServerPath{"/tmp/RialtoServer"};
constexpr std::chrono::milliseconds kJsonOverrideStartupTimeout{12};
constexpr std::chrono::seconds kJsonOverrideHealthcheckInterval{4};
const SocketPermissions kJsonOverrideSocketPermissions{7, 3, 4, "user3", "group3"};
constexpr unsigned kJsonOverrideNumOfFailedPingsBeforeRecovery{7};
const LoggingLevels kJsonOverrideLoggingLevels{LoggingLevel::INFO, LoggingLevel::INFO, LoggingLevel::INFO,
                                               LoggingLevel::INFO, LoggingLevel::INFO, LoggingLevel::INFO};
} // namespace

class ConfigHelperTests : public testing::Test
{
public:
    void shouldReturnStructValues()
    {
        EXPECT_EQ(m_sut->getSessionServerEnvVars(), kServerManagerConfig.sessionServerEnvVars);
        EXPECT_EQ(m_sut->getSessionServerPath(), kServerManagerConfig.sessionServerPath);
        EXPECT_EQ(m_sut->getSessionServerStartupTimeout(), kServerManagerConfig.sessionServerStartupTimeout);
        EXPECT_EQ(m_sut->getHealthcheckInterval(), kServerManagerConfig.healthcheckInterval);
        EXPECT_EQ(m_sut->getSocketPermissions().ownerPermissions,
                  kServerManagerConfig.sessionManagementSocketPermissions.ownerPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().groupPermissions,
                  kServerManagerConfig.sessionManagementSocketPermissions.groupPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().otherPermissions,
                  kServerManagerConfig.sessionManagementSocketPermissions.otherPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().owner, kServerManagerConfig.sessionManagementSocketPermissions.owner);
        EXPECT_EQ(m_sut->getSocketPermissions().group, kServerManagerConfig.sessionManagementSocketPermissions.group);
        EXPECT_EQ(m_sut->getNumOfPreloadedServers(), kServerManagerConfig.numOfPreloadedServers);
        EXPECT_EQ(m_sut->getNumOfFailedPingsBeforeRecovery(), kServerManagerConfig.numOfFailedPingsBeforeRecovery);
        EXPECT_EQ(m_sut->getLoggingLevels().defaultLoggingLevel, LoggingLevel::UNCHANGED);
        EXPECT_EQ(m_sut->getLoggingLevels().clientLoggingLevel, LoggingLevel::UNCHANGED);
        EXPECT_EQ(m_sut->getLoggingLevels().sessionServerLoggingLevel, LoggingLevel::UNCHANGED);
        EXPECT_EQ(m_sut->getLoggingLevels().ipcLoggingLevel, LoggingLevel::UNCHANGED);
        EXPECT_EQ(m_sut->getLoggingLevels().serverManagerLoggingLevel, LoggingLevel::UNCHANGED);
        EXPECT_EQ(m_sut->getLoggingLevels().commonLoggingLevel, LoggingLevel::UNCHANGED);
    }

    void shouldReturnJsonValues()
    {
        const std::list<std::string> kExpectedEnvVars{kServerManagerConfig.sessionServerEnvVars.front(),
                                                      kJsonSessionServerEnvVars.front()};
        EXPECT_EQ(m_sut->getSessionServerEnvVars(), kExpectedEnvVars);
        EXPECT_EQ(m_sut->getSessionServerPath(), kJsonSessionServerPath);
        EXPECT_EQ(m_sut->getSessionServerStartupTimeout(), kJsonStartupTimeout);
        EXPECT_EQ(m_sut->getHealthcheckInterval(), kJsonHealthcheckInterval);
        EXPECT_EQ(m_sut->getSocketPermissions().ownerPermissions, kJsonSocketPermissions.ownerPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().groupPermissions, kJsonSocketPermissions.groupPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().otherPermissions, kJsonSocketPermissions.otherPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().owner, kJsonSocketPermissions.owner);
        EXPECT_EQ(m_sut->getSocketPermissions().group, kJsonSocketPermissions.group);
        EXPECT_EQ(m_sut->getNumOfPreloadedServers(), kJsonNumOfPreloadedServers);
        EXPECT_EQ(m_sut->getNumOfFailedPingsBeforeRecovery(), kJsonNumOfFailedPingsBeforeRecovery);
        EXPECT_EQ(m_sut->getLoggingLevels().defaultLoggingLevel, kJsonLoggingLevels.defaultLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().clientLoggingLevel, kJsonLoggingLevels.clientLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().sessionServerLoggingLevel, kJsonLoggingLevels.sessionServerLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().ipcLoggingLevel, kJsonLoggingLevels.ipcLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().serverManagerLoggingLevel, kJsonLoggingLevels.serverManagerLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().commonLoggingLevel, kJsonLoggingLevels.commonLoggingLevel);
    }

    void shouldReturnJsonOverrideValues()
    {
        const std::list<std::string> kExpectedEnvVars{kServerManagerConfig.sessionServerEnvVars.front(),
                                                      kJsonSessionServerEnvVars.front(),
                                                      kJsonOverrideSessionServerEnvVars.front()};
        EXPECT_EQ(m_sut->getSessionServerEnvVars(), kExpectedEnvVars);
        EXPECT_EQ(m_sut->getSessionServerPath(), kJsonOverrideSessionServerPath);
        EXPECT_EQ(m_sut->getSessionServerStartupTimeout(), kJsonOverrideStartupTimeout);
        EXPECT_EQ(m_sut->getHealthcheckInterval(), kJsonOverrideHealthcheckInterval);
        EXPECT_EQ(m_sut->getSocketPermissions().ownerPermissions, kJsonOverrideSocketPermissions.ownerPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().groupPermissions, kJsonOverrideSocketPermissions.groupPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().otherPermissions, kJsonOverrideSocketPermissions.otherPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().owner, kJsonOverrideSocketPermissions.owner);
        EXPECT_EQ(m_sut->getSocketPermissions().group, kJsonOverrideSocketPermissions.group);
        EXPECT_EQ(m_sut->getNumOfPreloadedServers(), kJsonOverrideNumOfPreloadedServers);
        EXPECT_EQ(m_sut->getNumOfFailedPingsBeforeRecovery(), kJsonOverrideNumOfFailedPingsBeforeRecovery);
        EXPECT_EQ(m_sut->getLoggingLevels().defaultLoggingLevel, kJsonOverrideLoggingLevels.defaultLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().clientLoggingLevel, kJsonOverrideLoggingLevels.clientLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().sessionServerLoggingLevel,
                  kJsonOverrideLoggingLevels.sessionServerLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().ipcLoggingLevel, kJsonOverrideLoggingLevels.ipcLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().serverManagerLoggingLevel,
                  kJsonOverrideLoggingLevels.serverManagerLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().commonLoggingLevel, kJsonOverrideLoggingLevels.commonLoggingLevel);
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

    void jsonConfigOverridesReaderWillReturnNulloptsWithEnvVars(const std::list<std::string> &envVars)
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigOverridesPath))
            .WillOnce(Return(m_configOverridesReaderMock));
        EXPECT_CALL(*m_configOverridesReaderMock, read()).WillOnce(Return(true));
        EXPECT_CALL(*m_configOverridesReaderMock, getEnvironmentVariables()).WillOnce(Return(envVars));
        EXPECT_CALL(*m_configOverridesReaderMock, getSessionServerPath()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configOverridesReaderMock, getSessionServerStartupTimeout()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configOverridesReaderMock, getHealthcheckInterval()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configOverridesReaderMock, getSocketPermissions()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configOverridesReaderMock, getSocketOwner()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configOverridesReaderMock, getSocketGroup()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configOverridesReaderMock, getNumOfPreloadedServers()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configOverridesReaderMock, getLoggingLevels()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configOverridesReaderMock, getNumOfPingsBeforeRecovery()).WillOnce(Return(std::nullopt));
    }

    void jsonConfigOverridesReaderWillReturnNewValues()
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigOverridesPath))
            .WillOnce(Return(m_configOverridesReaderMock));
        EXPECT_CALL(*m_configOverridesReaderMock, read()).WillOnce(Return(true));
        EXPECT_CALL(*m_configOverridesReaderMock, getEnvironmentVariables())
            .WillRepeatedly(Return(kJsonOverrideSessionServerEnvVars));
        EXPECT_CALL(*m_configOverridesReaderMock, getSessionServerPath())
            .WillRepeatedly(Return(kJsonOverrideSessionServerPath));
        EXPECT_CALL(*m_configOverridesReaderMock, getSessionServerStartupTimeout())
            .WillRepeatedly(Return(kJsonOverrideStartupTimeout));
        EXPECT_CALL(*m_configOverridesReaderMock, getHealthcheckInterval())
            .WillRepeatedly(Return(kJsonOverrideHealthcheckInterval));
        EXPECT_CALL(*m_configOverridesReaderMock, getSocketPermissions())
            .WillRepeatedly(Return(kJsonOverrideSocketPermissions));
        EXPECT_CALL(*m_configOverridesReaderMock, getSocketOwner())
            .WillRepeatedly(Return(kJsonOverrideSocketPermissions.owner));
        EXPECT_CALL(*m_configOverridesReaderMock, getSocketGroup())
            .WillRepeatedly(Return(kJsonOverrideSocketPermissions.group));
        EXPECT_CALL(*m_configOverridesReaderMock, getNumOfPreloadedServers())
            .WillRepeatedly(Return(kJsonOverrideNumOfPreloadedServers));
        EXPECT_CALL(*m_configOverridesReaderMock, getLoggingLevels()).WillRepeatedly(Return(kJsonOverrideLoggingLevels));
        EXPECT_CALL(*m_configOverridesReaderMock, getNumOfPingsBeforeRecovery())
            .WillRepeatedly(Return(kJsonOverrideNumOfFailedPingsBeforeRecovery));
    }

    void initSut(std::unique_ptr<StrictMock<ConfigReaderFactoryMock>> &&configReaderFactory)
    {
        m_sut = std::make_unique<ConfigHelper>(std::move(configReaderFactory), kServerManagerConfig);
    }

protected:
    std::unique_ptr<ConfigHelper> m_sut;
    std::unique_ptr<StrictMock<ConfigReaderFactoryMock>> m_configReaderFactoryMock{
        std::make_unique<StrictMock<ConfigReaderFactoryMock>>()};
    std::shared_ptr<StrictMock<ConfigReaderMock>> m_configReaderMock{std::make_shared<StrictMock<ConfigReaderMock>>()};
    std::shared_ptr<StrictMock<ConfigReaderMock>> m_configOverridesReaderMock{
        std::make_shared<StrictMock<ConfigReaderMock>>()};
};

TEST_F(ConfigHelperTests, ShouldNotUseMainJsonValuesWhenFactoryIsNull)
{
    initSut(nullptr);
    shouldReturnStructValues();
}

TEST_F(ConfigHelperTests, ShouldNotUseMainJsonValuesWhenConfigReaderIsNull)
{
    jsonConfigReaderWillFailToReadFile();
    jsonConfigOverridesReaderWillFailToReadFile();
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnStructValues();
}

TEST_F(ConfigHelperTests, ShouldNotUseMainJsonValuesWhenConfigReaderReturnsNullopts)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kEmptyEnvVars);
    jsonConfigOverridesReaderWillFailToReadFile();
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnStructValues();
}

// TEST_F(ConfigHelperTests, ShouldUseMainJsonValues)
// {
//     jsonConfigReaderWillReturnNewValues();
//     jsonConfigOverridesReaderWillFailToReadFile();
//     initSut(std::move(m_configReaderFactoryMock));
//     shouldReturnJsonValues();
// }

// TEST_F(ConfigHelperTests, ShouldNotOverrideEnvVariable)
// {
//     jsonConfigReaderWillReturnNulloptsWithEnvVars(kOverwrittenEnvVar);
//     jsonConfigOverridesReaderWillFailToReadFile();
//     initSut(std::move(m_configReaderFactoryMock));
//     shouldReturnStructValues();
// }

// TEST_F(ConfigHelperTests, ShouldNotUseMainJsonValuesWhenConfigReadersReturnNullopts)
// {
//     jsonConfigReaderWillReturnNulloptsWithEnvVars(kEmptyEnvVars);
//     jsonConfigOverridesReaderWillReturnNulloptsWithEnvVars(kEmptyEnvVars);
//     initSut(std::move(m_configReaderFactoryMock));
//     shouldReturnStructValues();
// }

// TEST_F(ConfigHelperTests, ShouldNotOverrideEnvVariableByOverridesFile)
// {
//     jsonConfigReaderWillReturnNulloptsWithEnvVars(kOverwrittenEnvVar);
//     jsonConfigOverridesReaderWillReturnNulloptsWithEnvVars(kOverwrittenEnvVar);
//     initSut(std::move(m_configReaderFactoryMock));
//     shouldReturnStructValues();
// }

// TEST_F(ConfigHelperTests, ShouldUseJsonOverrideValues)
// {
//     jsonConfigReaderWillReturnNewValues();
//     jsonConfigOverridesReaderWillReturnNewValues();
//     initSut(std::move(m_configReaderFactoryMock));
//     shouldReturnJsonOverrideValues();
// }
