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

#ifndef IPC_CLIENT_TEST_BASE_H_
#define IPC_CLIENT_TEST_BASE_H_

#include "BlockingClosureFactoryMock.h"
#include "BlockingClosureMock.h"
#include "IIpcClient.h"
#include "IpcChannelFactoryMock.h"
#include "IpcChannelMock.h"
#include "IpcControllerFactoryMock.h"
#include "RpcControllerMock.h"
#include <condition_variable>
#include <gtest/gtest.h>
#include <memory>
#include <mutex>

using namespace firebolt::rialto;
using namespace firebolt::rialto::ipc;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Sequence;
using ::testing::StrictMock;

class IpcClientTestBase : public ::testing::Test
{
protected:
    // Strict Mocks
    std::shared_ptr<StrictMock<ChannelFactoryMock>> m_channelFactoryMock;
    std::shared_ptr<StrictMock<ChannelMock>> m_channelMock;
    std::shared_ptr<StrictMock<ControllerFactoryMock>> m_controllerFactoryMock;
    std::shared_ptr<StrictMock<RpcControllerMock>> m_controllerMock;
    std::shared_ptr<StrictMock<BlockingClosureFactoryMock>> m_blockingClosureFactoryMock;
    std::shared_ptr<StrictMock<BlockingClosureMock>> m_blockingClosureMock;

    std::unique_ptr<IIpcClient> m_sut;

    // Common variables
    const char *m_kRialtoPath = getenv("RIALTO_SOCKET_PATH");

    // Variables for the ipc event loop
    std::mutex m_eventsLock;
    std::condition_variable m_eventsCond;
    bool m_disconnected = true;

    IpcClientTestBase();
    virtual ~IpcClientTestBase();
    void createIpcClient();
    void failToCreateIpcClient();
    void createBlockingClosure();
    void createRpcController();
    void disconnectIpcClient();
    void expectCreateChannel();
    void expectIpcLoop();
};

#endif // IPC_CLIENT_TEST_BASE_H_
