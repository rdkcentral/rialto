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

#include <gtest/gtest.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "ClientController.h"
#include "ControlClientMock.h"
#include "ControlIpcFactoryMock.h"
#include "ControlIpcMock.h"
#include "PrivateMetricsIpcFactoryMock.h"
#include "PrivateMetricsIpcMock.h"

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
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
    std::shared_ptr<StrictMock<PrivateMetricsIpcFactoryMock>> m_privateMetricsIpcFactoryMock;
    std::shared_ptr<NiceMock<PrivateMetricsIpcMock>> m_privateMetricsIpcMock;
    std::shared_ptr<StrictMock<ControlClientMock>> m_controlClientMock;
    std::unique_ptr<ClientController> m_sut;

    ClientControllerMemoryManagementTest()
        : m_controlIpcFactoryMock{std::make_shared<StrictMock<ControlIpcFactoryMock>>()},
          m_controlIpcMock{std::make_shared<StrictMock<ControlIpcMock>>()},
          m_privateMetricsIpcFactoryMock{std::make_shared<StrictMock<PrivateMetricsIpcFactoryMock>>()},
          m_privateMetricsIpcMock{std::make_shared<NiceMock<PrivateMetricsIpcMock>>()},
          m_controlClientMock{std::make_shared<StrictMock<ControlClientMock>>()}
    {
        // Create a valid file descriptor
        m_fd = memfd_create("memfdfile", 0);

        EXPECT_CALL(*m_controlIpcFactoryMock, createControlIpc(_)).WillOnce(Return(m_controlIpcMock));
        EXPECT_CALL(*m_privateMetricsIpcFactoryMock, createPrivateMetricsIpc(_)).WillOnce(Return(m_privateMetricsIpcMock));
        EXPECT_NO_THROW(
            m_sut = std::make_unique<ClientController>(m_controlIpcFactoryMock, m_privateMetricsIpcFactoryMock));
    }

    ~ClientControllerMemoryManagementTest()
    {
        m_sut.reset();

        m_controlIpcMock.reset();
        m_controlIpcFactoryMock.reset();
        m_privateMetricsIpcMock.reset();
        m_privateMetricsIpcFactoryMock.reset();

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

TEST_F(ClientControllerMemoryManagementTest, RegisterClientFailureWhenClientIsNull)
{
    ApplicationState appState;
    EXPECT_FALSE(m_sut->registerClient(std::weak_ptr<IControlClient>{}, appState));
}

TEST_F(ClientControllerMemoryManagementTest, RegisterClientFailureWhenIpcFails)
{
    ApplicationState appState;
    EXPECT_CALL(*m_controlIpcMock, registerClient()).WillOnce(Return(false));
    EXPECT_FALSE(m_sut->registerClient(m_controlClientMock, appState));
}

TEST_F(ClientControllerMemoryManagementTest, RegisterClientSuccess)
{
    ApplicationState appState;
    EXPECT_CALL(*m_controlIpcMock, registerClient()).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);
}

TEST_F(ClientControllerMemoryManagementTest, RegisterClientSuccessTwiceAtTheSameTime)
{
    ApplicationState appState;
    // Client should be registered only once
    EXPECT_CALL(*m_controlIpcMock, registerClient()).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);
    EXPECT_TRUE(m_sut->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);
}

TEST_F(ClientControllerMemoryManagementTest, SwitchStatesWithClientNotification)
{
    ApplicationState appState;
    EXPECT_CALL(*m_controlIpcMock, registerClient()).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::RUNNING));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::INACTIVE));
    changeAppState(ApplicationState::INACTIVE);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());
}

TEST_F(ClientControllerMemoryManagementTest, SwitchStatesWithoutClientNotification)
{
    ApplicationState appState;
    EXPECT_CALL(*m_controlIpcMock, registerClient()).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->registerClient(m_controlClientMock, appState));
    m_sut->unregisterClient(m_controlClientMock);
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
    // Register first client
    constexpr ApplicationState kExpectedAppState{ApplicationState::RUNNING};
    ApplicationState appState;
    EXPECT_CALL(*m_controlIpcMock, registerClient()).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(kExpectedAppState));
    changeAppState(kExpectedAppState);

    // Second client registration should skip sending registerClient via IPC
    EXPECT_TRUE(m_sut->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(kExpectedAppState, appState);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());
}

