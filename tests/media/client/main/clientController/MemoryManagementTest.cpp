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

#include "ClientController.h"
#include "ControlClientMock.h"
#include "ControlIpcFactoryMock.h"
#include "ControlIpcMock.h"
#include <gtest/gtest.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

class ClientControllerMemoryManagementTest : public ::testing::Test
{
protected:
    int32_t m_fd;
    uint32_t m_size = 456U;

    std::shared_ptr<StrictMock<ControlIpcFactoryMock>> m_controlIpcFactoryMock;
    std::shared_ptr<StrictMock<ControlIpcMock>> m_controlIpcMock;
    StrictMock<ControlClientMock> m_controlClientMock;
    std::unique_ptr<ClientController> m_sut;

    ClientControllerMemoryManagementTest()
        : m_controlIpcFactoryMock{std::make_shared<StrictMock<ControlIpcFactoryMock>>()},
          m_controlIpcMock{std::make_shared<StrictMock<ControlIpcMock>>()}
    {
        // Create a valid file descriptor
        m_fd = memfd_create("memfdfile", 0);

        EXPECT_CALL(*m_controlIpcFactoryMock, getControlIpc(_)).WillOnce(Return(m_controlIpcMock));
        EXPECT_CALL(*m_controlIpcMock, registerClient()).WillOnce(Return(true));
        EXPECT_NO_THROW(m_sut = std::make_unique<ClientController>(m_controlIpcFactoryMock));
    }

    ~ClientControllerMemoryManagementTest()
    {
        m_sut.reset();

        m_controlIpcMock.reset();
        m_controlIpcFactoryMock.reset();

        close(m_fd);
    }

    void changeAppState(const ApplicationState &state)
    {
        ASSERT_TRUE(m_sut);
        IControlClient &client = *m_sut;
        client.notifyApplicationState(state);
    }
};

TEST_F(ClientControllerMemoryManagementTest, DoNothingWhenSwitchedToCurrentState)
{
    changeAppState(ApplicationState::UNKNOWN);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());
}

TEST_F(ClientControllerMemoryManagementTest, SwitchToRunningShouldFailWhenIpcReturnsError)
{
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(false)));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());
}

TEST_F(ClientControllerMemoryManagementTest, SwitchToRunningShouldFailWhenIpcReturnsInvalidFdAndSize)
{
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(-1), SetArgReferee<1>(0), Return(true)));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());
}

TEST_F(ClientControllerMemoryManagementTest, SwitchToRunningShouldFailWhenMemoryFailsToMap)
{
    // Send random numbers as a fd and size
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(12345), SetArgReferee<1>(10), Return(true)));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());
}

TEST_F(ClientControllerMemoryManagementTest, SwitchToRunningShouldSucceed)
{
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());
}

TEST_F(ClientControllerMemoryManagementTest, SwitchToInactiveWithoutMemoryTerminationShouldSucceed)
{
    changeAppState(ApplicationState::INACTIVE);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());
}

TEST_F(ClientControllerMemoryManagementTest, SwitchToInactiveWithMemoryTerminationShouldSucceed)
{
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());
    changeAppState(ApplicationState::INACTIVE);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());
}

TEST_F(ClientControllerMemoryManagementTest, SwitchStatesWithClientNotification)
{
    ApplicationState appState;
    m_sut->registerClient(&m_controlClientMock, appState);
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    EXPECT_CALL(m_controlClientMock, notifyApplicationState(ApplicationState::RUNNING));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());
    EXPECT_CALL(m_controlClientMock, notifyApplicationState(ApplicationState::INACTIVE));
    changeAppState(ApplicationState::INACTIVE);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());
}

TEST_F(ClientControllerMemoryManagementTest, SwitchStatesWithoutClientNotification)
{
    ApplicationState appState;
    m_sut->registerClient(&m_controlClientMock, appState);
    m_sut->unregisterClient(&m_controlClientMock);
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());
    changeAppState(ApplicationState::INACTIVE);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());
}

TEST_F(ClientControllerMemoryManagementTest, RegisterClientInRunningState)
{
    constexpr ApplicationState kExpectedAppState{ApplicationState::RUNNING};
    ApplicationState appState;
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    changeAppState(kExpectedAppState);
    m_sut->registerClient(&m_controlClientMock, appState);
    EXPECT_EQ(kExpectedAppState, appState);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());
}

TEST_F(ClientControllerMemoryManagementTest, SwitchToInvalidStateShouldFail)
{
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());
    changeAppState(ApplicationState::UNKNOWN);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());
}
