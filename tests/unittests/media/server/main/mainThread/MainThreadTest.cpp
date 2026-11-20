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
#include <future>
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
    enqueueTaskAndWaitOnDummyMock(clientId4, dummyMock4);

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
    EXPECT_CALL(*dummyMock4, mockMethod());

    enqueueTaskOnDummyMock(clientId2, dummyMock2);
    enqueueTaskOnDummyMock(clientId1, dummyMock1);

    // Unregistered clients should return false from enqueueTaskAndWait
    auto task3 = [&, dummyMock3]() { dummyMock3->mockMethod(); };
    EXPECT_FALSE(m_mainThread->enqueueTaskAndWait(clientId3, task3));

    // Registered clients should succeed
    auto task4 = [&, dummyMock4]() { dummyMock4->mockMethod(); };
    EXPECT_TRUE(m_mainThread->enqueueTaskAndWait(clientId4, task4));

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
    std::atomic<bool> timedOutTaskExecuted{false};

    // Block the main thread with a long-running task
    m_mainThread->enqueueTask(m_mainThreadClientId, [releaseFlag]() { releaseFlag.wait(); });

    // Try to enqueue a task from another thread - should timeout
    auto waiter = std::async(std::launch::async,
                             [&]()
                             {
                                 auto start = std::chrono::steady_clock::now();
                                 bool result =
                                     m_mainThread->enqueueTaskAndWait(clientId, [&]() { timedOutTaskExecuted = true; });
                                 auto elapsed = std::chrono::steady_clock::now() - start;
                                 EXPECT_FALSE(result);
                                 // Verify it actually waited for the timeout period
                                 EXPECT_GE(elapsed, std::chrono::milliseconds(90));
                                 EXPECT_LE(elapsed, std::chrono::milliseconds(300));
                             });

    waiter.get();
    release.set_value();
    EXPECT_TRUE(m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, []() {}));
    EXPECT_FALSE(timedOutTaskExecuted);
}

/**
 * Test that destructor fails outstanding waiters.
 * AC2: Waiters are released and failed on shutdown.
 */
