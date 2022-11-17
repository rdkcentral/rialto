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

#include "MainThread.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace firebolt::rialto::server;

using ::testing::InSequence;

class DummyMock
{
public:
    MOCK_METHOD(void, mockMethod, (), ());
};

class MainThreadTests : public testing::Test
{
protected:
    std::shared_ptr<IMainThread> m_mainThread;
    const uint32_t m_mainThreadClientId{0};

    void enqueueTaskAndWaitOnDummyMock(uint32_t clientId, const std::shared_ptr<DummyMock> &dummyMock)
    {
        auto task = [&, dummyMock]() { dummyMock->mockMethod(); };

        m_mainThread->enqueueTaskAndWait(clientId, task);
    }

    void enqueueTaskOnDummyMock(uint32_t clientId, const std::shared_ptr<DummyMock> &dummyMock)
    {
        auto task = [&, dummyMock]() { dummyMock->mockMethod(); };

        m_mainThread->enqueueTask(clientId, task);
    }

    void unregisterClient(uint32_t clientId)
    {
        auto task = [&, clientId]() { m_mainThread->unregisterClient(clientId); };

        m_mainThread->enqueueTaskAndWait(clientId, task);
    }
};

/**
 * Test that a MainThread object can be created and destroyed successfully.
 */
TEST_F(MainThreadTests, CreateDestroy)
{
    m_mainThread = std::make_shared<MainThread>();
}

/**
 * Test that a MainThread registeres itself on creation, and can enqueue tasks.
 */
TEST_F(MainThreadTests, RegisterItself)
{
    m_mainThread = std::make_shared<MainThread>();
    std::shared_ptr<DummyMock> dummyMock = std::make_shared<DummyMock>();

    EXPECT_CALL(*dummyMock, mockMethod());
    enqueueTaskAndWaitOnDummyMock(m_mainThreadClientId, dummyMock);
}

/**
 * Test that a MainThread can register, enqueue tasks & unregsister for multiple clients.
 */
TEST_F(MainThreadTests, MultipleClients)
{
    m_mainThread = std::make_shared<MainThread>();

    uint32_t clientId1 = m_mainThread->registerClient();
    uint32_t clientId2 = m_mainThread->registerClient();
    uint32_t clientId3 = m_mainThread->registerClient();

    // Test that the tasks are called in the right order
    InSequence s;

    std::shared_ptr<DummyMock> dummyMock1 = std::make_shared<DummyMock>();
    std::shared_ptr<DummyMock> dummyMock2 = std::make_shared<DummyMock>();
    std::shared_ptr<DummyMock> dummyMock3 = std::make_shared<DummyMock>();
    EXPECT_CALL(*dummyMock2, mockMethod());
    EXPECT_CALL(*dummyMock1, mockMethod());
    EXPECT_CALL(*dummyMock3, mockMethod());

    enqueueTaskOnDummyMock(clientId2, dummyMock2);
    enqueueTaskOnDummyMock(clientId1, dummyMock1);
    enqueueTaskAndWaitOnDummyMock(clientId3, dummyMock3);

    unregisterClient(clientId3);
    unregisterClient(clientId1);
    unregisterClient(clientId2);
}

/**
 * Test that a MainThread ignores tasks from unregistered clients.
 */
TEST_F(MainThreadTests, IgnoreUnregisteredClients)
{
    m_mainThread = std::make_shared<MainThread>();

    uint32_t clientId1 = m_mainThread->registerClient();
    uint32_t clientId2 = m_mainThread->registerClient();
    uint32_t clientId3 = m_mainThread->registerClient();

    unregisterClient(clientId3);
    unregisterClient(clientId1);

    std::shared_ptr<DummyMock> dummyMock1 = std::make_shared<DummyMock>();
    std::shared_ptr<DummyMock> dummyMock2 = std::make_shared<DummyMock>();
    std::shared_ptr<DummyMock> dummyMock3 = std::make_shared<DummyMock>();
    EXPECT_CALL(*dummyMock2, mockMethod());

    enqueueTaskOnDummyMock(clientId2, dummyMock2);
    enqueueTaskOnDummyMock(clientId1, dummyMock1);
    enqueueTaskAndWaitOnDummyMock(clientId3, dummyMock3);

    unregisterClient(clientId2);
}