TEST_F(ClientControllerMemoryManagementTest, RegisterClientIpcOnceAfterSwitchToInactive)
{
    // Register first client in Running state
    ApplicationState appState;
    EXPECT_CALL(*m_controlIpcMock, registerClient()).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::RUNNING));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());

    // Switch state to Inactive
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::INACTIVE));
    changeAppState(ApplicationState::INACTIVE);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());

    // Second client registration should skip sending registerClient via IPC and return Inactive state
    EXPECT_TRUE(m_sut->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::INACTIVE, appState);
}

TEST_F(ClientControllerMemoryManagementTest, SendRegisterClientIpcTwiceAfterSwitchToUnknown)
{
    // Register first client in Running state
    ApplicationState appState;
    EXPECT_CALL(*m_controlIpcMock, registerClient()).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::RUNNING));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());

    // Switch state to Unknown
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::UNKNOWN));
    changeAppState(ApplicationState::UNKNOWN);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());

    // Second client registration should send registerClient again
    EXPECT_CALL(*m_controlIpcMock, registerClient()).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);
}

TEST_F(ClientControllerMemoryManagementTest, SwitchToUnknown)
{
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    changeAppState(ApplicationState::RUNNING);
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle());
    EXPECT_NE(nullptr, m_sut->getSharedMemoryHandle()->getShm());
    changeAppState(ApplicationState::UNKNOWN);
    EXPECT_EQ(nullptr, m_sut->getSharedMemoryHandle());
}

/**
 * Regression test: unsubscribeSharedMemoryHandle() must not return while a notification
 * that already captured this observer is still invoking it, otherwise the caller can
 * tear itself down and leave the in-flight callback accessing destroyed state.
 */
TEST_F(ClientControllerMemoryManagementTest, UnsubscribeBlocksUntilInFlightNotificationCompletes)
{
    std::mutex cbMutex;
    std::condition_variable cbCond;
    bool callbackStarted{false};
    bool releaseCallback{false};
    std::atomic<int> invocationCount{0};

    const int kToken = m_sut->subscribeSharedMemoryHandle(
        [&](const std::shared_ptr<ISharedMemoryHandle> &)
        {
            // Ignore the synchronous initial delivery made by subscribeSharedMemoryHandle()
            // itself; only the later notification (triggered below) should block.
            if (invocationCount.fetch_add(1) == 0)
            {
                return;
            }
            std::unique_lock<std::mutex> lock(cbMutex);
            callbackStarted = true;
            cbCond.notify_all();
            cbCond.wait(lock, [&] { return releaseCallback; });
        });

    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));

    std::thread notifyThread([this] { changeAppState(ApplicationState::RUNNING); });

    // Wait until the slow observer callback is actively executing inside the notification.
    {
        std::unique_lock<std::mutex> lock(cbMutex);
        cbCond.wait(lock, [&] { return callbackStarted; });
    }

    std::atomic<bool> unsubscribeReturned{false};
    std::thread unsubscribeThread(
        [&]
        {
            m_sut->unsubscribeSharedMemoryHandle(kToken);
            unsubscribeReturned.store(true);
        });

    // The in-flight callback has not been released yet, so unsubscribe must still be blocked.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(unsubscribeReturned.load());

    {
        std::lock_guard<std::mutex> lock(cbMutex);
        releaseCallback = true;
    }
    cbCond.notify_all();

    notifyThread.join();
    unsubscribeThread.join();
    EXPECT_TRUE(unsubscribeReturned.load());
}

/**
 * Regression test: an observer must be able to unsubscribe itself from within its own
 * notification callback without deadlocking, and must not be invoked again afterwards.
 */
TEST_F(ClientControllerMemoryManagementTest, ObserverCanUnsubscribeItselfDuringNotificationWithoutDeadlock)
{
    int token = -1;
    int invocationCount = 0;

    token = m_sut->subscribeSharedMemoryHandle(
        [&](const std::shared_ptr<ISharedMemoryHandle> &)
        {
            ++invocationCount;
            if (invocationCount == 1)
            {
                // Ignore the synchronous initial delivery; token isn't assigned yet at that point.
                return;
            }
            m_sut->unsubscribeSharedMemoryHandle(token);
        });

    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));

    // Triggers notifySharedMemoryHandleObservers(), whose callback unsubscribes itself.
    changeAppState(ApplicationState::RUNNING);
    EXPECT_EQ(2, invocationCount);

    // Further changes must not reach the now-unsubscribed observer.
    changeAppState(ApplicationState::INACTIVE);
    EXPECT_EQ(2, invocationCount);
}

