/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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
#include "ServerStub.h"
#include "TestClientMock.h"
#include "TestModuleMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::ipc;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

MATCHER_P(SingleVarRequestMatcher, var1, "")
{
    return (arg->var1() == var1);
}

MATCHER_P4(MultiVarRequestMatcher, var1, var2, var3, var4, "")
{
    return ((arg->var1() == var1) && (arg->var2() == var2) && (arg->var3() == var3) && (arg->var4() == var4));
}

class RialtoIpcTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<TestModuleMock>> m_testModuleMock;
    std::shared_ptr<StrictMock<TestClientMock>> m_testClientMock;
    std::shared_ptr<ServerStub> m_serverStub;
    std::shared_ptr<ClientStub> m_clientStub;
    std::string m_socketName = "/tmp/rialto-0";

    int32_t m_int = 432;
    uint32_t m_uint = 678U;
    firebolt::rialto::TestMultiVar_TestType m_enum = firebolt::rialto::TestMultiVar_TestType_ENUM1;
    std::string m_str = "test";

    virtual void SetUp()
    {
        m_testModuleMock = std::make_shared<StrictMock<TestModuleMock>>();
        m_testClientMock = std::make_shared<StrictMock<TestClientMock>>();

        m_serverStub = std::make_shared<ServerStub>(m_testModuleMock);

        m_clientStub = std::make_shared<ClientStub>(m_testClientMock, m_socketName);
        m_clientStub->connect();
    }

    virtual void TearDown()
    {
        m_clientStub->disconnect();
        m_clientStub.reset();

        m_serverStub.reset();

        m_testClientMock.reset();
        m_testModuleMock.reset();
    }
};

/**
 * Test that IPC can send a request with a single variable.
 */
TEST_F(RialtoIpcTest, SingleVarRequest)
{
    EXPECT_CALL(*m_testModuleMock, TestRequestSingleVar(_, SingleVarRequestMatcher(m_int), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn)));

    EXPECT_TRUE(m_clientStub->sendSingleVarRequest(m_int));
}

/**
 * Test that IPC can send a request and expect a response with a single variable.
 */
TEST_F(RialtoIpcTest, SingleVarResponse)
{
    int32_t retInt = 0;

    EXPECT_CALL(*m_testModuleMock, TestResponseSingleVar(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_testModuleMock->getSingleVarResponse(m_int)),
                        WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));

    EXPECT_TRUE(m_clientStub->sendRequestWithSingleVarResponse(retInt));

    EXPECT_EQ(m_int, retInt);
}

/**
 * Test that IPC can send a request with a multiple variables.
 */
TEST_F(RialtoIpcTest, MultiVarRequest)
{
    EXPECT_CALL(*m_testModuleMock, TestRequestMultiVar(_, MultiVarRequestMatcher(m_int, m_uint, m_enum, m_str), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn)));

    EXPECT_TRUE(m_clientStub->sendMultiVarRequest(m_int, m_uint, m_enum, m_str));
}

/**
 * Test that IPC can send a request and expect a response with multiple variables.
 */
TEST_F(RialtoIpcTest, MultiVarResponse)
{
    int32_t retInt = 0;
    uint32_t retUint = 0;
    firebolt::rialto::TestMultiVar_TestType retEnum = firebolt::rialto::TestMultiVar_TestType_ENUM2;
    std::string retStr = "";

    EXPECT_CALL(*m_testModuleMock, TestResponseMultiVar(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_testModuleMock->getMultiVarResponse(m_int, m_uint, m_enum, m_str)),
                        WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));

    EXPECT_TRUE(m_clientStub->sendRequestWithMultiVarResponse(retInt, retUint, retEnum, retStr));

    EXPECT_EQ(m_int, retInt);
    EXPECT_EQ(m_uint, retUint);
    EXPECT_EQ(m_enum, retEnum);
    EXPECT_EQ(m_str, retStr);
}

/**
 * Test that IPC client returns failure if server fails.
 */
TEST_F(RialtoIpcTest, ServerFailure)
{
    EXPECT_CALL(*m_testModuleMock, TestRequestSingleVar(_, _, _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::failureReturn)));

    EXPECT_FALSE(m_clientStub->sendSingleVarRequest(m_int));
}

/**
 * Test that IPC client can received single variable events from the server.
 */
TEST_F(RialtoIpcTest, SingleVarEvent)
{
    int32_t retInt = 0;

    m_clientStub->startMessageThread();

    m_serverStub->sendSingleVarEvent(m_int);

    m_clientStub->waitForSingleVarEvent(retInt);
}

/**
 * Test that IPC client can received multiple variable events from the server.
 */
TEST_F(RialtoIpcTest, MultiVarEvent)
{
    firebolt::rialto::TestEventMultiVar_TestType eventEnum = firebolt::rialto::TestEventMultiVar_TestType_ENUM1;
    int32_t retInt = 0;
    uint32_t retUint = 0;
    firebolt::rialto::TestEventMultiVar_TestType retEnum = firebolt::rialto::TestEventMultiVar_TestType_ENUM2;
    std::string retStr = "";

    m_clientStub->startMessageThread();

    m_serverStub->sendMultiVarEvent(m_int, m_uint, eventEnum, m_str);

    m_clientStub->waitForMultiVarEvent(retInt, retUint, retEnum, retStr);
}

/**
 * Test that IPC client returns false when message is timeouted.
 */
TEST_F(RialtoIpcTest, Timeout)
{
    constexpr bool expectMessage{false};
    ::google::protobuf::RpcController *controller;
    ::google::protobuf::Closure *done;
    m_clientStub->startMessageThread(expectMessage);

    EXPECT_CALL(*m_testModuleMock, TestRequestSingleVar(_, SingleVarRequestMatcher(m_int), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(
            [&](auto *c, auto *d)
            {
                controller = c;
                done = d;
            })));

    EXPECT_FALSE(m_clientStub->sendSingleVarRequest(m_int));

    // To prevent mock leak (can be removed when we remove TODOs from IPC code - RIALTO-20)
    m_testModuleMock->failureReturn(controller, done);
}
