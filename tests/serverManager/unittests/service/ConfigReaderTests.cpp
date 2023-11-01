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

#include "ConfigReader.h"
#include "FileReaderMock.h"
#include "JsonCppWrapperMock.h"
#include "SessionServerCommon.h"
#include "gtest/gtest.h"

using testing::_;
using testing::ByMove;
using testing::DoAll;
using testing::Matcher;
using testing::Return;
using testing::ReturnRef;
using testing::SetArgReferee;
using testing::StrictMock;
using testing::StrNe;
using testing::UnorderedElementsAre;
using namespace rialto::servermanager::service;

class ConfigReaderTests : public testing::Test
{
public:
    ConfigReaderTests()
        : m_jsonCppWrapperMock(std::make_shared<StrictMock<JsonCppWrapperMock>>()),
          m_fileReaderMock(std::make_shared<StrictMock<FileReaderMock>>()),
          m_rootJsonValueMock(std::make_shared<StrictMock<JsonValueWrapperMock>>()),
          m_objectJsonValueMock(std::make_shared<StrictMock<JsonValueWrapperMock>>())
    {
        m_sut = std::make_unique<ConfigReader>(m_jsonCppWrapperMock, m_fileReaderMock);
    }

    void expectSuccessfulParsing()
    {
        EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
        EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
        EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
            .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));
    }

    void expectNotUint(const std::string &key)
    {
        EXPECT_CALL(*m_rootJsonValueMock, isMember(key)).WillOnce(Return(true));
        EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe(key))).WillRepeatedly(Return(false));

        EXPECT_CALL(*m_rootJsonValueMock, at(key)).WillRepeatedly(Return(m_objectJsonValueMock));
        EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(false));
    }

    void expectReturnUint(const std::string &key, const int expectedValue)
    {
        EXPECT_CALL(*m_rootJsonValueMock, isMember(key)).WillOnce(Return(true));
        EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe(key))).WillRepeatedly(Return(false));

        EXPECT_CALL(*m_rootJsonValueMock, at(key)).WillRepeatedly(Return(m_objectJsonValueMock));
        EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(true));
        EXPECT_CALL(*m_objectJsonValueMock, asUInt()).WillOnce(Return(expectedValue));
    }

    void expectNotString(const std::string &key)
    {
        EXPECT_CALL(*m_rootJsonValueMock, isMember(key)).WillOnce(Return(true));
        EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe(key))).WillRepeatedly(Return(false));

        EXPECT_CALL(*m_rootJsonValueMock, at(key)).WillRepeatedly(Return(m_objectJsonValueMock));
        EXPECT_CALL(*m_objectJsonValueMock, isString()).WillOnce(Return(false));
    }

    void expectReturnString(const std::string &key, const std::string &expectedValue)
    {
        EXPECT_CALL(*m_rootJsonValueMock, isMember(key)).WillOnce(Return(true));
        EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe(key))).WillRepeatedly(Return(false));

        EXPECT_CALL(*m_rootJsonValueMock, at(key)).WillRepeatedly(Return(m_objectJsonValueMock));
        EXPECT_CALL(*m_objectJsonValueMock, isString()).WillOnce(Return(true));
        EXPECT_CALL(*m_objectJsonValueMock, asString()).WillOnce(Return(expectedValue));
    }

    ~ConfigReaderTests() override = default;

protected:
    std::shared_ptr<StrictMock<JsonCppWrapperMock>> m_jsonCppWrapperMock;
    std::shared_ptr<StrictMock<FileReaderMock>> m_fileReaderMock;
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_rootJsonValueMock;
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock;

    std::ifstream m_jsonFile;

    std::unique_ptr<ConfigReader> m_sut;
};

TEST_F(ConfigReaderTests, fileOpenFailed)
{
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(false));

    EXPECT_FALSE(m_sut->read());
}

TEST_F(ConfigReaderTests, fileParsingFailed)
{
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(Return(false));

    EXPECT_FALSE(m_sut->read());
}

TEST_F(ConfigReaderTests, thereWillBeNothing)
{
    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember(_)).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0);
    EXPECT_EQ(m_sut->getSessionServerPath().has_value(), false);
    EXPECT_EQ(m_sut->getSessionServerStartupTimeout().has_value(), false);
    EXPECT_EQ(m_sut->getHealthcheckInterval().has_value(), false);
    EXPECT_EQ(m_sut->getSocketPermissions().has_value(), false);
    EXPECT_EQ(m_sut->getSocketOwner().has_value(), false);
    EXPECT_EQ(m_sut->getSocketGroup().has_value(), false);
    EXPECT_EQ(m_sut->getNumOfPreloadedServers().has_value(), false);
}

TEST_F(ConfigReaderTests, envVariablesNotArray)
{
    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember("environment_variables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("environment_variables")).WillOnce(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("environment_variables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0);
}

