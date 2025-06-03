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
const std::string kRialtoConfigSocPath{"/etc/rialto-soc.json"};
const std::string kRialtoConfigOverridesPath{"/opt/persistent/sky/rialto-config-overrides.json"};
const std::list<std::string> kEmptyEnvVars{};
const std::list<std::string> kOverwrittenEnvVar{"env1=var2"};
const std::list<std::string> kEnvVarSet1{"env1=var1"};
const std::list<std::string> kEnvVarSet2{"env2=var2"};
const std::list<std::string> kEnvVarSet3{"env3=var3"};
const std::list<std::string> kEnvVarSet4{"env4=var4"};
const ServerManagerConfig kServerManagerConfig{kEnvVarSet1,
                                               2,
                                               "sessionServerPath",
                                               std::chrono::milliseconds{3},
                                               std::chrono::seconds{4},
                                               {7, 7, 7, "user1", "group1"},
                                               5};
constexpr unsigned kJsonNumOfPreloadedServers{10};
const std::string kJsonSessionServerPath{"/usr/bin/RialtoServer"};
constexpr std::chrono::milliseconds kJsonStartupTimeout{0};
constexpr std::chrono::seconds kJsonHealthcheckInterval{5};
const SocketPermissions kJsonSocketPermissions{5, 5, 5, "user2", "group2"};
constexpr unsigned kJsonNumOfFailedPingsBeforeRecovery{3};
const LoggingLevels kJsonLoggingLevels{LoggingLevel::DEBUG, LoggingLevel::DEBUG, LoggingLevel::DEBUG,
                                       LoggingLevel::DEBUG, LoggingLevel::DEBUG, LoggingLevel::DEBUG};
constexpr unsigned kJsonSocNumOfPreloadedServers{11};
const std::string kJsonSocSessionServerPath{"/usr/sbin/RialtoServer"};
constexpr std::chrono::milliseconds kJsonSocStartupTimeout{2};
constexpr std::chrono::seconds kJsonSocHealthcheckInterval{3};
const SocketPermissions kJsonSocSocketPermissions{6, 0, 0, "user3", "group3"};
constexpr unsigned kJsonSocNumOfFailedPingsBeforeRecovery{5};
const LoggingLevels kJsonSocLoggingLevels{LoggingLevel::MILESTONE, LoggingLevel::MILESTONE, LoggingLevel::MILESTONE,
                                          LoggingLevel::MILESTONE, LoggingLevel::MILESTONE, LoggingLevel::MILESTONE};
constexpr unsigned kJsonOverrideNumOfPreloadedServers{13};
const std::string kJsonOverrideSessionServerPath{"/tmp/RialtoServer"};
constexpr std::chrono::milliseconds kJsonOverrideStartupTimeout{12};
constexpr std::chrono::seconds kJsonOverrideHealthcheckInterval{4};
const SocketPermissions kJsonOverrideSocketPermissions{7, 3, 4, "user4", "group4"};
constexpr unsigned kJsonOverrideNumOfFailedPingsBeforeRecovery{7};
const LoggingLevels kJsonOverrideLoggingLevels{LoggingLevel::INFO, LoggingLevel::INFO, LoggingLevel::INFO,
                                               LoggingLevel::INFO, LoggingLevel::INFO, LoggingLevel::INFO};
template <typename... Container> std::list<std::string> mergeLists(const Container... containers)
{
    std::list<std::string> result;
    for (const auto &container : {containers...})
    {
        result.insert(result.end(), container.begin(), container.end());
    }
    return result;
}
} // namespace

