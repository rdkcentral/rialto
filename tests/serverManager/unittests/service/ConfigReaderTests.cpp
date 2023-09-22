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
          m_rootJsonValueMock(std::make_shared<StrictMock<JsonValueWrapperMock>>())
    {
        m_sut = std::make_unique<ConfigReader>(m_jsonCppWrapperMock, m_fileReaderMock);
    }
    ~ConfigReaderTests() override = default;

protected:
    std::shared_ptr<StrictMock<JsonCppWrapperMock>> m_jsonCppWrapperMock;
    std::shared_ptr<StrictMock<FileReaderMock>> m_fileReaderMock;
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_rootJsonValueMock;
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
    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

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
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("environment_variables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("environment_variables")).WillOnce(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("environment_variables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0);
}

TEST_F(ConfigReaderTests, envVariablesEmptyArray)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

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
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_object1JsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("environment_variables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("environment_variables")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(1));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(0u))).WillOnce(Return(m_object1JsonValueMock));
    EXPECT_CALL(*m_object1JsonValueMock, isString()).WillOnce(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("environment_variables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getEnvironmentVariables().size(), 0);
}

TEST_F(ConfigReaderTests, envVariablesMultipleElementArray)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_object1JsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_object2JsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("environment_variables")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, at("environment_variables")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isArray()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, size()).WillOnce(Return(2));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(0u))).WillRepeatedly(Return(m_object1JsonValueMock));
    EXPECT_CALL(*m_object1JsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*m_object1JsonValueMock, asString()).WillOnce(Return("ELEM_1"));
    EXPECT_CALL(*m_objectJsonValueMock, at(Matcher<Json::ArrayIndex>(1u))).WillRepeatedly(Return(m_object2JsonValueMock));
    EXPECT_CALL(*m_object2JsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*m_object2JsonValueMock, asString()).WillOnce(Return("ELEM_2"));

    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("environment_variables"))).WillRepeatedly(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_THAT(m_sut->getEnvironmentVariables(), UnorderedElementsAre("ELEM_1", "ELEM_2"));
}

TEST_F(ConfigReaderTests, sessionServerPathNotString)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("session_server_path")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("session_server_path"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("session_server_path")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isString()).WillOnce(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerPath().has_value(), false);
}

TEST_F(ConfigReaderTests, sessionServerPathExists)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("session_server_path")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("session_server_path"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("session_server_path")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isString()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, asString()).WillOnce(Return("/usr/bin/RialtoServer"));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerPath(), "/usr/bin/RialtoServer");
}

TEST_F(ConfigReaderTests, startupTimerNotUint)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("startup_timeout_ms")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("startup_timeout_ms"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("startup_timeout_ms")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerStartupTimeout().has_value(), false);
}

TEST_F(ConfigReaderTests, startupTimerExists)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("startup_timeout_ms")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("startup_timeout_ms"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("startup_timeout_ms")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, asUInt()).WillOnce(Return(5000));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSessionServerStartupTimeout(), std::chrono::milliseconds(5000));
}

TEST_F(ConfigReaderTests, healthCheckIntervalNotUint)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("healthcheck_interval_s")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("healthcheck_interval_s"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("healthcheck_interval_s")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getHealthcheckInterval().has_value(), false);
}

TEST_F(ConfigReaderTests, healthCheckIntervalExists)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("healthcheck_interval_s")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("healthcheck_interval_s"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("healthcheck_interval_s")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, asUInt()).WillOnce(Return(1));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getHealthcheckInterval(), std::chrono::seconds(1));
}

TEST_F(ConfigReaderTests, socketPermissionsNotUint)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("socket_permissions")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("socket_permissions"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("socket_permissions")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getSocketPermissions().has_value(), false);
}

TEST_F(ConfigReaderTests, socketPermissionsExists)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("socket_permissions")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("socket_permissions"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("socket_permissions")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, asUInt()).WillOnce(Return(666));

    EXPECT_TRUE(m_sut->read());
    firebolt::rialto::common::SocketPermissions expectedPermissions{6,6,6};
    EXPECT_EQ(m_sut->getSocketPermissions().value().ownerPermissions, expectedPermissions.ownerPermissions);
    EXPECT_EQ(m_sut->getSocketPermissions().value().groupPermissions, expectedPermissions.groupPermissions);
    EXPECT_EQ(m_sut->getSocketPermissions().value().otherPermissions, expectedPermissions.otherPermissions);
}

TEST_F(ConfigReaderTests, numOfPreloadedServersNotUint)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("num_of_preloaded_servers")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("num_of_preloaded_servers"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("num_of_preloaded_servers")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(false));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPreloadedServers().has_value(), false);
}

TEST_F(ConfigReaderTests, numOfPreloadedServersExists)
{
    std::shared_ptr<StrictMock<JsonValueWrapperMock>> m_objectJsonValueMock =
        std::make_shared<StrictMock<JsonValueWrapperMock>>();

    EXPECT_CALL(*m_fileReaderMock, isOpen()).WillOnce(Return(true));
    EXPECT_CALL(*m_fileReaderMock, get()).WillOnce(ReturnRef(m_jsonFile));
    EXPECT_CALL(*m_jsonCppWrapperMock, parseFromStream(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_rootJsonValueMock), Return(true)));

    EXPECT_CALL(*m_rootJsonValueMock, isMember("num_of_preloaded_servers")).WillOnce(Return(true));
    EXPECT_CALL(*m_rootJsonValueMock, isMember(StrNe("num_of_preloaded_servers"))).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_rootJsonValueMock, at("num_of_preloaded_servers")).WillRepeatedly(Return(m_objectJsonValueMock));
    EXPECT_CALL(*m_objectJsonValueMock, isUInt()).WillOnce(Return(true));
    EXPECT_CALL(*m_objectJsonValueMock, asUInt()).WillOnce(Return(2));

    EXPECT_TRUE(m_sut->read());
    EXPECT_EQ(m_sut->getNumOfPreloadedServers(), 2);
}