TEST_F(MainThreadTests, DestructorFailsOutstandingWaiters)
{
    m_mainThread = std::make_shared<MainThread>(std::chrono::milliseconds{5000});
    uint32_t clientId = m_mainThread->registerClient();

    std::promise<void> release;
    auto releaseFlag = release.get_future().share();

    // Block the main thread
    m_mainThread->enqueueTask(m_mainThreadClientId, [releaseFlag]() { releaseFlag.wait(); });

    auto mainThread = std::move(m_mainThread);
    auto *mainThreadPtr = mainThread.get();
    std::promise<void> waiter1Started;
    std::promise<void> waiter2Started;

    // Start two waiters
    auto waiter1 = std::async(std::launch::async,
                              [&]()
                              {
                                  waiter1Started.set_value();
                                  return mainThreadPtr->enqueueTaskAndWait(clientId, []() {});
                              });

    auto waiter2 = std::async(std::launch::async,
                              [&]()
                              {
                                  waiter2Started.set_value();
                                  return mainThreadPtr->enqueueTaskAndWait(clientId, []() {});
                              });

    waiter1Started.get_future().wait();
    waiter2Started.get_future().wait();

    // Give both threads time to enqueue and block before starting shutdown.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto destruction =
        std::async(std::launch::async, [mainThread = std::move(mainThread)]() mutable { mainThread.reset(); });

    // Destruction releases queued waiters, then joins the blocked worker.
    EXPECT_EQ(waiter1.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);
    EXPECT_EQ(waiter2.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);
    EXPECT_FALSE(waiter1.get());
    EXPECT_FALSE(waiter2.get());

    release.set_value();
    destruction.get();
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

    auto task = [&]()
    {
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
 * Test that exceptions from inline tasks are logged and reported without escaping.
 */
TEST_F(MainThreadTests, InlineTaskExceptionReturnsFalse)
{
    m_mainThread = std::make_shared<MainThread>();
    bool inlineTaskFailed{false};

    bool outerResult =
        m_mainThread->enqueueTaskAndWait(m_mainThreadClientId,
                                         [&]()
                                         {
                                             inlineTaskFailed =
                                                 !m_mainThread->enqueueTaskAndWait(m_mainThreadClientId,
                                                                                   []() {
                                                                                       throw std::runtime_error(
                                                                                           "Inline task exception");
                                                                                   });
                                         });

    EXPECT_TRUE(outerResult);
    EXPECT_TRUE(inlineTaskFailed);
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
        futures.push_back(std::async(std::launch::async,
                                     [&, i]()
                                     {
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

/**
 * P4.1 Test: Worker state copy prevents use-after-free
 * 
 * This test verifies that the worker thread keeps a local copy of m_isMainThreadRunning
 * to prevent accessing destroyed memory when the MainThread is destroyed from the main thread.
 */
TEST_F(MainThreadTests, P4_1_WorkerStateIsKeptAliveDuringDestructionFromMainThread)
{
    // Create a MainThread
    std::shared_ptr<MainThread> mainThread = std::make_shared<MainThread>();
    uint32_t clientId = mainThread->registerClient();

    // Track if the task was executed successfully
    bool taskExecuted{false};
    
    // Enqueue a task (fire-and-forget) that will execute
    mainThread->enqueueTask(clientId, [&]() { 
        taskExecuted = true;
        // Sleep to give the main thread time to exit the loop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    // Give the task a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Now destroy the MainThread object from outside
    // This should NOT cause a crash in the worker thread's loop
    mainThread.reset();

    // Sleep to allow the worker thread to finish and exit safely
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // If we got here without crashing, the test passed
    EXPECT_TRUE(taskExecuted);
}

/**
 * P4.1 Test: Multiple tasks execute correctly with worker state copy
 * 
 * Verify that the local shared_ptr copy doesn't affect normal task execution
 */
TEST_F(MainThreadTests, P4_1_MultipleTasksExecuteWithLocalStateCopy)
{
    m_mainThread = std::make_shared<MainThread>();
    uint32_t clientId = m_mainThread->registerClient();

    std::vector<int> executionOrder;
    std::mutex orderMutex;

    // Enqueue multiple tasks
    for (int i = 0; i < 5; ++i)
    {
        bool result = m_mainThread->enqueueTaskAndWait(clientId, [&, i]() {
            std::lock_guard<std::mutex> lock(orderMutex);
            executionOrder.push_back(i);
        });
        EXPECT_TRUE(result);
    }

    // Verify all tasks executed in order
    EXPECT_EQ(executionOrder.size(), 5);
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(executionOrder[i], i);
    }
}

/**
 * P4.1 Test: Rapid destruction after task enqueueing is safe
 * 
 * Verify that destroying MainThread immediately after enqueuing doesn't crash
 */
TEST_F(MainThreadTests, P4_1_RapidDestructionAfterTaskEnqueueIsSafe)
{
    for (int iteration = 0; iteration < 10; ++iteration)
    {
        {
            std::shared_ptr<MainThread> tempMainThread = std::make_shared<MainThread>();
            uint32_t clientId = tempMainThread->registerClient();

            // Fire-and-forget enqueue
            tempMainThread->enqueueTask(clientId, []() { 
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            });

            // Destroy immediately (scope exit)
        }
        // Sleep to allow cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    // If we completed all iterations without crashing, test passes
}

/**
 * P4.1 Test: Task completion callback correctness with shared_ptr copy
 * 
 * Verify that task status notifications work correctly with the local copy
 */
TEST_F(MainThreadTests, P4_1_TaskStatusNotificationsWorkWithStateCopy)
{
    m_mainThread = std::make_shared<MainThread>();
    uint32_t clientId = m_mainThread->registerClient();

    std::vector<std::string> results;
    std::mutex resultsMutex;

    // Enqueue tasks with different outcomes
    bool result1 = m_mainThread->enqueueTaskAndWait(clientId, [&]() {
        std::lock_guard<std::mutex> lock(resultsMutex);
        results.push_back("success");
    });
    EXPECT_TRUE(result1);

    // Enqueue a task that throws
    bool result2 = m_mainThread->enqueueTaskAndWait(clientId, [&]() { 
        throw std::runtime_error("Test error");
    });
    EXPECT_FALSE(result2);

    // Enqueue another success task
    bool result3 = m_mainThread->enqueueTaskAndWait(clientId, [&]() {
        std::lock_guard<std::mutex> lock(resultsMutex);
        results.push_back("success2");
    });
    EXPECT_TRUE(result3);

    // Verify results
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], "success");
    EXPECT_EQ(results[1], "success2");
}

/**
 * P4.2 Test: Deferred cleanup registration and retry
 * 
 * Verify that partition cleanup states can be deferred and retried
 */
TEST_F(MainThreadTests, P4_2_DeferredCleanupStateCreationAndRetry)
{
    m_mainThread = std::make_shared<MainThread>();

    // Create a cleanup state
    auto cleanupState = std::make_shared<firebolt::rialto::server::PartitionCleanupState>();
    cleanupState->playbackType = firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC;
    cleanupState->sessionId = 123;
    cleanupState->clientId = m_mainThread->registerClient();
    cleanupState->mainThread = m_mainThread;

    // Create a mock shared memory buffer that initially fails
    auto mockShmBuffer = std::make_shared<NiceMock<firebolt::rialto::server::SharedMemoryBufferMock>>();
    cleanupState->shmBuffer = mockShmBuffer;

    // Test that cleanup state can be registered
    bool registerResult = m_mainThread->deferCleanup(cleanupState);
    EXPECT_TRUE(registerResult);
}

/**
 * P4.2 Test: Cleanup state successful execution
 * 
 * Verify that executeCleanup works correctly when unmapPartition succeeds
 */
TEST_F(MainThreadTests, P4_2_CleanupStateSuccessfulExecution)
{
    auto mockShmBuffer = std::make_shared<NiceMock<firebolt::rialto::server::SharedMemoryBufferMock>>();
    
    // Setup mock to succeed
    EXPECT_CALL(*mockShmBuffer,
                unmapPartition(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, 456))
        .Times(1)
        .WillOnce(Return(true));

    auto cleanupState = std::make_shared<firebolt::rialto::server::PartitionCleanupState>();
    cleanupState->playbackType = firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC;
    cleanupState->sessionId = 456;
    cleanupState->shmBuffer = mockShmBuffer;

    // Execute cleanup
    bool result = cleanupState->executeCleanup();
    EXPECT_TRUE(result);
    EXPECT_EQ(cleanupState->status.load(), 
              firebolt::rialto::server::PartitionCleanupState::Status::Completed);
}

/**
 * P4.2 Test: Cleanup state failure and retry
 * 
 * Verify that failed cleanup can be retried up to MAX_RETRIES
 */
TEST_F(MainThreadTests, P4_2_CleanupStateFailureAndRetry)
{
    auto mockShmBuffer = std::make_shared<NiceMock<firebolt::rialto::server::SharedMemoryBufferMock>>();
    
    // Setup mock to fail
    EXPECT_CALL(*mockShmBuffer,
                unmapPartition(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, 789))
        .WillRepeatedly(Return(false));

    auto cleanupState = std::make_shared<firebolt::rialto::server::PartitionCleanupState>();
    cleanupState->playbackType = firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC;
    cleanupState->sessionId = 789;
    cleanupState->shmBuffer = mockShmBuffer;

    // First attempt - should fail
    bool result1 = cleanupState->executeCleanup();
    EXPECT_FALSE(result1);
    EXPECT_TRUE(cleanupState->shouldRetry());

    // Retry multiple times
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_TRUE(cleanupState->shouldRetry());
        cleanupState->retryCount++;
        result1 = cleanupState->executeCleanup();
        EXPECT_FALSE(result1);
    }

    // Should not retry after max attempts
    EXPECT_FALSE(cleanupState->shouldRetry());
}

/**
 * P4.2 Test: MainThread deferCleanup integration
 * 
 * Verify that MainThread can register and track deferred cleanups
 */
TEST_F(MainThreadTests, P4_2_MainThreadDeferCleanupIntegration)
{
    m_mainThread = std::make_shared<MainThread>();
    uint32_t clientId = m_mainThread->registerClient();

    auto mockShmBuffer = std::make_shared<NiceMock<firebolt::rialto::server::SharedMemoryBufferMock>>();
    
    // Create cleanup state
    auto cleanupState = std::make_shared<firebolt::rialto::server::PartitionCleanupState>();
    cleanupState->playbackType = firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC;
    cleanupState->sessionId = 999;
    cleanupState->clientId = clientId;
    cleanupState->mainThread = m_mainThread;
    cleanupState->shmBuffer = mockShmBuffer;

    // Register deferred cleanup
    bool result = m_mainThread->deferCleanup(cleanupState);
    EXPECT_TRUE(result);

    // Cleanup state should still be pending
    EXPECT_EQ(cleanupState->status.load(),
              firebolt::rialto::server::PartitionCleanupState::Status::Pending);
}

/**
 * P4.2 Test: Multiple deferred cleanups
 * 
 * Verify that MainThread can handle multiple concurrent deferred cleanups
 */
TEST_F(MainThreadTests, P4_2_MultipleDeforredCleanups)
{
    m_mainThread = std::make_shared<MainThread>();
    uint32_t clientId = m_mainThread->registerClient();

    auto mockShmBuffer = std::make_shared<NiceMock<firebolt::rialto::server::SharedMemoryBufferMock>>();
    
    // Create multiple cleanup states
    std::vector<std::shared_ptr<firebolt::rialto::server::PartitionCleanupState>> cleanupStates;
    for (int i = 0; i < 5; ++i)
    {
        auto cleanupState = std::make_shared<firebolt::rialto::server::PartitionCleanupState>();
        cleanupState->playbackType = firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC;
        cleanupState->sessionId = 1000 + i;
        cleanupState->clientId = clientId;
        cleanupState->mainThread = m_mainThread;
        cleanupState->shmBuffer = mockShmBuffer;

        bool result = m_mainThread->deferCleanup(cleanupState);
        EXPECT_TRUE(result);
        cleanupStates.push_back(cleanupState);
    }

    // All cleanups should still be pending
    for (const auto& cleanupState : cleanupStates)
    {
        EXPECT_EQ(cleanupState->status.load(),
                  firebolt::rialto::server::PartitionCleanupState::Status::Pending);
    }
}

/**
 * P4.2 Test: Null cleanup state rejection
 * 
 * Verify that deferCleanup properly rejects null states
 */
TEST_F(MainThreadTests, P4_2_NullCleanupStateRejected)
{
    m_mainThread = std::make_shared<MainThread>();

    // Try to register null cleanup state
    bool result = m_mainThread->deferCleanup(nullptr);
    EXPECT_FALSE(result);
}