class ConfigHelperTests : public testing::Test
{
public:
    void shouldReturnStructValuesWithEnvVars(const std::list<std::string> &expectedEnvVars)
    {
        EXPECT_EQ(m_sut->getSessionServerEnvVars(), expectedEnvVars);
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

    void shouldReturnJsonValues(const std::list<std::string> &expectedEnvVars)
    {
        EXPECT_EQ(m_sut->getSessionServerEnvVars(), expectedEnvVars);
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

    void shouldReturnJsonOverrideValues(const std::list<std::string> &expectedEnvVars)
    {
        EXPECT_EQ(m_sut->getSessionServerEnvVars(), expectedEnvVars);
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

    void shouldReturnJsonSocValues(const std::list<std::string> &expectedEnvVars)
    {
        EXPECT_EQ(m_sut->getSessionServerEnvVars(), expectedEnvVars);
        EXPECT_EQ(m_sut->getSessionServerPath(), kJsonSocSessionServerPath);
        EXPECT_EQ(m_sut->getSessionServerStartupTimeout(), kJsonSocStartupTimeout);
        EXPECT_EQ(m_sut->getHealthcheckInterval(), kJsonSocHealthcheckInterval);
        EXPECT_EQ(m_sut->getSocketPermissions().ownerPermissions, kJsonSocSocketPermissions.ownerPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().groupPermissions, kJsonSocSocketPermissions.groupPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().otherPermissions, kJsonSocSocketPermissions.otherPermissions);
        EXPECT_EQ(m_sut->getSocketPermissions().owner, kJsonSocSocketPermissions.owner);
        EXPECT_EQ(m_sut->getSocketPermissions().group, kJsonSocSocketPermissions.group);
        EXPECT_EQ(m_sut->getNumOfPreloadedServers(), kJsonSocNumOfPreloadedServers);
        EXPECT_EQ(m_sut->getNumOfFailedPingsBeforeRecovery(), kJsonSocNumOfFailedPingsBeforeRecovery);
        EXPECT_EQ(m_sut->getLoggingLevels().defaultLoggingLevel, kJsonSocLoggingLevels.defaultLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().clientLoggingLevel, kJsonSocLoggingLevels.clientLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().sessionServerLoggingLevel, kJsonSocLoggingLevels.sessionServerLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().ipcLoggingLevel, kJsonSocLoggingLevels.ipcLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().serverManagerLoggingLevel, kJsonSocLoggingLevels.serverManagerLoggingLevel);
        EXPECT_EQ(m_sut->getLoggingLevels().commonLoggingLevel, kJsonSocLoggingLevels.commonLoggingLevel);
    }

    void jsonConfigReaderWillFailToReadFile()
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigPath)).WillOnce(Return(m_configReaderMock));
        EXPECT_CALL(*m_configReaderMock, read()).WillOnce(Return(false));
    }

