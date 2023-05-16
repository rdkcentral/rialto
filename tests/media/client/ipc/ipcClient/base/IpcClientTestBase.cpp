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

#include "IpcClientTestBase.h"
#include "IpcClient.h"
#include <string>

IpcClientTestBase::IpcClientTestBase()
    : m_channelFactoryMock{std::make_shared<StrictMock<ChannelFactoryMock>>()},
      m_channelMock{std::make_shared<StrictMock<ChannelMock>>()},
      m_controllerFactoryMock{std::make_shared<StrictMock<ControllerFactoryMock>>()},
      m_controllerMock{std::make_shared<StrictMock<RpcControllerMock>>()},
      m_blockingClosureFactoryMock{std::make_shared<StrictMock<BlockingClosureFactoryMock>>()},
      m_blockingClosureMock{std::make_shared<StrictMock<BlockingClosureMock>>()}
{
}

IpcClientTestBase::~IpcClientTestBase()
{
    // Destroy StrickMocks
    m_blockingClosureMock.reset();
    m_blockingClosureFactoryMock.reset();
    m_controllerMock.reset();
    m_controllerFactoryMock.reset();
    m_channelMock.reset();
    m_channelFactoryMock.reset();
}

void IpcClientTestBase::createIpcClient()
{
    expectCreateChannel();
    expectIpcLoop();

    EXPECT_NO_THROW(m_sut = std::make_unique<IpcClient>(m_channelFactoryMock, m_controllerFactoryMock,
                                                        m_blockingClosureFactoryMock));

    EXPECT_EQ(m_sut->getChannel().lock(), m_channelMock);
}

void IpcClientTestBase::expectIpcLoop()
{
    EXPECT_CALL(*m_channelMock, process()).InSequence(m_processSeq).WillOnce(Return(true));
    EXPECT_CALL(*m_channelMock, wait(_))
        .WillOnce(Invoke(
            [this](int timeoutMSecs)
            {
                std::unique_lock<std::mutex> locker(m_eventsLock);
                if (!m_disconnected)
                    m_eventsCond.wait(locker);
                return true;
            }));
}

void IpcClientTestBase::failToCreateIpcClient()
{
    EXPECT_CALL(*m_channelFactoryMock, createChannel(m_kRialtoPath)).WillOnce(Return(nullptr));
    EXPECT_THROW(std::make_unique<IpcClient>(m_channelFactoryMock, m_controllerFactoryMock, m_blockingClosureFactoryMock),
                 std::runtime_error);
}

void IpcClientTestBase::createBlockingClosure()
{
    EXPECT_CALL(*m_blockingClosureFactoryMock, createBlockingClosureSemaphore()).WillOnce(Return(m_blockingClosureMock));
    EXPECT_EQ(m_sut->createBlockingClosure(), m_blockingClosureMock);
}

void IpcClientTestBase::createRpcController()
{
    EXPECT_CALL(*m_controllerFactoryMock, create()).WillOnce(Return(m_controllerMock));
    EXPECT_NE(m_sut->createRpcController(), nullptr);
}

void IpcClientTestBase::disconnectIpcClient()
{
    EXPECT_CALL(*m_channelMock, process()).InSequence(m_processSeq).WillOnce(Return(false));
    EXPECT_CALL(*m_channelMock, disconnect())
        .WillOnce(Invoke(
            [this]()
            {
                std::lock_guard<std::mutex> locker(m_eventsLock);
                m_disconnected = true;
                m_eventsCond.notify_all();
            }));
    m_sut.reset();
}

void IpcClientTestBase::expectCreateChannel()
{
    EXPECT_CALL(*m_channelFactoryMock, createChannel(m_kRialtoPath))
        .WillOnce(Invoke(
            [this](const std::string &socketPath)
            {
                std::lock_guard<std::mutex> locker(m_eventsLock);
                m_disconnected = false;
                return m_channelMock;
            }));
}
