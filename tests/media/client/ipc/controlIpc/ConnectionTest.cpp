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

#include "ControlIpcTestBase.h"
#include <gtest/gtest.h>

class RialtoClientControlIpcConnectionTest : public ControlIpcTestBase
{
protected:
    virtual void SetUp() { ControlIpcTestBase::SetUp(); }

    virtual void TearDown()
    {
        destroyControlIpc(true);

        ControlIpcTestBase::TearDown();
    }
};

/**
 * Test that ControlIpc::disconnect disconnects from the IPC server.
 */
TEST_F(RialtoClientControlIpcConnectionTest, Disconnect)
{
    createControlIpc();

    EXPECT_CALL(*m_channelMock, disconnect());
    EXPECT_EQ(m_controlIpc->disconnect(), true);
}

/**
 * Test that ControlIpc::connect connects to the IPC server.
 */
TEST_F(RialtoClientControlIpcConnectionTest, Connect)
{
    createControlIpc();

    // Disconnect
    EXPECT_CALL(*m_channelMock, disconnect());
    EXPECT_EQ(m_controlIpc->disconnect(), true);

    // Connect
    EXPECT_CALL(*m_channelFactoryMock, createChannel(m_kRialtoPath)).WillOnce(Return(m_channelMock));
    EXPECT_CALL(*m_channelMock, process()).WillOnce(Return(false));

    EXPECT_EQ(m_controlIpc->connect(), true);

    // Expect disconnect on destruction
    EXPECT_CALL(*m_channelMock, disconnect());
}
