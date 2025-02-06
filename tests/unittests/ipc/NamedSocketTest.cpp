/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "ClientStub.h"
#include "INamedSocket.h"
#include "ServerStub.h"
#include "TestClientMock.h"
#include "TestModuleMock.h"
#include <gtest/gtest.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::ipc;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

class NamedSocketTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<TestModuleMock>> m_testModuleMock;
    std::shared_ptr<StrictMock<TestClientMock>> m_testClientMock;
    std::unique_ptr<INamedSocket> m_namedSocket;
    std::shared_ptr<ServerStub> m_serverStub;
    std::shared_ptr<ClientStub> m_clientStub;
    std::string m_socketName = "/tmp/rialto-0";

    int32_t m_int = 432;
    uint32_t m_uint = 678U;
    firebolt::rialto::TestMultiVar_TestType m_enum = firebolt::rialto::TestMultiVar_TestType_ENUM1;
    std::string m_str = "test";

    void init()
    {
        m_testModuleMock = std::make_shared<StrictMock<TestModuleMock>>();
        m_testClientMock = std::make_shared<StrictMock<TestClientMock>>();

        m_serverStub = std::make_shared<ServerStub>(m_testModuleMock);
        m_serverStub->initWithFd(m_namedSocket->getFd());

        m_clientStub = std::make_shared<ClientStub>(m_testClientMock, m_socketName);
        m_clientStub->connect();
    }

    virtual void TearDown()
    {
        if (m_clientStub)
        {
            m_clientStub->disconnect();
            m_clientStub.reset();
        }

        m_serverStub.reset();

        m_testClientMock.reset();
        m_testModuleMock.reset();
    }
};

/**
 * Test that IPC can send a request and expect a response with a single variable.
 */
TEST_F(NamedSocketTest, Communication)
{
    m_namedSocket = INamedSocketFactory::getFactory().createNamedSocket(m_socketName);

    init();

    int32_t retInt = 0;

    EXPECT_CALL(*m_testModuleMock, TestResponseSingleVar(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_testModuleMock->getSingleVarResponse(m_int)),
                        WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));

    EXPECT_TRUE(m_clientStub->sendRequestWithSingleVarResponse(retInt));

    EXPECT_EQ(m_int, retInt);
}

/**
 * Test that IPC can send a request and expect a response with a single variable.
 */
TEST_F(NamedSocketTest, CommunicationWithBind)
{
    m_namedSocket = INamedSocketFactory::getFactory().createNamedSocket();
    m_namedSocket->bind(m_socketName);

    init();

    int32_t retInt = 0;

    EXPECT_CALL(*m_testModuleMock, TestResponseSingleVar(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_testModuleMock->getSingleVarResponse(m_int)),
                        WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));

    EXPECT_TRUE(m_clientStub->sendRequestWithSingleVarResponse(retInt));

    EXPECT_EQ(m_int, retInt);
}

/**
 * Test that it's possible to change socket owner and permissions
 */
TEST_F(NamedSocketTest, OwnershipAndPermissions)
{
    uid_t uid = getuid();
    passwd pwd;
    passwd *result;
    char buf[1024];
    getpwuid_r(uid, &pwd, buf, sizeof(buf), &result);

    m_namedSocket = INamedSocketFactory::getFactory().createNamedSocket(m_socketName);
    EXPECT_TRUE(m_namedSocket->setSocketPermissions(0777));
    EXPECT_TRUE(m_namedSocket->setSocketOwnership(pwd.pw_name, pwd.pw_name));

    init();

    int32_t retInt = 0;

    EXPECT_CALL(*m_testModuleMock, TestResponseSingleVar(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_testModuleMock->getSingleVarResponse(m_int)),
                        WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));

    EXPECT_TRUE(m_clientStub->sendRequestWithSingleVarResponse(retInt));

    EXPECT_EQ(m_int, retInt);
}

/**
 * Test that it's possible to block new connections
 */
TEST_F(NamedSocketTest, BlockNewConnections)
{
    m_namedSocket = INamedSocketFactory::getFactory().createNamedSocket(m_socketName);
    EXPECT_TRUE(m_namedSocket->blockNewConnections());
}

/**
 * Test that block new connections is skipped when socket name is not set
 */
TEST_F(NamedSocketTest, SkipBlockNewConnections)
{
    m_namedSocket = INamedSocketFactory::getFactory().createNamedSocket();
    EXPECT_TRUE(m_namedSocket->blockNewConnections());
}

/**
 * Test that it's not possible to bind twice
 */
TEST_F(NamedSocketTest, SkipBindTwice)
{
    m_namedSocket = INamedSocketFactory::getFactory().createNamedSocket();
    m_namedSocket->bind(m_socketName);
    m_namedSocket->bind(m_socketName);

    init();

    int32_t retInt = 0;

    EXPECT_CALL(*m_testModuleMock, TestResponseSingleVar(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_testModuleMock->getSingleVarResponse(m_int)),
                        WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));

    EXPECT_TRUE(m_clientStub->sendRequestWithSingleVarResponse(retInt));

    EXPECT_EQ(m_int, retInt);
}