    void jsonConfigReaderWillReturnNulloptsWithEnvVars(const std::list<std::string> &envVars,
                                                       const std::list<std::string> &extraEnvVars)
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigPath)).WillOnce(Return(m_configReaderMock));
        EXPECT_CALL(*m_configReaderMock, read()).WillOnce(Return(true));
        EXPECT_CALL(*m_configReaderMock, getEnvironmentVariables()).WillOnce(Return(envVars));
        EXPECT_CALL(*m_configReaderMock, getExtraEnvVariables()).WillOnce(Return(extraEnvVars));
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

    void jsonConfigReaderWillReturnNewValues(const std::list<std::string> &envVars,
                                             const std::list<std::string> &extraEnvVars)
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigPath)).WillOnce(Return(m_configReaderMock));
        EXPECT_CALL(*m_configReaderMock, read()).WillOnce(Return(true));
        EXPECT_CALL(*m_configReaderMock, getEnvironmentVariables()).WillRepeatedly(Return(envVars));
        EXPECT_CALL(*m_configReaderMock, getExtraEnvVariables()).WillOnce(Return(extraEnvVars));
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

    void jsonConfigOverridesReaderWillReturnNulloptsWithEnvVars(const std::list<std::string> &envVars,
                                                                const std::list<std::string> &extraEnvVars)
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigOverridesPath))
            .WillOnce(Return(m_configOverridesReaderMock));
        EXPECT_CALL(*m_configOverridesReaderMock, read()).WillOnce(Return(true));
        EXPECT_CALL(*m_configOverridesReaderMock, getEnvironmentVariables()).WillOnce(Return(envVars));
        EXPECT_CALL(*m_configOverridesReaderMock, getExtraEnvVariables()).WillOnce(Return(extraEnvVars));
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

    void jsonConfigOverridesReaderWillReturnNewValues(const std::list<std::string> &envVars,
                                                      const std::list<std::string> &extraEnvVars)
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigOverridesPath))
            .WillOnce(Return(m_configOverridesReaderMock));
        EXPECT_CALL(*m_configOverridesReaderMock, read()).WillOnce(Return(true));
        EXPECT_CALL(*m_configOverridesReaderMock, getEnvironmentVariables()).WillRepeatedly(Return(envVars));
        EXPECT_CALL(*m_configOverridesReaderMock, getExtraEnvVariables()).WillOnce(Return(extraEnvVars));
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

    void jsonConfigSocReaderWillFailToReadFile()
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigSocPath))
            .WillOnce(Return(m_configSocReaderMock));
        EXPECT_CALL(*m_configSocReaderMock, read()).WillOnce(Return(false));
    }

    void jsonConfigSocReaderWillReturnNulloptsWithEnvVars(const std::list<std::string> &envVars,
                                                          const std::list<std::string> &extraEnvVars)
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigSocPath))
            .WillOnce(Return(m_configSocReaderMock));
        EXPECT_CALL(*m_configSocReaderMock, read()).WillOnce(Return(true));
        EXPECT_CALL(*m_configSocReaderMock, getEnvironmentVariables()).WillOnce(Return(envVars));
        EXPECT_CALL(*m_configSocReaderMock, getExtraEnvVariables()).WillOnce(Return(extraEnvVars));
        EXPECT_CALL(*m_configSocReaderMock, getSessionServerPath()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configSocReaderMock, getSessionServerStartupTimeout()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configSocReaderMock, getHealthcheckInterval()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configSocReaderMock, getSocketPermissions()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configSocReaderMock, getSocketOwner()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configSocReaderMock, getSocketGroup()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configSocReaderMock, getNumOfPreloadedServers()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configSocReaderMock, getLoggingLevels()).WillOnce(Return(std::nullopt));
        EXPECT_CALL(*m_configSocReaderMock, getNumOfPingsBeforeRecovery()).WillOnce(Return(std::nullopt));
    }

    void jsonConfigSocReaderWillReturnNewValues(const std::list<std::string> &envVars,
                                                const std::list<std::string> &extraEnvVars)
    {
        EXPECT_CALL(*m_configReaderFactoryMock, createConfigReader(kRialtoConfigSocPath))
            .WillOnce(Return(m_configSocReaderMock));
        EXPECT_CALL(*m_configSocReaderMock, read()).WillOnce(Return(true));
        EXPECT_CALL(*m_configSocReaderMock, getEnvironmentVariables()).WillRepeatedly(Return(envVars));
        EXPECT_CALL(*m_configSocReaderMock, getExtraEnvVariables()).WillOnce(Return(extraEnvVars));
        EXPECT_CALL(*m_configSocReaderMock, getSessionServerPath()).WillRepeatedly(Return(kJsonSocSessionServerPath));
        EXPECT_CALL(*m_configSocReaderMock, getSessionServerStartupTimeout()).WillRepeatedly(Return(kJsonSocStartupTimeout));
        EXPECT_CALL(*m_configSocReaderMock, getHealthcheckInterval()).WillRepeatedly(Return(kJsonSocHealthcheckInterval));
        EXPECT_CALL(*m_configSocReaderMock, getSocketPermissions()).WillRepeatedly(Return(kJsonSocSocketPermissions));
        EXPECT_CALL(*m_configSocReaderMock, getSocketOwner()).WillRepeatedly(Return(kJsonSocSocketPermissions.owner));
        EXPECT_CALL(*m_configSocReaderMock, getSocketGroup()).WillRepeatedly(Return(kJsonSocSocketPermissions.group));
        EXPECT_CALL(*m_configSocReaderMock, getNumOfPreloadedServers()).WillRepeatedly(Return(kJsonSocNumOfPreloadedServers));
        EXPECT_CALL(*m_configSocReaderMock, getLoggingLevels()).WillRepeatedly(Return(kJsonSocLoggingLevels));
        EXPECT_CALL(*m_configSocReaderMock, getNumOfPingsBeforeRecovery())
            .WillRepeatedly(Return(kJsonSocNumOfFailedPingsBeforeRecovery));
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
    std::shared_ptr<StrictMock<ConfigReaderMock>> m_configSocReaderMock{std::make_shared<StrictMock<ConfigReaderMock>>()};
};

TEST_F(ConfigHelperTests, ShouldNotUseMainJsonValuesWhenFactoryIsNull)
{
    initSut(nullptr);
    shouldReturnStructValuesWithEnvVars(kEnvVarSet1);
}

TEST_F(ConfigHelperTests, ShouldNotUseMainJsonValuesWhenConfigReaderIsNull)
{
    jsonConfigReaderWillFailToReadFile();
    jsonConfigSocReaderWillFailToReadFile();
    jsonConfigOverridesReaderWillFailToReadFile();
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnStructValuesWithEnvVars(kEnvVarSet1);
}

TEST_F(ConfigHelperTests, ShouldNotUseMainJsonValuesWhenConfigReaderReturnsNullopts)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kEmptyEnvVars, kEmptyEnvVars);
    jsonConfigSocReaderWillFailToReadFile();
    jsonConfigOverridesReaderWillFailToReadFile();
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnStructValuesWithEnvVars(kEnvVarSet1);
}

