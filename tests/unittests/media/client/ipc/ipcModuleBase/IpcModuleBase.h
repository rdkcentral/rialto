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

#ifndef IPC_MODULE_BASE_H_
#define IPC_MODULE_BASE_H_

#include "BlockingClosureMock.h"
#include "IpcChannelMock.h"
#include "IpcClientMock.h"
#include "MediaPipelineIpc.h"
#include "MediaPipelineIpcClientMock.h"
#include "RpcControllerMock.h"
#include <gtest/gtest.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::ipc;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::Return;
using ::testing::Sequence;
using ::testing::StrictMock;

MATCHER_P(methodMatcher, name, "")
{
    return (name == arg->name());
}

class IpcModuleBase
{
public:
    IpcModuleBase();
    virtual ~IpcModuleBase() = default;

protected:
    std::shared_ptr<StrictMock<IpcClientMock>> m_ipcClientMock;
    std::shared_ptr<StrictMock<ChannelMock>> m_channelMock;
    std::shared_ptr<StrictMock<BlockingClosureMock>> m_blockingClosureMock;
    std::shared_ptr<StrictMock<RpcControllerMock>> m_controllerMock;

    Sequence m_isConnectedSeq;

    void expectInitIpc();
    void expectInitIpcWithReconnection();
    void expectInitIpcFailure();
    void expectInitIpcButAttachChannelFailure();
    void expectInitIpcButNotConnectedChannelAfterReconnect();
    void expectAttachChannel();
    void expectIpcApiCallDisconnected();
    void expectIpcApiCallReconnected();
    void expectIpcApiCallSuccess();
    void expectIpcApiCallFailure();
};

#endif // IPC_MODULE_BASE_H_
