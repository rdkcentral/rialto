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

class RialtoClientRialtoControlIpcConnectionTest : public RialtoControlIpcTestBase
{
protected:
    virtual void SetUp() { RialtoControlIpcTestBase::SetUp(); }

    virtual void TearDown()
    {
        destroyRialtoControlIpc(true);

        RialtoControlIpcTestBase::TearDown();
    }
};

/**
 * Test that RialtoControlIpc::disconnect disconnects from the IPC server.
 */
TEST_F(RialtoClientRialtoControlIpcConnectionTest, Disconnect)
{
    createRialtoControlIpc();

    EXPECT_CALL(*m_channelMock, disconnect());
    EXPECT_EQ(m_rialtoControlIpc->disconnect(), true);
}

/**
 * Test that RialtoControlIpc::connect connects to the IPC server.
 */
TEST_F(RialtoClientRialtoControlIpcConnectionTest, Connect)
{
    createRialtoControlIpc();

    // Disconnect
    EXPECT_CALL(*m_channelMock, disconnect());
    EXPECT_EQ(m_rialtoControlIpc->disconnect(), true);

    // Connect
    EXPECT_CALL(*m_channelFactoryMock, createChannel(m_kRialtoPath)).WillOnce(Return(m_channelMock));
    EXPECT_CALL(*m_channelMock, process()).WillOnce(Return(false));

    EXPECT_EQ(m_rialtoControlIpc->connect(), true);

    // Expect disconnect on destruction
    EXPECT_CALL(*m_channelMock, disconnect());
}
