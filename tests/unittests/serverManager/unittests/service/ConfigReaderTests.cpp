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
        : m_jsonCppWrapperMock(std::make_shared<StrictMock<firebolt::rialto::wrappers::JsonCppWrapperMock>>()),
          m_fileReaderMock(std::make_shared<StrictMock<FileReaderMock>>()),
          m_rootJsonValueMock(std::make_shared<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>>()),
          m_objectJsonValueMock(std::make_shared<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>>())
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
    std::shared_ptr<StrictMock<firebolt::rialto::wrappers::JsonCppWrapperMock>> m_jsonCppWrapperMock;
    std::shared_ptr<StrictMock<FileReaderMock>> m_fileReaderMock;
    std::shared_ptr<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>> m_rootJsonValueMock;
    std::shared_ptr<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>> m_objectJsonValueMock;

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
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0u);
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

    EXPECT_CALL(*m_rootJsonValueMock, isMember("environmentVariables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("environmentVariables")).WillOnce(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("environmentVariables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0u);
}

TEST_F(ConfigReaderTests, envVariablesEmptyArray)
{
    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember("environmentVariables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("environmentVariables")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(0));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("environmentVariables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0u);
}

TEST_F(ConfigReaderTests, envVariablesOneElementArrayNotString)
{
    std::shared_ptr<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>> object1JsonValueMock =
        std::make_shared<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>>();

    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember("environmentVariables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("environmentVariables")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(1));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(0u))).WillOnce(Return(object1JsonValueMock));
    EXPECT_CALL(*object1JsonValueMock, isString()).WillOnce(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("environmentVariables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0u);
}

TEST_F(ConfigReaderTests, envVariablesMultipleElementArray)
{
    std::shared_ptr<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>> object1JsonValueMock =
        std::make_shared<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>>();
    std::shared_ptr<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>> object2JsonValueMock =
        std::make_shared<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>>();

    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember("environmentVariables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("environmentVariables")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(2));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(0u))).WillRepeatedly(Return(object1JsonValueMock));
    EXPECT_CALL(*object1JsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*object1JsonValueMock, asString()).WillOnce(Return("ELEM_1"));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(1u))).WillRepeatedly(Return(object2JsonValueMock));
    EXPECT_CALL(*object2JsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*object2JsonValueMock, asString()).WillOnce(Return("ELEM_2"));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("environmentVariables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_THAT(m_sut->getEnvironmentVariables(), UnorderedElementsAre("ELEM_1", "ELEM_2"));
}

TEST_F(ConfigReaderTests, sessionServerPathNotString)
{
    expectSuccessfulParsing();
    expectNotString("sessionServerPath");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerPath().has_value(), false);
}

TEST_F(ConfigReaderTests, sessionServerPathExists)
{
    expectSuccessfulParsing();
    expectReturnString("sessionServerPath", "/usr/bin/RialtoServer");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerPath(), "/usr/bin/RialtoServer");
}

TEST_F(ConfigReaderTests, startupTimerNotUint)
{
    expectSuccessfulParsing();
    expectNotUint("startupTimeoutMs");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerStartupTimeout().has_value(), false);
}

TEST_F(ConfigReaderTests, startupTimerExists)
{
    expectSuccessfulParsing();
    expectReturnUint("startupTimeoutMs", 5000);

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerStartupTimeout(), std::chrono::milliseconds(5000));
}

TEST_F(ConfigReaderTests, healthCheckIntervalNotUint)
{
    expectSuccessfulParsing();
    expectNotUint("healthcheckIntervalInSeconds");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getHealthcheckInterval().has_value(), false);
}

TEST_F(ConfigReaderTests, healthCheckIntervalExists)
{
    expectSuccessfulParsing();
    expectReturnUint("healthcheckIntervalInSeconds", 1);

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getHealthcheckInterval(), std::chrono::seconds(1));
}

TEST_F(ConfigReaderTests, socketPermissionsNotUint)
{
    expectSuccessfulParsing();
    expectNotUint("socketPermissions");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketPermissions().has_value(), false);
}

TEST_F(ConfigReaderTests, socketPermissionsExists)
{
    expectSuccessfulParsing();
    expectReturnUint("socketPermissions", 666);

    EXPECT_TRUE(m_sut->read());
    firebolt::rialto::common::SocketPermissions expectedPermissions{6, 6, 6};
    EXPECT_EQ(m_sut->getSocketPermissions().value().ownerPermissions, expectedPermissions.ownerPermissions);
    EXPECT_EQ(m_sut->getSocketPermissions().value().groupPermissions, expectedPermissions.groupPermissions);
    EXPECT_EQ(m_sut->getSocketPermissions().value().otherPermissions, expectedPermissions.otherPermissions);
}

