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

#include "RialtoControl.h"
#include "RialtoControlIpcFactoryMock.h"
#include "RialtoControlIpcMock.h"
#include "SharedMemoryManagerClientMock.h"
#include <gtest/gtest.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

class RialtoClientRialtoControlSharedMemoryClientTest : public ::testing::Test
{
protected:
    std::unique_ptr<RialtoControl> rialtoControl;
    std::shared_ptr<StrictMock<RialtoControlIpcFactoryMock>> m_rialtoControlIpcFactoryMock;
    std::shared_ptr<StrictMock<RialtoControlIpcMock>> m_rialtoControlIpcMock;
    StrictMock<SharedMemoryManagerClientMock> *m_sharedMemoryManagerClientMock;

    virtual void SetUp()
    {
        m_rialtoControlIpcFactoryMock = std::make_shared<StrictMock<RialtoControlIpcFactoryMock>>();
        m_rialtoControlIpcMock = std::make_shared<StrictMock<RialtoControlIpcMock>>();

        createRialtoControl();
    }

    virtual void TearDown()
    {
        destroyRialtoControl();

        m_rialtoControlIpcMock.reset();
        m_rialtoControlIpcFactoryMock.reset();
    }

    void createRialtoControl()
    {
        EXPECT_CALL(*m_rialtoControlIpcFactoryMock, getRialtoControlIpc()).WillOnce(Return(m_rialtoControlIpcMock));

        EXPECT_NO_THROW(rialtoControl = std::make_unique<RialtoControl>(m_rialtoControlIpcFactoryMock));
    }

    void destroyRialtoControl() { rialtoControl.reset(); }

    void triggerBufferTermFromInactive()
    {
        int32_t fd;
        uint32_t size = 456U;

        // Create a valid file descriptor
        fd = memfd_create("memfdfile", 0);

        EXPECT_CALL(*m_rialtoControlIpcMock, getSharedMemory(_, _))
            .WillOnce(DoAll(SetArgReferee<0>(fd), SetArgReferee<1>(size), Return(true)));

        EXPECT_EQ(rialtoControl->setApplicationState(ApplicationState::RUNNING), true);

        EXPECT_EQ(rialtoControl->setApplicationState(ApplicationState::INACTIVE), true);

        close(fd);
    }
};

/**
 * Test that RialtoControl can succeesfully register and unregister a shared memory client.
 */
TEST_F(RialtoClientRialtoControlSharedMemoryClientTest, RegisterUnregisterClient)
{
    StrictMock<SharedMemoryManagerClientMock> *clientMock1 = new StrictMock<SharedMemoryManagerClientMock>();

    // Register client
    EXPECT_EQ(rialtoControl->registerClient(clientMock1), true);

    EXPECT_CALL(*clientMock1, notifyBufferTerm());
    triggerBufferTermFromInactive();

    // Unregister client
    EXPECT_EQ(rialtoControl->unregisterClient(clientMock1), true);

    triggerBufferTermFromInactive();

    delete clientMock1;
}

/**
 * Test that RialtoControl can succeesfully  register and unregister multiple shared memory client.
 */
TEST_F(RialtoClientRialtoControlSharedMemoryClientTest, RegisterUnregisterClientMultiple)
{
    StrictMock<SharedMemoryManagerClientMock> *clientMock1 = new StrictMock<SharedMemoryManagerClientMock>();
    StrictMock<SharedMemoryManagerClientMock> *clientMock2 = new StrictMock<SharedMemoryManagerClientMock>();
    StrictMock<SharedMemoryManagerClientMock> *clientMock3 = new StrictMock<SharedMemoryManagerClientMock>();

    // Register clients
    EXPECT_EQ(rialtoControl->registerClient(clientMock1), true);
    EXPECT_EQ(rialtoControl->registerClient(clientMock2), true);
    EXPECT_EQ(rialtoControl->registerClient(clientMock3), true);

    EXPECT_CALL(*clientMock1, notifyBufferTerm());
    EXPECT_CALL(*clientMock2, notifyBufferTerm());
    EXPECT_CALL(*clientMock3, notifyBufferTerm());
    triggerBufferTermFromInactive();

    // Unregister one client
    EXPECT_EQ(rialtoControl->unregisterClient(clientMock2), true);

    EXPECT_CALL(*clientMock1, notifyBufferTerm());
    EXPECT_CALL(*clientMock3, notifyBufferTerm());
    triggerBufferTermFromInactive();

    // Unregister all clients
    EXPECT_EQ(rialtoControl->unregisterClient(clientMock1), true);
    EXPECT_EQ(rialtoControl->unregisterClient(clientMock3), true);
    triggerBufferTermFromInactive();

    delete clientMock1;
    delete clientMock2;
    delete clientMock3;
}

/**
 * Test that registerClient fails for invalid client.
 */
TEST_F(RialtoClientRialtoControlSharedMemoryClientTest, RegisterInvalidClient)
{
    EXPECT_EQ(rialtoControl->registerClient(nullptr), false);
}

/**
 * Test that unregisterClient fails for invalid client.
 */
TEST_F(RialtoClientRialtoControlSharedMemoryClientTest, UnregisterInvalidClient)
{
    StrictMock<SharedMemoryManagerClientMock> *clientMock1 = new StrictMock<SharedMemoryManagerClientMock>();

    EXPECT_EQ(rialtoControl->registerClient(clientMock1), true);

    EXPECT_EQ(rialtoControl->unregisterClient(nullptr), false);

    EXPECT_EQ(rialtoControl->unregisterClient(clientMock1), true);

    delete clientMock1;
}

/**
 * Test that unregisterClient fails for invalid client.
 */
TEST_F(RialtoClientRialtoControlSharedMemoryClientTest, UnregisterClientNotRegistered)
{
    StrictMock<SharedMemoryManagerClientMock> *clientMock1 = new StrictMock<SharedMemoryManagerClientMock>();
    StrictMock<SharedMemoryManagerClientMock> *clientMock2 = new StrictMock<SharedMemoryManagerClientMock>();

    EXPECT_EQ(rialtoControl->registerClient(clientMock2), true);

    EXPECT_EQ(rialtoControl->unregisterClient(clientMock1), false);

    EXPECT_EQ(rialtoControl->unregisterClient(clientMock2), true);

    delete clientMock1;
    delete clientMock2;
}