TEST_F(ConfigHelperTests, ShouldUseMainJsonValues)
{
    jsonConfigReaderWillReturnNewValues(kEnvVarSet2, kEmptyEnvVars);
    jsonConfigSocReaderWillFailToReadFile();
    jsonConfigOverridesReaderWillFailToReadFile();
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnJsonValues(kEnvVarSet2);
}

TEST_F(ConfigHelperTests, ShouldOverrideEnvVariable)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kOverwrittenEnvVar, kEmptyEnvVars);
    jsonConfigSocReaderWillFailToReadFile();
    jsonConfigOverridesReaderWillFailToReadFile();
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnStructValuesWithEnvVars(kOverwrittenEnvVar);
}

TEST_F(ConfigHelperTests, ShouldOverrideEnvVariableWithExtraArgs)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kOverwrittenEnvVar, kEnvVarSet2);
    jsonConfigSocReaderWillFailToReadFile();
    jsonConfigOverridesReaderWillFailToReadFile();
    initSut(std::move(m_configReaderFactoryMock));

    const std::list<std::string> kExpectedEnvVars{mergeLists(kOverwrittenEnvVar, kEnvVarSet2)};
    shouldReturnStructValuesWithEnvVars(kExpectedEnvVars);
}

TEST_F(ConfigHelperTests, ShouldNotUseMainJsonValuesWhenConfigReadersReturnNullopts)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kEmptyEnvVars, kEmptyEnvVars);
    jsonConfigSocReaderWillReturnNulloptsWithEnvVars(kEmptyEnvVars, kEmptyEnvVars);
    jsonConfigOverridesReaderWillReturnNulloptsWithEnvVars(kEmptyEnvVars, kEmptyEnvVars);
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnStructValuesWithEnvVars(kEnvVarSet1);
}

TEST_F(ConfigHelperTests, ShouldOverrideEnvVariablesBySocFile)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kEnvVarSet1, kEnvVarSet2);
    jsonConfigSocReaderWillReturnNulloptsWithEnvVars(kEnvVarSet3, kEnvVarSet4);
    jsonConfigOverridesReaderWillFailToReadFile();
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnStructValuesWithEnvVars(mergeLists(kEnvVarSet3, kEnvVarSet4));
}

TEST_F(ConfigHelperTests, ShouldUseJsonSocValues)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kEnvVarSet1, kEnvVarSet2);
    jsonConfigSocReaderWillReturnNewValues(kEnvVarSet3, kEmptyEnvVars);
    jsonConfigOverridesReaderWillFailToReadFile();
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnJsonSocValues(kEnvVarSet3);
}

TEST_F(ConfigHelperTests, ShouldOverrideEnvVariablesByOverridesFile)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kEnvVarSet1, kEnvVarSet2);
    jsonConfigSocReaderWillReturnNulloptsWithEnvVars(kEnvVarSet3, kEnvVarSet4);
    jsonConfigOverridesReaderWillReturnNulloptsWithEnvVars(kOverwrittenEnvVar, kEmptyEnvVars);
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnStructValuesWithEnvVars(kOverwrittenEnvVar);
}

TEST_F(ConfigHelperTests, ShouldAppendExtraEnvVars)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kEnvVarSet1, kEnvVarSet2);
    jsonConfigSocReaderWillReturnNulloptsWithEnvVars(kEmptyEnvVars, kEnvVarSet3);
    jsonConfigOverridesReaderWillReturnNulloptsWithEnvVars(kEmptyEnvVars, kEnvVarSet4);
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnStructValuesWithEnvVars(mergeLists(kEnvVarSet1, kEnvVarSet4));
}

TEST_F(ConfigHelperTests, ShouldUseJsonOverrideValues)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kEnvVarSet1, kEnvVarSet2);
    jsonConfigSocReaderWillReturnNulloptsWithEnvVars(kEnvVarSet3, kEnvVarSet4);
    jsonConfigOverridesReaderWillReturnNewValues(kEnvVarSet3, kEmptyEnvVars);
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnJsonOverrideValues(kEnvVarSet3);
}

TEST_F(ConfigHelperTests, ShouldOverrideOldValues)
{
    jsonConfigReaderWillReturnNulloptsWithEnvVars(kEnvVarSet1, kEnvVarSet2);
    jsonConfigSocReaderWillReturnNewValues(kEnvVarSet3, kOverwrittenEnvVar);
    jsonConfigOverridesReaderWillReturnNewValues(kEnvVarSet4, kEmptyEnvVars);
    initSut(std::move(m_configReaderFactoryMock));
    shouldReturnJsonOverrideValues(kEnvVarSet4);
}
