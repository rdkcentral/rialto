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

class RialtoClientRialtoControlSetAppStateTest : public ::testing::Test
{
protected:
    std::unique_ptr<RialtoControl> rialtoControl;
    std::shared_ptr<StrictMock<RialtoControlIpcFactoryMock>> m_rialtoControlIpcFactoryMock;
    std::shared_ptr<StrictMock<RialtoControlIpcMock>> m_rialtoControlIpcMock;
    StrictMock<SharedMemoryManagerClientMock> *m_sharedMemoryManagerClientMock;

    int32_t m_fd;
    uint32_t m_size = 456U;

    virtual void SetUp()
    {
        m_rialtoControlIpcFactoryMock = std::make_shared<StrictMock<RialtoControlIpcFactoryMock>>();
        m_rialtoControlIpcMock = std::make_shared<StrictMock<RialtoControlIpcMock>>();
        m_sharedMemoryManagerClientMock = new StrictMock<SharedMemoryManagerClientMock>();

        // Create a valid file descriptor
        m_fd = memfd_create("memfdfile", 0);

        createRialtoControl();

        // register client mock
        EXPECT_EQ(rialtoControl->registerClient(m_sharedMemoryManagerClientMock), true);
    }

    virtual void TearDown()
    {
        destroyRialtoControl();

        close(m_fd);

        delete m_sharedMemoryManagerClientMock;
        m_rialtoControlIpcMock.reset();
        m_rialtoControlIpcFactoryMock.reset();
    }

    void createRialtoControl()
    {
        EXPECT_CALL(*m_rialtoControlIpcFactoryMock, getRialtoControlIpc()).WillOnce(Return(m_rialtoControlIpcMock));

        EXPECT_NO_THROW(rialtoControl = std::make_unique<RialtoControl>(m_rialtoControlIpcFactoryMock));
    }

    void destroyRialtoControl() { rialtoControl.reset(); }

    void expectTermCalls(ApplicationState currentState)
    {
        if (currentState == ApplicationState::RUNNING)
        {
            EXPECT_CALL(*m_sharedMemoryManagerClientMock, notifyBufferTerm());
        }
    }

    void setRunningAppState()
    {
        // RialtoControl is default initialised to Inactive
        EXPECT_CALL(*m_rialtoControlIpcMock, getSharedMemory(_, _))
            .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));

        EXPECT_EQ(rialtoControl->setApplicationState(ApplicationState::RUNNING), true);
    }
};

/**
 * Test that SetAppState(Running) when RialtoControl in Inactive initialises the shared memory.
 */
TEST_F(RialtoClientRialtoControlSetAppStateTest, InactiveToRunning)
{
    setRunningAppState();

    // Terminate called on destruction
    expectTermCalls(ApplicationState::RUNNING);
}

/**
 * Test that SetAppState(Inactive) when RialtoControl in Running terminates the shared memory.
 */
TEST_F(RialtoClientRialtoControlSetAppStateTest, RunningToInactive)
{
    setRunningAppState();

    EXPECT_CALL(*m_sharedMemoryManagerClientMock, notifyBufferTerm());

    EXPECT_EQ(rialtoControl->setApplicationState(ApplicationState::INACTIVE), true);

    // Terminate called on destruction
    expectTermCalls(ApplicationState::INACTIVE);
}

/**
 * Test that SetAppState when RialtoControl is already in that state succeeds.
 */
TEST_F(RialtoClientRialtoControlSetAppStateTest, SameState)
{
    // RialtoControl is default initialised to Inactive
    EXPECT_EQ(rialtoControl->setApplicationState(ApplicationState::INACTIVE), true);

    // Terminate called on destruction
    expectTermCalls(ApplicationState::INACTIVE);
}

/**
 * Test that SetAppState with an invalid state returns error.
 */
TEST_F(RialtoClientRialtoControlSetAppStateTest, InvalidState)
{
    // RialtoControl is default initialised to Inactive
    EXPECT_EQ(rialtoControl->setApplicationState(ApplicationState::UNKNOWN), false);

    // Terminate called on destruction
    expectTermCalls(ApplicationState::INACTIVE);
}
