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

#include "RialtoControlIpcTestBase.h"
#include <memory>
#include <string>

void RialtoControlIpcTestBase::SetUp() // NOLINT(build/function_format)
{
    // Create StrictMocks
    m_channelFactoryMock = std::make_shared<StrictMock<ChannelFactoryMock>>();
    m_channelMock = std::make_shared<StrictMock<ChannelMock>>();
    m_controllerFactoryMock = std::make_shared<StrictMock<ControllerFactoryMock>>();
    m_controllerMock = std::make_shared<StrictMock<RpcControllerMock>>();
    m_blockingClosureFactoryMock = std::make_shared<StrictMock<BlockingClosureFactoryMock>>();
    m_blockingClosureMock = std::make_shared<StrictMock<BlockingClosureMock>>();
}

void RialtoControlIpcTestBase::TearDown() // NOLINT(build/function_format)
{
    // Destroy StrickMocks
    m_blockingClosureMock.reset();
    m_blockingClosureFactoryMock.reset();
    m_controllerMock.reset();
    m_controllerFactoryMock.reset();
    m_channelMock.reset();
    m_channelFactoryMock.reset();
}

void RialtoControlIpcTestBase::createRialtoControlIpc()
{
    expectCreateChannel();
    expectIpcLoop();

    EXPECT_NO_THROW(m_rialtoControlIpc = std::make_shared<RialtoControlIpc>(m_channelFactoryMock, m_controllerFactoryMock,
                                                                            m_blockingClosureFactoryMock));
}

void RialtoControlIpcTestBase::expectIpcLoop()
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

void RialtoControlIpcTestBase::destroyRialtoControlIpc(bool alreadyDisconnected)
{
    if (!alreadyDisconnected)
    {
        expectDisconnect();
    }

    m_rialtoControlIpc.reset();
}

void RialtoControlIpcTestBase::expectIpcApiCallSuccess()
{
    EXPECT_CALL(*m_controllerFactoryMock, create()).WillOnce(Return(m_controllerMock));
    EXPECT_CALL(*m_blockingClosureFactoryMock, createBlockingClosureSemaphore()).WillOnce(Return(m_blockingClosureMock));

    EXPECT_CALL(*m_blockingClosureMock, wait()).RetiresOnSaturation();
    EXPECT_CALL(*m_controllerMock, Failed()).WillOnce(Return(false)).RetiresOnSaturation();
}

void RialtoControlIpcTestBase::expectIpcApiCallFailure()
{
    EXPECT_CALL(*m_controllerFactoryMock, create()).WillOnce(Return(m_controllerMock));
    EXPECT_CALL(*m_blockingClosureFactoryMock, createBlockingClosureSemaphore()).WillOnce(Return(m_blockingClosureMock));

    EXPECT_CALL(*m_blockingClosureMock, wait()).RetiresOnSaturation();
    EXPECT_CALL(*m_controllerMock, Failed()).WillOnce(Return(true)).RetiresOnSaturation();
    EXPECT_CALL(*m_controllerMock, ErrorText()).WillOnce(Return("Failed for some reason...")).RetiresOnSaturation();
}

void RialtoControlIpcTestBase::expectDisconnect()
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
}

void RialtoControlIpcTestBase::expectCreateChannel()
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
