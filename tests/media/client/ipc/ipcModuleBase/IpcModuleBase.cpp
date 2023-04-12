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

#include "IpcModuleBase.h"
#include <memory>
#include <string>
#include <utility>

IpcModuleBase::IpcModuleBase()
    : m_channelMock{std::make_shared<StrictMock<ChannelMock>>()},
      m_blockingClosureMock{std::make_shared<StrictMock<BlockingClosureMock>>()},
      m_controllerMock{std::make_shared<StrictMock<RpcControllerMock>>()}
{
}

void IpcModuleBase::expectInitIpc()
{
    expectAttachChannel();
}

void IpcModuleBase::expectInitIpcFailure() {}

void IpcModuleBase::expectInitIpcButAttachChannelFailure()
{
    EXPECT_CALL(m_ipcClientMock, getChannel()).WillOnce(Return(nullptr)).RetiresOnSaturation();
}

void IpcModuleBase::expectAttachChannel()
{
    EXPECT_CALL(m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(true)).RetiresOnSaturation();
}

void IpcModuleBase::expectIpcApiCallSuccess()
{
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(true)).RetiresOnSaturation();

    EXPECT_CALL(m_ipcClientMock, createRpcController()).WillOnce(Return(m_controllerMock)).RetiresOnSaturation();
    EXPECT_CALL(m_ipcClientMock, createBlockingClosure()).WillOnce(Return(m_blockingClosureMock)).RetiresOnSaturation();

    EXPECT_CALL(*m_blockingClosureMock, wait()).RetiresOnSaturation();
    EXPECT_CALL(*m_controllerMock, Failed()).WillOnce(Return(false)).RetiresOnSaturation();
}

void IpcModuleBase::expectIpcApiCallFailure()
{
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(true)).RetiresOnSaturation();

    EXPECT_CALL(m_ipcClientMock, createRpcController()).WillOnce(Return(m_controllerMock)).RetiresOnSaturation();
    EXPECT_CALL(m_ipcClientMock, createBlockingClosure()).WillOnce(Return(m_blockingClosureMock)).RetiresOnSaturation();

    EXPECT_CALL(*m_blockingClosureMock, wait()).RetiresOnSaturation();
    EXPECT_CALL(*m_controllerMock, Failed()).WillOnce(Return(true)).RetiresOnSaturation();
    EXPECT_CALL(*m_controllerMock, ErrorText()).WillOnce(Return("Failed for some reason...")).RetiresOnSaturation();
}

void IpcModuleBase::expectIpcApiCallDisconnected()
{
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(false)).RetiresOnSaturation();

    EXPECT_CALL(m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(false)).RetiresOnSaturation();
}

void IpcModuleBase::expectIpcApiCallReconnected()
{
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(false)).RetiresOnSaturation();

    expectAttachChannel();

    EXPECT_CALL(m_ipcClientMock, createRpcController()).WillOnce(Return(m_controllerMock)).RetiresOnSaturation();
    EXPECT_CALL(m_ipcClientMock, createBlockingClosure()).WillOnce(Return(m_blockingClosureMock)).RetiresOnSaturation();

    EXPECT_CALL(*m_blockingClosureMock, wait()).RetiresOnSaturation();
    EXPECT_CALL(*m_controllerMock, Failed()).WillOnce(Return(false)).RetiresOnSaturation();
}
