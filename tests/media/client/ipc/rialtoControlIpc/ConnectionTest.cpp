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

    expectDisconnect();
    EXPECT_EQ(m_rialtoControlIpc->disconnect(), true);
}

/**
 * Test that RialtoControlIpc::connect connects to the IPC server.
 */
TEST_F(RialtoClientRialtoControlIpcConnectionTest, Connect)
{
    createRialtoControlIpc();

    // Disconnect
    expectDisconnect();
    EXPECT_EQ(m_rialtoControlIpc->disconnect(), true);

    // Connect
    expectCreateChannel();
    expectIpcLoop();

    EXPECT_EQ(m_rialtoControlIpc->connect(), true);

    // Expect disconnect on destruction
    expectDisconnect();
}

/**
 * Test that RialtoControlIpc handles a unexpected disconnect of the ipc channel.
 */
TEST_F(RialtoClientRialtoControlIpcConnectionTest, UnexpectedDisconnect)
{
    // Connect
    expectCreateChannel();

    // Exit the ipc loop, simulates an unexpected disconnect
    long ipcChannelCount = 0;
    EXPECT_CALL(*m_channelMock, process()).InSequence(m_processSeq)
        .WillOnce(Invoke(
            [this, &ipcChannelCount]()
            {
                std::unique_lock<std::mutex> locker(m_eventsLock);
                ipcChannelCount = m_channelMock.use_count();
                m_eventsCond.notify_all();
                return false;
            }));

    EXPECT_NO_THROW(m_rialtoControlIpc = std::make_shared<RialtoControlIpc>(m_channelFactoryMock, m_controllerFactoryMock,
                                                                            m_blockingClosureFactoryMock));

    // Wait for process to set the ipcChannelCount
    {
        std::unique_lock<std::mutex> locker(m_eventsLock);
        m_eventsCond.wait(locker);
    }

    // Wait for shared_ptr to be reset in ipc thread
    while (m_channelMock.use_count() == ipcChannelCount){}

    // On destruction RialtoControlIpc does not disconnect
}
