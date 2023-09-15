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

#include "gtest/gtest.h"
#include "JsonCppWrapperMock.h"
#include "FileReaderMock.h"
#include "ConfigReader.h"

using testing::_;
using testing::ByMove;
using testing::DoAll;
using testing::Return;
using testing::SetArgReferee;
using testing::StrictMock;
using testing::StrNe;
using testing::Matcher;
using testing::UnorderedElementsAre;
using namespace rialto::servermanager::service;

class ConfigReaderTests : public testing::Test
{
public:
    ConfigReaderTests()
        : m_jsonCppWrapperMock(std::make_shared<StrictMock<JsonCppWrapperMock>>()),
          m_fileReaderMock(std::make_shared<StrictMock<FileReaderMock>>()),
          m_rootJsonValueMock(std::make_shared<StrictMock<JsonValueWrapperMock>>())
    {
        m_sut = std::make_unique<ConfigReader>("FILE_NAME", m_jsonCppWrapperMock, m_fileReaderMock);
    }
    ~ConfigReaderTests() override = default;

protected:
    std::shared_ptr<StrictMock<JsonCppWrapperMock>> m_jsonCppWrapperMock;
    std::shared_ptr<StrictMock<FileReaderMock>> m_fileReaderMock;
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_rootJsonValueMock;

    std::unique_ptr<ConfigReader> m_sut;
};

TEST_F(ConfigReaderTests, fileOpenFailed)
{
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(false));

    EXPECT_FALSE(m_sut->read());
}

TEST_F(ConfigReaderTests, fileParsingFailed)
{
    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(Return(false));

    EXPECT_FALSE(m_sut->read());
}

TEST_F(ConfigReaderTests, thereWillBeNothing)
{
    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(_)).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0);
    EXPECT_EQ(m_sut->getSessionServerPath().has_value(), false);
    EXPECT_EQ(m_sut->getSessionServerStartupTimeout().has_value(), false);
    EXPECT_EQ(m_sut->getHealthcheckInterval().has_value(), false);
    EXPECT_EQ(m_sut->getSocketPermissions().has_value(), false);
    EXPECT_EQ(m_sut->getNumOfPreloadedServers().has_value(), false);
}

TEST_F(ConfigReaderTests, envVariablesNotArray)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();
    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("ENVIRONMENT_VARIABLES")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("ENVIRONMENT_VARIABLES")).WillOnce(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("ENVIRONMENT_VARIABLES"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0);
}

TEST_F(ConfigReaderTests, envVariablesEmptyArray)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();
    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("ENVIRONMENT_VARIABLES")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("ENVIRONMENT_VARIABLES")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(0));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("ENVIRONMENT_VARIABLES"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0);
}

TEST_F(ConfigReaderTests, envVariablesOneElementArrayNotString)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_object1JsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();
    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("ENVIRONMENT_VARIABLES")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("ENVIRONMENT_VARIABLES")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(1));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(0u))).WillOnce(Return(m_object1JsonValueMock));
    EXPECT_CALL(*m_object1JsonValueMock, isString()).WillOnce(Return(false));


    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("ENVIRONMENT_VARIABLES"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0);
}

TEST_F(ConfigReaderTests, envVariablesMultipleElementArray)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_object1JsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_object2JsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();
    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("ENVIRONMENT_VARIABLES")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("ENVIRONMENT_VARIABLES")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(2));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(0u))).WillRepeatedly(Return(m_object1JsonValueMock));
    EXPECT_CALL(*m_object1JsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*m_object1JsonValueMock, asString()).WillOnce(Return("ELEM_1"));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(1u))).WillRepeatedly(Return(m_object2JsonValueMock));
    EXPECT_CALL(*m_object2JsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*m_object2JsonValueMock, asString()).WillOnce(Return("ELEM_2"));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("ENVIRONMENT_VARIABLES"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_THAT(m_sut->getEnvironmentVariables(), UnorderedElementsAre("ELEM_1", "ELEM_2"));
}

TEST_F(ConfigReaderTests, sessionServerPathNotString)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();

    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("SESSION_SERVER_PATH")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("SESSION_SERVER_PATH"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("SESSION_SERVER_PATH")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isString()).WillOnce(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerPath().has_value(), false);
}

TEST_F(ConfigReaderTests, sessionServerPathExists)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();

    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("SESSION_SERVER_PATH")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("SESSION_SERVER_PATH"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("SESSION_SERVER_PATH")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, asString()).WillOnce(Return("/usr/bin/RialtoServer"));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerPath(), "/usr/bin/RialtoServer");
}

TEST_F(ConfigReaderTests, startupTimerNotUint)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();

    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("STARTUP_TIMEOUT_MS")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("STARTUP_TIMEOUT_MS"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("STARTUP_TIMEOUT_MS")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerStartupTimeout().has_value(), false);
}

TEST_F(ConfigReaderTests, startupTimerExists)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();

    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("STARTUP_TIMEOUT_MS")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("STARTUP_TIMEOUT_MS"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("STARTUP_TIMEOUT_MS")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, asUInt()).WillOnce(Return(5000));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerStartupTimeout(), std::chrono::milliseconds(5000));
}

TEST_F(ConfigReaderTests, healthCheckIntervalNotUint)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();

    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("HEALTHCHECK_INTERVAL")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("HEALTHCHECK_INTERVAL"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("HEALTHCHECK_INTERVAL")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getHealthcheckInterval().has_value(), false);
}

TEST_F(ConfigReaderTests, healthCheckIntervalExists)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();

    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("HEALTHCHECK_INTERVAL")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("HEALTHCHECK_INTERVAL"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("HEALTHCHECK_INTERVAL")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, asUInt()).WillOnce(Return(1));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getHealthcheckInterval(), std::chrono::seconds(1));
}

TEST_F(ConfigReaderTests, socketPermissionsNotUint)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();

    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("SOCKET_PERMISSIONS")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("SOCKET_PERMISSIONS"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("SOCKET_PERMISSIONS")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketPermissions().has_value(), false);
}

TEST_F(ConfigReaderTests, socketPermissionsExists)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();

    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("SOCKET_PERMISSIONS")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("SOCKET_PERMISSIONS"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("SOCKET_PERMISSIONS")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, asUInt()).WillOnce(Return(666));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketPermissions(), 666);
}

TEST_F(ConfigReaderTests, numOfPreloadedServersNotUint)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();

    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("NUM_OF_PRELOADED_SERVERS")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("NUM_OF_PRELOADED_SERVERS"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("NUM_OF_PRELOADED_SERVERS")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPreloadedServers().has_value(), false);
}

TEST_F(ConfigReaderTests, numOfPreloadedServersExists)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock = std::make_shared<StrictMock<JsonValueWrapperMock>>();

    //todo: fix file 
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _)).WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("NUM_OF_PRELOADED_SERVERS")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("NUM_OF_PRELOADED_SERVERS"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("NUM_OF_PRELOADED_SERVERS")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, asUInt()).WillOnce(Return(2));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPreloadedServers(), 2);
}
