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

#include "RialtoControlIpcTestBase.h"
#include <gtest/gtest.h>

using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::WithArgs;

class RialtoClientRialtoControlIpcSharedMemoryTest : public RialtoControlIpcTestBase
{
protected:
    int32_t m_responseFd = -1;
    uint32_t m_responseSize = 0U;

    virtual void SetUp()
    {
        RialtoControlIpcTestBase::SetUp();

        createRialtoControlIpc();
    }

    virtual void TearDown()
    {
        destroyRialtoControlIpc(true);

        RialtoControlIpcTestBase::TearDown();
    }

public:
    void setGetSharedMemoryResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::GetSharedMemoryResponse *getSharedMemoryResponseResponse =
            dynamic_cast<firebolt::rialto::GetSharedMemoryResponse *>(response);
        getSharedMemoryResponseResponse->set_fd(m_responseFd);
        getSharedMemoryResponseResponse->set_size(m_responseSize);
    }
};

/**
 * Test that RialtoControlIpc::getSharedMemory succeeds and returns the shared memory.
 */
TEST_F(RialtoClientRialtoControlIpcSharedMemoryTest, Success)
{
    int32_t fd = -1;
    uint32_t size = 0U;

    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("getSharedMemory"), m_controllerMock.get(), _, _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientRialtoControlIpcSharedMemoryTest::setGetSharedMemoryResponse)));

    EXPECT_EQ(m_rialtoControlIpc->getSharedMemory(fd, size), true);
    EXPECT_EQ(size, m_responseSize);
    EXPECT_EQ(fd, m_responseFd);

    // Expect disconnect on destruction
    EXPECT_CALL(*m_channelMock, disconnect());
}

/**
 *  Test that RialtoControlIpc::getSharedMemory fails if the client isnt connected.
 */
TEST_F(RialtoClientRialtoControlIpcSharedMemoryTest, ClientDisconnected)
{
    int32_t fd = -1;
    uint32_t size = 0U;

    EXPECT_CALL(*m_channelMock, disconnect());
    EXPECT_EQ(m_rialtoControlIpc->disconnect(), true);

    EXPECT_EQ(m_rialtoControlIpc->getSharedMemory(fd, size), false);
}

/**
 *  Test that RialtoControlIpc::getSharedMemory fails if the ipc request fails.
 */
TEST_F(RialtoClientRialtoControlIpcSharedMemoryTest, GetSharedMemoryFailure)
{
    int32_t fd = -1;
    uint32_t size = 0U;

    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSharedMemory"), _, _, _, _));

    EXPECT_EQ(m_rialtoControlIpc->getSharedMemory(fd, size), false);

    // Expect disconnect on destruction
    EXPECT_CALL(*m_channelMock, disconnect());
}
