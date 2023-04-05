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

#include "Control.h"
#include "ControlIpcFactoryMock.h"
#include "ControlIpcMock.h"
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

class RialtoClientControlSetAppStateTest : public ::testing::Test
{
protected:
    std::unique_ptr<Control> m_control;
    std::shared_ptr<StrictMock<ControlIpcFactoryMock>> m_controlIpcFactoryMock;
    std::shared_ptr<StrictMock<ControlIpcMock>> m_controlIpcMock;
    StrictMock<SharedMemoryManagerClientMock> *m_sharedMemoryManagerClientMock;

    int32_t m_fd;
    uint32_t m_size = 456U;

    virtual void SetUp()
    {
        m_controlIpcFactoryMock = std::make_shared<StrictMock<ControlIpcFactoryMock>>();
        m_controlIpcMock = std::make_shared<StrictMock<ControlIpcMock>>();
        m_sharedMemoryManagerClientMock = new StrictMock<SharedMemoryManagerClientMock>();

        // Create a valid file descriptor
        m_fd = memfd_create("memfdfile", 0);

        createControl();

        // register client mock
        EXPECT_EQ(m_control->registerClient(m_sharedMemoryManagerClientMock), true);
    }

    virtual void TearDown()
    {
        destroyControl();

        close(m_fd);

        delete m_sharedMemoryManagerClientMock;
        m_controlIpcMock.reset();
        m_controlIpcFactoryMock.reset();
    }

    void createControl()
    {
        EXPECT_CALL(*m_controlIpcFactoryMock, getControlIpc()).WillOnce(Return(m_controlIpcMock));

        EXPECT_NO_THROW(m_control = std::make_unique<Control>(m_controlIpcFactoryMock));
    }

    void destroyControl() { m_control.reset(); }

    void expectTermCalls(ApplicationState currentState)
    {
        if (currentState == ApplicationState::RUNNING)
        {
            EXPECT_CALL(*m_sharedMemoryManagerClientMock, notifyBufferTerm());
        }
    }

    void setRunningAppState()
    {
        // Control is default initialised to Inactive
        EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
            .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));

        EXPECT_EQ(m_control->setApplicationState(ApplicationState::RUNNING), true);
    }
};

/**
 * Test that SetAppState(Running) when Control in Inactive initialises the shared memory.
 */
TEST_F(RialtoClientControlSetAppStateTest, InactiveToRunning)
{
    setRunningAppState();

    // Terminate called on destruction
    expectTermCalls(ApplicationState::RUNNING);
}

/**
 * Test that SetAppState(Inactive) when Control in Running terminates the shared memory.
 */
TEST_F(RialtoClientControlSetAppStateTest, RunningToInactive)
{
    setRunningAppState();

    EXPECT_CALL(*m_sharedMemoryManagerClientMock, notifyBufferTerm());

    EXPECT_EQ(m_control->setApplicationState(ApplicationState::INACTIVE), true);

    // Terminate called on destruction
    expectTermCalls(ApplicationState::INACTIVE);
}

/**
 * Test that SetAppState when Control is already in that state succeeds.
 */
TEST_F(RialtoClientControlSetAppStateTest, SameState)
{
    // Control is default initialised to Inactive
    EXPECT_EQ(m_control->setApplicationState(ApplicationState::INACTIVE), true);

    // Terminate called on destruction
    expectTermCalls(ApplicationState::INACTIVE);
}

/**
 * Test that SetAppState with an invalid state returns error.
 */
TEST_F(RialtoClientControlSetAppStateTest, InvalidState)
{
    // Control is default initialised to Inactive
    EXPECT_EQ(m_control->setApplicationState(ApplicationState::UNKNOWN), false);

    // Terminate called on destruction
    expectTermCalls(ApplicationState::INACTIVE);
}