TEST_F(ConfigReaderTests, envVariablesEmptyArray)
{
    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember("environment_variables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("environment_variables")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(0));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("environment_variables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0);
}

TEST_F(ConfigReaderTests, envVariablesOneElementArrayNotString)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> object1JsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember("environment_variables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("environment_variables")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(1));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(0u))).WillOnce(Return(object1JsonValueMock));
    EXPECT_CALL(*object1JsonValueMock, isString()).WillOnce(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("environment_variables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0);
}

TEST_F(ConfigReaderTests, envVariablesMultipleElementArray)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> object1JsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> object2JsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember("environment_variables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("environment_variables")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(2));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(0u))).WillRepeatedly(Return(object1JsonValueMock));
    EXPECT_CALL(*object1JsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*object1JsonValueMock, asString()).WillOnce(Return("ELEM_1"));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(1u))).WillRepeatedly(Return(object2JsonValueMock));
    EXPECT_CALL(*object2JsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*object2JsonValueMock, asString()).WillOnce(Return("ELEM_2"));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("environment_variables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_THAT(m_sut->getEnvironmentVariables(), UnorderedElementsAre("ELEM_1", "ELEM_2"));
}

TEST_F(ConfigReaderTests, sessionServerPathNotString)
{
    expectSuccessfulParsing();
    expectNotString("session_server_path");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerPath().has_value(), false);
}

TEST_F(ConfigReaderTests, sessionServerPathExists)
{
    expectSuccessfulParsing();
    expectReturnString("session_server_path", "/usr/bin/RialtoServer");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerPath(), "/usr/bin/RialtoServer");
}

TEST_F(ConfigReaderTests, startupTimerNotUint)
{
    expectSuccessfulParsing();
    expectNotUint("startup_timeout_ms");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerStartupTimeout().has_value(), false);
}

TEST_F(ConfigReaderTests, startupTimerExists)
{
    expectSuccessfulParsing();
    expectReturnUint("startup_timeout_ms", 5000);

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerStartupTimeout(), std::chrono::milliseconds(5000));
}

TEST_F(ConfigReaderTests, healthCheckIntervalNotUint)
{
    expectSuccessfulParsing();
    expectNotUint("healthcheck_interval_s");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getHealthcheckInterval().has_value(), false);
}

TEST_F(ConfigReaderTests, healthCheckIntervalExists)
{
    expectSuccessfulParsing();
    expectReturnUint("healthcheck_interval_s", 1);

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getHealthcheckInterval(), std::chrono::seconds(1));
}

TEST_F(ConfigReaderTests, socketPermissionsNotUint)
{
    expectSuccessfulParsing();
    expectNotUint("socket_permissions");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketPermissions().has_value(), false);
}

TEST_F(ConfigReaderTests, socketPermissionsExists)
{
    expectSuccessfulParsing();
    expectReturnUint("socket_permissions", 666);

    EXPECT_TRUE(m_sut->read());
    firebolt::rialto::common::SocketPermissions expectedPermissions{6, 6, 6};
    EXPECT_EQ(m_sut->getSocketPermissions().value().ownerPermissions, expectedPermissions.ownerPermissions);
    EXPECT_EQ(m_sut->getSocketPermissions().value().groupPermissions, expectedPermissions.groupPermissions);
    EXPECT_EQ(m_sut->getSocketPermissions().value().otherPermissions, expectedPermissions.otherPermissions);
}

TEST_F(ConfigReaderTests, socketOwnerNotString)
{
    expectSuccessfulParsing();
    expectNotString("socket_owner");
    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketOwner().has_value(), false);
}

TEST_F(ConfigReaderTests, socketOwnerExists)
{
    const char *kTestValue = "root";
    expectSuccessfulParsing();
    expectReturnString("socket_owner", kTestValue);
    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketOwner().value(), kTestValue);
}

TEST_F(ConfigReaderTests, socketGroupNotString)
{
    expectSuccessfulParsing();
    expectNotString("socket_group");
    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketGroup().has_value(), false);
}

TEST_F(ConfigReaderTests, socketGroupExists)
{
    const char *kTestValue = "root";
    expectSuccessfulParsing();
    expectReturnString("socket_group", kTestValue);
    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketGroup().value(), kTestValue);
}

TEST_F(ConfigReaderTests, numOfPreloadedServersNotUint)
{
    expectSuccessfulParsing();
    expectNotUint("num_of_preloaded_servers");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPreloadedServers().has_value(), false);
}

TEST_F(ConfigReaderTests, numOfPreloadedServersExists)
{
    expectSuccessfulParsing();
    expectReturnUint("num_of_preloaded_servers", 2);

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPreloadedServers(), 2);
}

TEST_F(ConfigReaderTests, logLevelNotUint)
{
    expectSuccessfulParsing();
    expectNotUint("log_level");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getLoggingLevels().has_value(), false);
}

