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
        std::chrono::duration kMaximumWaitTime{std::chrono::seconds(5)};
        if (!connectionBrokenCallbackCalled)
        {
            m_eventsCond.wait_for(locker, kMaximumWaitTime);
        }
    }

    // On destruction IpcClient does not disconnect
}
