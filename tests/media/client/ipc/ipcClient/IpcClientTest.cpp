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
    EXPECT_CALL(*m_channelMock, process())
        .InSequence(m_processSeq)
        .WillOnce(Invoke(
            [this, &ipcChannelCount]()
            {
                std::unique_lock<std::mutex> locker(m_eventsLock);
                ipcChannelCount = m_channelMock.use_count();
                m_eventsCond.notify_all();
                return false;
            }));

    EXPECT_NO_THROW(m_sut = std::make_unique<IpcClient>(m_channelFactoryMock, m_controllerFactoryMock,
                                                        m_blockingClosureFactoryMock));

    // Wait for process to set the ipcChannelCount
    {
        std::unique_lock<std::mutex> locker(m_eventsLock);
        m_eventsCond.wait(locker);
    }

    // Wait for shared_ptr to be reset in ipc thread
    while (m_channelMock.use_count() == ipcChannelCount)
    {
    }

    // On destruction IpcClient does not disconnect
}
