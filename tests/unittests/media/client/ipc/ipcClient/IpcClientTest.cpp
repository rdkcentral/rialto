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

#include "IpcClient.h"
#include "ConnectionObserverMock.h"
#include "base/IpcClientTestBase.h"
#include <memory>

class IpcClientTest : public IpcClientTestBase
{
public:
    IpcClientTest() = default;
    ~IpcClientTest() override = default;
};

TEST_F(IpcClientTest, successfulCreate)
{
    createIpcClient();
    disconnectIpcClient();
}

TEST_F(IpcClientTest, createBlockingClosure)
{
    createIpcClient();
    createBlockingClosure();
    disconnectIpcClient();
}

TEST_F(IpcClientTest, createRpcController)
{
    createIpcClient();
    createRpcController();
    disconnectIpcClient();
}

TEST_F(IpcClientTest, createFailureDueToChannelCreationProblem)
{
    failToCreateIpcClient();
}

/**
 * Test that reconnect does nothing if the channel is still connected. Several client threads can
 * notice a broken connection at the same time, only the first of them has to restore it.
 */
TEST_F(IpcClientTest, reconnectWhenChannelStillConnected)
{
    createIpcClient();

    EXPECT_CALL(*m_channelMock, isConnected()).WillOnce(Return(true)).RetiresOnSaturation();

    EXPECT_TRUE(m_sut->reconnect());

    disconnectIpcClient();
}

/**
 * Test that reconnect closes the broken connection and establishes a new one. The ipc thread of the
 * old connection has to be joined before the new one is started.
 */
TEST_F(IpcClientTest, reconnectSuccess)
{
    // Two connections are established, the initial one and the one built by reconnect()
    expectCreateChannel();
    expectCreateChannel();
    expectIpcLoopTwice();

    EXPECT_NO_THROW(m_sut = std::make_unique<IpcClient>(m_channelFactoryMock, m_controllerFactoryMock,
                                                        m_blockingClosureFactoryMock));

    EXPECT_CALL(*m_channelMock, isConnected()).WillOnce(Return(false)).RetiresOnSaturation();
    expectDisconnectChannel();

    EXPECT_TRUE(m_sut->reconnect());

    EXPECT_EQ(m_sut->getChannel().lock(), m_channelMock);

    disconnectIpcClient();
}

/**
 * Test that reconnect fails when called from the ipc thread. Reconnection joins that thread, so
 * performing it there would deadlock.
 */
TEST_F(IpcClientTest, reconnectFromIpcThreadFailure)
{
    expectCreateChannel();

    bool sutCreated{false};
    bool reconnectDone{false};
    bool reconnectResult{true};

    EXPECT_CALL(*m_channelMock, process())
        .WillOnce(Invoke(
            [&]()
            {
                std::unique_lock<std::mutex> locker(m_eventsLock);

                // Wait for the constructor to return, so that the sut can be used here
                m_eventsCond.wait(locker, [&]() { return sutCreated; });
                locker.unlock();

                reconnectResult = m_sut->reconnect();

                locker.lock();
                reconnectDone = true;
                m_eventsCond.notify_all();
                locker.unlock();

                // Exit the ipc loop, simulates an unexpected disconnect
                return false;
            }));

    // The ipc thread destroys the channel on the unexpected disconnect, so it is a race whether the
    // client still has one to disconnect when it is destructed
    EXPECT_CALL(*m_channelMock, disconnect()).Times(AtMost(1));

    EXPECT_NO_THROW(m_sut = std::make_unique<IpcClient>(m_channelFactoryMock, m_controllerFactoryMock,
                                                        m_blockingClosureFactoryMock));
    {
        std::lock_guard<std::mutex> locker(m_eventsLock);
        sutCreated = true;
    }
    m_eventsCond.notify_all();

    // The sut can only be destroyed once the ipc thread has finished using it
    {
        std::unique_lock<std::mutex> locker(m_eventsLock);
        m_eventsCond.wait(locker, [&]() { return reconnectDone; });
    }

    EXPECT_FALSE(reconnectResult);

    m_sut.reset();
}

TEST_F(IpcClientTest, UnexpectedDisconnect)
{
    // Connect
    expectCreateChannel();

    // Exit the ipc loop, simulates an unexpected disconnect
    int32_t ipcChannelCount = 0;
    bool isProcessed = false;
    EXPECT_CALL(*m_channelMock, process())
        .WillOnce(Invoke(
            [this, &ipcChannelCount, &isProcessed]()
            {
                std::unique_lock<std::mutex> locker(m_eventsLock);
                ipcChannelCount = m_channelMock.use_count();
                isProcessed = true;
                m_eventsCond.notify_all();
                return false;
            }));

    EXPECT_NO_THROW(m_sut = std::make_unique<IpcClient>(m_channelFactoryMock, m_controllerFactoryMock,
                                                        m_blockingClosureFactoryMock));

    // Wait for process to set the ipcChannelCount
    {
        std::unique_lock<std::mutex> locker(m_eventsLock);
        if (!isProcessed)
        {
            m_eventsCond.wait(locker);
        }
    }

    // Wait for shared_ptr to be reset in ipc thread
    while (m_channelMock.use_count() == ipcChannelCount)
    {
    }

    // On destruction IpcClient does not disconnect
}

TEST_F(IpcClientTest, UnexpectedDisconnectWithNotification)
{
    // Connect
    expectCreateChannel();

    // Exit the ipc loop, simulates an unexpected disconnect
    bool connectionBrokenCallbackCalled = false;
    auto connectionObserverMock{std::make_shared<StrictMock<firebolt::rialto::client::ConnectionObserverMock>>()};
    EXPECT_CALL(*m_channelMock, process())
        .WillOnce(Invoke(
            [this, &connectionObserverMock]()
            {
                m_sut->registerConnectionObserver(connectionObserverMock);
                return false;
            }));

    EXPECT_CALL(*connectionObserverMock, onConnectionBroken())
        .WillOnce(Invoke(
            [this, &connectionBrokenCallbackCalled]()
            {
                std::unique_lock<std::mutex> locker(m_eventsLock);
                connectionBrokenCallbackCalled = true;
                m_eventsCond.notify_all();
            }));
    EXPECT_NO_THROW(m_sut = std::make_unique<IpcClient>(m_channelFactoryMock, m_controllerFactoryMock,
                                                        m_blockingClosureFactoryMock));

    // Wait for the callback
    {
        std::unique_lock<std::mutex> locker(m_eventsLock);
        constexpr std::chrono::duration kMaximumWaitTime{std::chrono::seconds(5)};
        if (!connectionBrokenCallbackCalled)
        {
            // The EXPECT_CALL() above will catch a timeout if one occurs
            m_eventsCond.wait_for(locker, kMaximumWaitTime);
        }
    }

    // On destruction IpcClient does not disconnect
}
