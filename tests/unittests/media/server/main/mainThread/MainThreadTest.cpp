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

        EXPECT_TRUE(m_mainThread->enqueueTaskAndWait(clientId, task));
    }

    void enqueuePriorityTaskAndWaitOnDummyMock(uint32_t clientId, const std::shared_ptr<DummyMock> &dummyMock)
    {
        auto task = [&, dummyMock]() { dummyMock->mockMethod(); };

        EXPECT_TRUE(m_mainThread->enqueuePriorityTaskAndWait(clientId, task));
    }

    void enqueueTaskOnDummyMock(uint32_t clientId, const std::shared_ptr<DummyMock> &dummyMock)
    {
        auto task = [&, dummyMock]() { dummyMock->mockMethod(); };

        m_mainThread->enqueueTask(clientId, task);
    }

    void unregisterClient(uint32_t clientId)
    {
        auto task = [&, clientId]() { m_mainThread->unregisterClient(clientId); };

        EXPECT_TRUE(m_mainThread->enqueueTaskAndWait(clientId, task));
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
 * Test the factory
 */
TEST_F(MainThreadTests, FactoryCreatesObject)
{
    std::shared_ptr<firebolt::rialto::server::IMainThreadFactory> factory =
        firebolt::rialto::server::IMainThreadFactory::createFactory();
    EXPECT_NE(factory, nullptr);
    EXPECT_NE(factory->getMainThread(), nullptr);
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
    uint32_t clientId4 = m_mainThread->registerClient();

    // Test that the tasks are called in the right order
    InSequence s;

    std::shared_ptr<DummyMock> dummyMock1 = std::make_shared<DummyMock>();
    std::shared_ptr<DummyMock> dummyMock2 = std::make_shared<DummyMock>();
    std::shared_ptr<DummyMock> dummyMock3 = std::make_shared<DummyMock>();
    std::shared_ptr<DummyMock> dummyMock4 = std::make_shared<DummyMock>();
    EXPECT_CALL(*dummyMock2, mockMethod());
    EXPECT_CALL(*dummyMock1, mockMethod());
    EXPECT_CALL(*dummyMock3, mockMethod());
    EXPECT_CALL(*dummyMock4, mockMethod());

    enqueueTaskOnDummyMock(clientId2, dummyMock2);
    enqueueTaskOnDummyMock(clientId1, dummyMock1);
    enqueueTaskAndWaitOnDummyMock(clientId3, dummyMock3);
    enqueuePriorityTaskAndWaitOnDummyMock(clientId4, dummyMock4);

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
    uint32_t clientId4 = m_mainThread->registerClient();

    unregisterClient(clientId3);
    unregisterClient(clientId1);

    std::shared_ptr<DummyMock> dummyMock1 = std::make_shared<DummyMock>();
    std::shared_ptr<DummyMock> dummyMock2 = std::make_shared<DummyMock>();
    std::shared_ptr<DummyMock> dummyMock3 = std::make_shared<DummyMock>();
    std::shared_ptr<DummyMock> dummyMock4 = std::make_shared<DummyMock>();
    EXPECT_CALL(*dummyMock2, mockMethod());

    enqueueTaskOnDummyMock(clientId2, dummyMock2);
    enqueueTaskOnDummyMock(clientId1, dummyMock1);
    enqueueTaskAndWaitOnDummyMock(clientId3, dummyMock3);
    enqueuePriorityTaskAndWaitOnDummyMock(clientId4, dummyMock4);

    unregisterClient(clientId2);
}

/**
 * Test that enqueueTaskAndWait times out when main thread is blocked.
 * AC1: Blocking enqueue returns an error within a bounded timeout.
 */
TEST_F(MainThreadTests, EnqueueTaskAndWaitTimesOutWhenMainThreadBlocked)
{
    m_mainThread = std::make_shared<MainThread>(std::chrono::milliseconds{100});
    uint32_t clientId = m_mainThread->registerClient();

    std::promise<void> release;
    auto releaseFlag = release.get_future().share();

    // Block the main thread with a long-running task
    m_mainThread->enqueueTask(m_mainThreadClientId, [releaseFlag]() { releaseFlag.wait(); });

    // Try to enqueue a task from another thread - should timeout
    auto waiter = std::async(std::launch::async, [&]() {
        auto start = std::chrono::steady_clock::now();
        bool result = m_mainThread->enqueueTaskAndWait(clientId, []() {});
        auto elapsed = std::chrono::steady_clock::now() - start;
        EXPECT_FALSE(result);
        // Verify it actually waited for the timeout period
        EXPECT_GE(elapsed, std::chrono::milliseconds(90));
        EXPECT_LE(elapsed, std::chrono::milliseconds(300));
    });

    waiter.wait();
    release.set_value();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * Test that enqueuePriorityTaskAndWait times out when main thread is blocked.
 * AC1: Priority variant.
 */
TEST_F(MainThreadTests, EnqueuePriorityTaskAndWaitTimesOutWhenMainThreadBlocked)
{
    m_mainThread = std::make_shared<MainThread>(std::chrono::milliseconds{100});
    uint32_t clientId = m_mainThread->registerClient();

    std::promise<void> release;
    auto releaseFlag = release.get_future().share();

    // Block the main thread
    m_mainThread->enqueueTask(m_mainThreadClientId, [releaseFlag]() { releaseFlag.wait(); });

    // Try to enqueue a priority task from another thread - should timeout
    auto waiter = std::async(std::launch::async, [&]() {
        auto start = std::chrono::steady_clock::now();
        bool result = m_mainThread->enqueuePriorityTaskAndWait(clientId, []() {});
        auto elapsed = std::chrono::steady_clock::now() - start;
        EXPECT_FALSE(result);
        EXPECT_GE(elapsed, std::chrono::milliseconds(90));
        EXPECT_LE(elapsed, std::chrono::milliseconds(300));
    });

    waiter.wait();
    release.set_value();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * Test that destructor fails outstanding waiters.
 * AC2: Waiters are released and failed on shutdown.
 */
TEST_F(MainThreadTests, DestructorFailsOutstandingWaiters)
{
    m_mainThread = std::make_shared<MainThread>(std::chrono::milliseconds{100});
    uint32_t clientId = m_mainThread->registerClient();

    std::promise<void> release;
    auto releaseFlag = release.get_future().share();

    // Block the main thread
    m_mainThread->enqueueTask(m_mainThreadClientId, [releaseFlag]() { releaseFlag.wait(); });

    // Start two waiters (normal and priority)
    auto waiter1 = std::async(std::launch::async, [&]() {
        bool result = m_mainThread->enqueueTaskAndWait(clientId, []() {});
        return result;
    });

    auto waiter2 = std::async(std::launch::async, [&]() {
        bool result = m_mainThread->enqueuePriorityTaskAndWait(clientId, []() {});
        return result;
    });

    // Give waiters time to block
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Destroy MainThread - should release all waiters
    m_mainThread.reset();

    // Both waiters should return false (cancelled)
    EXPECT_FALSE(waiter1.get());
    EXPECT_FALSE(waiter2.get());

    release.set_value();
}

/**
 * Test that self-thread call executes inline (no deadlock).
 * AC3: Calls from the main thread run inline.
 */
TEST_F(MainThreadTests, SelfCallExecutesInline)
{
    m_mainThread = std::make_shared<MainThread>();

    bool innerTaskExecuted{false};
    bool outerTaskCompleted{false};

    auto task = [&]() {
        // We're on the main thread, this should execute inline
        bool result = m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, [&]() { innerTaskExecuted = true; });
        EXPECT_TRUE(result);
        outerTaskCompleted = true;
    };

    bool result = m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
    EXPECT_TRUE(result);
    EXPECT_TRUE(innerTaskExecuted);
    EXPECT_TRUE(outerTaskCompleted);
}

/**
 * Test that self-thread priority call executes inline.
 * AC3: Calls from the main thread run inline (priority variant).
 */
TEST_F(MainThreadTests, SelfPriorityCallExecutesInline)
{
    m_mainThread = std::make_shared<MainThread>();

    bool innerTaskExecuted{false};
    bool outerTaskCompleted{false};

    auto task = [&]() {
        bool result = m_mainThread->enqueuePriorityTaskAndWait(m_mainThreadClientId, [&]() { innerTaskExecuted = true; });
        EXPECT_TRUE(result);
        outerTaskCompleted = true;
    };

    bool result = m_mainThread->enqueuePriorityTaskAndWait(m_mainThreadClientId, task);
    EXPECT_TRUE(result);
    EXPECT_TRUE(innerTaskExecuted);
    EXPECT_TRUE(outerTaskCompleted);
}

/**
 * Test that task exceptions are reported and the loop survives.
 */
TEST_F(MainThreadTests, TaskExceptionIsReportedAndLoopSurvives)
{
    m_mainThread = std::make_shared<MainThread>();
    uint32_t clientId = m_mainThread->registerClient();

    bool nextTaskExecuted{false};

    // Enqueue a task that throws
    bool result1 = m_mainThread->enqueueTaskAndWait(clientId, []() { throw std::runtime_error("Test exception"); });
    EXPECT_FALSE(result1); // Should return false due to exception

    // Enqueue another task to verify the loop survived
    bool result2 = m_mainThread->enqueueTaskAndWait(clientId, [&]() { nextTaskExecuted = true; });
    EXPECT_TRUE(result2);
    EXPECT_TRUE(nextTaskExecuted);
}

/**
 * Test that fire-and-forget task exceptions don't kill the loop.
 */
TEST_F(MainThreadTests, FireAndForgetExceptionDoesNotKillLoop)
{
    m_mainThread = std::make_shared<MainThread>();
    uint32_t clientId = m_mainThread->registerClient();

    bool nextTaskExecuted{false};

    // Enqueue a fire-and-forget task that throws
    m_mainThread->enqueueTask(clientId, []() { throw std::runtime_error("Fire and forget exception"); });

    // Give it time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Enqueue another task to verify the loop survived
    bool result = m_mainThread->enqueueTaskAndWait(clientId, [&]() { nextTaskExecuted = true; });
    EXPECT_TRUE(result);
    EXPECT_TRUE(nextTaskExecuted);
}

/**
 * Test concurrent registration - AC4: m_nextClientId access is race-free.
 */
TEST_F(MainThreadTests, ConcurrentRegisterClientsGetUniqueIds)
{
    m_mainThread = std::make_shared<MainThread>();
    std::set<int32_t> allIds;
    std::mutex idsMutex;
    const int kNumThreads = 8;
    const int kIterationsPerThread = 10;

    // Launch multiple threads registering clients concurrently
    std::vector<std::future<void>> futures;
    for (int i = 0; i < kNumThreads; ++i)
    {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            for (int j = 0; j < kIterationsPerThread; ++j)
            {
                int32_t id = m_mainThread->registerClient();
                {
                    std::lock_guard<std::mutex> lock(idsMutex);
                    EXPECT_EQ(allIds.find(id), allIds.end()); // No duplicates
                    allIds.insert(id);
                }
            }
        }));
    }

    // Wait for all threads
    for (auto &future : futures)
    {
        future.wait();
    }

    // Verify total count and uniqueness
    EXPECT_EQ(allIds.size(), kNumThreads * kIterationsPerThread);
}

/**
 * Test that unregister from non-main thread is safe.
 * AC4: m_registeredClients access is race-free.
 */
TEST_F(MainThreadTests, UnregisterFromNonMainThreadIsSafe)
{
    m_mainThread = std::make_shared<MainThread>();
    uint32_t testClientId = m_mainThread->registerClient();

    // Unregister from a different thread
    auto unregister_thread = std::async(std::launch::async, [&]() { m_mainThread->unregisterClient(testClientId); });

    unregister_thread.wait();

    // Task for this client should now return false (unregistered)
    bool taskExecuted{false};
    bool result = m_mainThread->enqueueTaskAndWait(testClientId, [&]() { taskExecuted = true; });

    EXPECT_FALSE(result); // Should fail because client is unregistered
    EXPECT_FALSE(taskExecuted);
}