TEST_F(ConfigReaderTests, socketOwnerNotString)
{
    expectSuccessfulParsing();
    expectNotString("socketOwner");
    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketOwner().has_value(), false);
}

TEST_F(ConfigReaderTests, socketOwnerExists)
{
    const char *kTestValue = "root";
    expectSuccessfulParsing();
    expectReturnString("socketOwner", kTestValue);
    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketOwner().value(), kTestValue);
}

TEST_F(ConfigReaderTests, socketGroupNotString)
{
    expectSuccessfulParsing();
    expectNotString("socketGroup");
    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketGroup().has_value(), false);
}

TEST_F(ConfigReaderTests, socketGroupExists)
{
    const char *kTestValue = "root";
    expectSuccessfulParsing();
    expectReturnString("socketGroup", kTestValue);
    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketGroup().value(), kTestValue);
}

TEST_F(ConfigReaderTests, numOfPreloadedServersNotUint)
{
    expectSuccessfulParsing();
    expectNotUint("numOfPreloadedServers");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPreloadedServers().has_value(), false);
}

TEST_F(ConfigReaderTests, numOfPreloadedServersExists)
{
    expectSuccessfulParsing();
    expectReturnUint("numOfPreloadedServers", 2);

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPreloadedServers(), 2);
}

TEST_F(ConfigReaderTests, logLevelNotUint)
{
    expectSuccessfulParsing();
    expectNotUint("logLevel");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getLoggingLevels().has_value(), false);
}

TEST_F(ConfigReaderTests, logLevelSuccessfulParsing)
{
    expectSuccessfulParsing();
    expectReturnUint("logLevel", 3);

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
    expectNotUint("numOfPingsBeforeRecovery");

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPreloadedServers().has_value(), false);
}

TEST_F(ConfigReaderTests, numOfPingsBeforeRecoveryExists)
{
    expectSuccessfulParsing();
    expectReturnUint("numOfPingsBeforeRecovery", 3);

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPingsBeforeRecovery(), 3);
}

TEST_F(ConfigReaderTests, defaultConfigValuesAreSet)
{
    // "Real world" constants defined in rialto/CMakeLists.txt
    constexpr std::size_t kSessionServerEnvVarsSize{3};
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

TEST_F(ConfigReaderTests, extraEnvVariablesNotArray)
{
    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember("extraEnvVariables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("extraEnvVariables")).WillOnce(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("extraEnvVariables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getExtraEnvVariables().size(), 0u);
}

TEST_F(ConfigReaderTests, extraEnvVariablesEmptyArray)
{
    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember("extraEnvVariables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("extraEnvVariables")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(0));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("extraEnvVariables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getExtraEnvVariables().size(), 0u);
}

TEST_F(ConfigReaderTests, extraEnvVariablesOneElementArrayNotString)
{
    std::shared_ptr<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>> object1JsonValueMock =
        std::make_shared<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>>();

    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember("extraEnvVariables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("extraEnvVariables")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(1));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(0u))).WillOnce(Return(object1JsonValueMock));
    EXPECT_CALL(*object1JsonValueMock, isString()).WillOnce(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("extraEnvVariables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getExtraEnvVariables().size(), 0u);
}

TEST_F(ConfigReaderTests, extraEnvVariablesMultipleElementArray)
{
    std::shared_ptr<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>> object1JsonValueMock =
        std::make_shared<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>>();
    std::shared_ptr<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>> object2JsonValueMock =
        std::make_shared<StrictMock<firebolt::rialto::wrappers::JsonValueWrapperMock>>();

    expectSuccessfulParsing();

    EXPECT_CALL(*m_rootJsonValueMock, isMember("extraEnvVariables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("extraEnvVariables")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(2));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(0u))).WillRepeatedly(Return(object1JsonValueMock));
    EXPECT_CALL(*object1JsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*object1JsonValueMock, asString()).WillOnce(Return("ELEM_1"));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(1u))).WillRepeatedly(Return(object2JsonValueMock));
    EXPECT_CALL(*object2JsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*object2JsonValueMock, asString()).WillOnce(Return("ELEM_2"));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("extraEnvVariables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_THAT(m_sut->getExtraEnvVariables(), UnorderedElementsAre("ELEM_1", "ELEM_2"));
}