TEST_F(ConfigReaderTests, logLevelSuccessfulParsing)
{
    expectSuccessfulParsing();
    expectReturnUint("log_level", 3);

    EXPECT_TRUE(m_sut->read());
    rialto::servermanager::service::LoggingLevels loggingLevel{rialto::servermanager::service::LoggingLevel::MILESTONE,
                                                               rialto::servermanager::service::LoggingLevel::MILESTONE,
                                                               rialto::servermanager::service::LoggingLevel::MILESTONE,
                                                               rialto::servermanager::service::LoggingLevel::MILESTONE,
                                                               rialto::servermanager::service::LoggingLevel::MILESTONE,
                                                               rialto::servermanager::service::LoggingLevel::MILESTONE};

    EXPECT_EQ(m_sut->getLoggingLevels().value().defaultLoggingLevel, loggingLevel.defaultLoggingLevel);
    EXPECT_EQ(m_sut->getLoggingLevels().value().clientLoggingLevel, loggingLevel.clientLoggingLevel);
    EXPECT_EQ(m_sut->getLoggingLevels().value().sessionServerLoggingLevel, loggingLevel.sessionServerLoggingLevel);
    EXPECT_EQ(m_sut->getLoggingLevels().value().ipcLoggingLevel, loggingLevel.ipcLoggingLevel);
    EXPECT_EQ(m_sut->getLoggingLevels().value().serverManagerLoggingLevel, loggingLevel.serverManagerLoggingLevel);
    EXPECT_EQ(m_sut->getLoggingLevels().value().commonLoggingLevel, loggingLevel.commonLoggingLevel);
}

TEST_F(ConfigReaderTests, numOfPingsBeforeRecoveryNotUint)
{
    expectSuccessfulParsing();
    expectNotUint("num_of_pings_before_recovery");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPreloadedServers().has_value(), false);
}

TEST_F(ConfigReaderTests, numOfPingsBeforeRecoveryExists)
{
    expectSuccessfulParsing();
    expectReturnUint("num_of_pings_before_recovery", 3);

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPingsBeforeRecovery(), 3);
}

TEST_F(ConfigReaderTests, defaultConfigValuesAreSet)
{
    // "Real world" constants defined in rialto/CMakeLists.txt
    constexpr std::size_t kSessionServerEnvVarsSize{5};
    constexpr unsigned kNumOfPreloadedServers{0};
    const std::string kSessionServerPath{"/usr/bin/RialtoServer"};
    constexpr unsigned kSessionServerStartupTimeout{0};
    constexpr unsigned kHealthcheckInterval{5};
    constexpr unsigned kDefaultPermissions{6};
    constexpr unsigned kNumOfFailedPingsBeforeRecovery{3};

    firebolt::rialto::common::ServerManagerConfig config;
    EXPECT_EQ(config.sessionServerEnvVars.size(), kSessionServerEnvVarsSize);
    EXPECT_NE(config.sessionServerEnvVars.end(),
              std::find(config.sessionServerEnvVars.begin(), config.sessionServerEnvVars.end(), "XDG_RUNTIME_DIR=/tmp"));
    EXPECT_NE(config.sessionServerEnvVars.end(),
              std::find(config.sessionServerEnvVars.begin(), config.sessionServerEnvVars.end(),
                        "WAYLAND_DISPLAY=westeros-rialto"));
    EXPECT_NE(config.sessionServerEnvVars.end(),
              std::find(config.sessionServerEnvVars.begin(), config.sessionServerEnvVars.end(), "RIALTO_SINKS_RANK=0"));
    EXPECT_NE(config.sessionServerEnvVars.end(),
              std::find(config.sessionServerEnvVars.begin(), config.sessionServerEnvVars.end(),
                        "GST_REGISTRY=/tmp/rialto-server-gstreamer-cache.bin"));
    EXPECT_NE(config.sessionServerEnvVars.end(),
              std::find(config.sessionServerEnvVars.begin(), config.sessionServerEnvVars.end(),
                        "WESTEROS_SINK_USE_ESSRMGR=1"));
    EXPECT_EQ(config.numOfPreloadedServers, kNumOfPreloadedServers);
    EXPECT_EQ(config.sessionServerPath, kSessionServerPath);
    EXPECT_EQ(config.sessionServerStartupTimeout.count(), kSessionServerStartupTimeout);
    EXPECT_EQ(config.healthcheckInterval.count(), kHealthcheckInterval);
    EXPECT_EQ(config.sessionManagementSocketPermissions.ownerPermissions, kDefaultPermissions);
    EXPECT_EQ(config.sessionManagementSocketPermissions.groupPermissions, kDefaultPermissions);
    EXPECT_EQ(config.sessionManagementSocketPermissions.otherPermissions, kDefaultPermissions);
    EXPECT_EQ(config.numOfFailedPingsBeforeRecovery, kNumOfFailedPingsBeforeRecovery);
}