TEST_F(ClientControllerMemoryManagementTest, LateSubscriberReceivesCurrentHandleImmediately)
{
    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    changeAppState(ApplicationState::RUNNING);

    int invocationCount = 0;
    std::shared_ptr<ISharedMemoryHandle> delivered;
    const int kToken = m_sut->subscribeSharedMemoryHandle(
        [&](const std::shared_ptr<ISharedMemoryHandle> &handle)
        {
            ++invocationCount;
            delivered = handle;
        });

    m_sut->unsubscribeSharedMemoryHandle(kToken);

    EXPECT_EQ(1, invocationCount);
    ASSERT_NE(nullptr, delivered);
    EXPECT_EQ(m_sut->getSharedMemoryHandle(), delivered);
}

TEST_F(ClientControllerMemoryManagementTest, ObserverReceivesNullHandleOnInactiveAndUnknown)
{
    const auto kDeliveries = std::make_shared<std::vector<std::shared_ptr<ISharedMemoryHandle>>>();
    auto &delivered = *kDeliveries;
    const int kToken = m_sut->subscribeSharedMemoryHandle(
        [kDeliveries](const std::shared_ptr<ISharedMemoryHandle> &handle) { kDeliveries->push_back(handle); });
    ASSERT_EQ(1u, delivered.size());
    EXPECT_EQ(nullptr, delivered[0]);

    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    changeAppState(ApplicationState::RUNNING);
    ASSERT_EQ(2u, delivered.size());
    EXPECT_NE(nullptr, delivered[1]);

    changeAppState(ApplicationState::INACTIVE);
    ASSERT_EQ(3u, delivered.size());
    EXPECT_EQ(nullptr, delivered[2]);

    changeAppState(ApplicationState::UNKNOWN);
    ASSERT_EQ(4u, delivered.size());
    EXPECT_EQ(nullptr, delivered[3]);

    m_sut->unsubscribeSharedMemoryHandle(kToken);
}

TEST_F(ClientControllerMemoryManagementTest, ReplacementHandleDeliveredOnResume)
{
    const int32_t kSecondFd = dup(m_fd);
    ASSERT_NE(-1, kSecondFd);

    const auto kDeliveries = std::make_shared<std::vector<std::shared_ptr<ISharedMemoryHandle>>>();
    auto &delivered = *kDeliveries;
    const int kToken = m_sut->subscribeSharedMemoryHandle(
        [kDeliveries](const std::shared_ptr<ISharedMemoryHandle> &handle) { kDeliveries->push_back(handle); });

    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    changeAppState(ApplicationState::RUNNING);
    ASSERT_EQ(2u, delivered.size());
    const std::shared_ptr<ISharedMemoryHandle> kFirstHandle = delivered[1];
    ASSERT_NE(nullptr, kFirstHandle);

    changeAppState(ApplicationState::INACTIVE);
    ASSERT_EQ(3u, delivered.size());

    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(kSecondFd), SetArgReferee<1>(m_size), Return(true)));
    changeAppState(ApplicationState::RUNNING);
    ASSERT_EQ(4u, delivered.size());
    ASSERT_NE(nullptr, delivered[3]);
    EXPECT_NE(kFirstHandle, delivered[3]);

    m_sut->unsubscribeSharedMemoryHandle(kToken);
}

TEST_F(ClientControllerMemoryManagementTest, NoDeliveryAfterUnsubscribe)
{
    int invocationCount = 0;
    const int kToken = m_sut->subscribeSharedMemoryHandle(
        [&](const std::shared_ptr<ISharedMemoryHandle> &) { ++invocationCount; });
    EXPECT_EQ(1, invocationCount);

    m_sut->unsubscribeSharedMemoryHandle(kToken);

    EXPECT_CALL(*m_controlIpcMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(m_fd), SetArgReferee<1>(m_size), Return(true)));
    changeAppState(ApplicationState::RUNNING);
    changeAppState(ApplicationState::INACTIVE);

    EXPECT_EQ(1, invocationCount);
}
