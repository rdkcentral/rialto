// /*
//  * If not stated otherwise in this file or this component's LICENSE file the
//  * following copyright and licenses apply:
//  *
//  * Copyright 2022 Sky UK
//  *
//  * Licensed under the Apache License, Version 2.0 (the "License");
//  * you may not use this file except in compliance with the License.
//  * You may obtain a copy of the License at
//  *
//  * http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing, software
//  * distributed under the License is distributed on an "AS IS" BASIS,
//  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  * See the License for the specific language governing permissions and
//  * limitations under the License.
//  */

// TODO(marcin.wojciechowski): To be rewritten

// #include "Control.h"
// #include "ControlIpcFactoryMock.h"
// #include "ControlIpcMock.h"
// #include "SharedMemoryManagerClientMock.h"
// #include <gtest/gtest.h>
// #include <linux/memfd.h>
// #include <sys/mman.h>
// #include <sys/syscall.h>
// #include <unistd.h>

// using namespace firebolt::rialto;
// using namespace firebolt::rialto::client;

// using ::testing::_;
// using ::testing::ByMove;
// using ::testing::DoAll;
// using ::testing::Return;
// using ::testing::SetArgReferee;
// using ::testing::StrictMock;

// class RialtoClientControlSharedMemoryClientTest : public ::testing::Test
// {
// protected:
//     std::unique_ptr<Control> m_control;
//     std::shared_ptr<StrictMock<ControlIpcFactoryMock>> m_controlIpcFactoryMock;
//     std::shared_ptr<StrictMock<ControlIpcMock>> m_controlIpcMock;
//     StrictMock<SharedMemoryManagerClientMock> *m_sharedMemoryManagerClientMock;
//     std::weak_ptr<firebolt::rialto::IControlClient> m_controlClient;

//     virtual void SetUp()
//     {
//         m_controlIpcFactoryMock = std::make_shared<StrictMock<ControlIpcFactoryMock>>();
//         m_controlIpcMock = std::make_shared<StrictMock<ControlIpcMock>>();

//         createControl();
//     }

//     virtual void TearDown()
//     {
//         destroyControl();

//         m_controlIpcMock.reset();
//         m_controlIpcFactoryMock.reset();
//     }

//     void createControl()
//     {
//         EXPECT_CALL(*m_controlIpcFactoryMock, getControlIpc()).WillOnce(Return(m_controlIpcMock));

//         EXPECT_NO_THROW(m_control = std::make_unique<Control>(m_controlIpcFactoryMock));
//     }

//     void destroyControl() { m_control.reset(); }

//     void triggerBufferTermFromInactive()
//     {
//         int32_t fd;
//         uint32_t size = 456U;

//         // Create a valid file descriptor
//         fd = memfd_create("memfdfile", 0);

//         EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
//             .WillOnce(DoAll(SetArgReferee<0>(fd), SetArgReferee<1>(size), Return(true)));

//         EXPECT_EQ(m_control->setControlClient(m_controlClient, ApplicationState::RUNNING), true);

//         EXPECT_EQ(m_control->setControlClient(m_controlClient, ApplicationState::INACTIVE), true);

//         close(fd);
//     }
// };

// /**
//  * Test that Control can succeesfully register and unregister a shared memory client.
//  */
// TEST_F(RialtoClientControlSharedMemoryClientTest, RegisterUnregisterClient)
// {
//     StrictMock<SharedMemoryManagerClientMock> *clientMock1 = new StrictMock<SharedMemoryManagerClientMock>();

//     // Register client
//     EXPECT_EQ(m_control->registerClient(clientMock1), true);

//     EXPECT_CALL(*clientMock1, notifyBufferTerm());
//     triggerBufferTermFromInactive();

//     // Unregister client
//     EXPECT_EQ(m_control->unregisterClient(clientMock1), true);

//     triggerBufferTermFromInactive();

//     delete clientMock1;
// }

// /**
//  * Test that Control can succeesfully  register and unregister multiple shared memory client.
//  */
// TEST_F(RialtoClientControlSharedMemoryClientTest, RegisterUnregisterClientMultiple)
// {
//     StrictMock<SharedMemoryManagerClientMock> *clientMock1 = new StrictMock<SharedMemoryManagerClientMock>();
//     StrictMock<SharedMemoryManagerClientMock> *clientMock2 = new StrictMock<SharedMemoryManagerClientMock>();
//     StrictMock<SharedMemoryManagerClientMock> *clientMock3 = new StrictMock<SharedMemoryManagerClientMock>();

//     // Register clients
//     EXPECT_EQ(m_control->registerClient(clientMock1), true);
//     EXPECT_EQ(m_control->registerClient(clientMock2), true);
//     EXPECT_EQ(m_control->registerClient(clientMock3), true);

//     EXPECT_CALL(*clientMock1, notifyBufferTerm());
//     EXPECT_CALL(*clientMock2, notifyBufferTerm());
//     EXPECT_CALL(*clientMock3, notifyBufferTerm());
//     triggerBufferTermFromInactive();

//     // Unregister one client
//     EXPECT_EQ(m_control->unregisterClient(clientMock2), true);

//     EXPECT_CALL(*clientMock1, notifyBufferTerm());
//     EXPECT_CALL(*clientMock3, notifyBufferTerm());
//     triggerBufferTermFromInactive();

//     // Unregister all clients
//     EXPECT_EQ(m_control->unregisterClient(clientMock1), true);
//     EXPECT_EQ(m_control->unregisterClient(clientMock3), true);
//     triggerBufferTermFromInactive();

//     delete clientMock1;
//     delete clientMock2;
//     delete clientMock3;
// }

// /**
//  * Test that registerClient fails for invalid client.
//  */
// TEST_F(RialtoClientControlSharedMemoryClientTest, RegisterInvalidClient)
// {
//     EXPECT_EQ(m_control->registerClient(nullptr), false);
// }

// /**
//  * Test that unregisterClient fails for invalid client.
//  */
// TEST_F(RialtoClientControlSharedMemoryClientTest, UnregisterInvalidClient)
// {
//     StrictMock<SharedMemoryManagerClientMock> *clientMock1 = new StrictMock<SharedMemoryManagerClientMock>();

//     EXPECT_EQ(m_control->registerClient(clientMock1), true);

//     EXPECT_EQ(m_control->unregisterClient(nullptr), false);

//     EXPECT_EQ(m_control->unregisterClient(clientMock1), true);

//     delete clientMock1;
// }

// /**
//  * Test that unregisterClient fails for invalid client.
//  */
// TEST_F(RialtoClientControlSharedMemoryClientTest, UnregisterClientNotRegistered)
// {
//     StrictMock<SharedMemoryManagerClientMock> *clientMock1 = new StrictMock<SharedMemoryManagerClientMock>();
//     StrictMock<SharedMemoryManagerClientMock> *clientMock2 = new StrictMock<SharedMemoryManagerClientMock>();

//     EXPECT_EQ(m_control->registerClient(clientMock2), true);

//     EXPECT_EQ(m_control->unregisterClient(clientMock1), false);

//     EXPECT_EQ(m_control->unregisterClient(clientMock2), true);

//     delete clientMock1;
//     delete clientMock2;
// }
