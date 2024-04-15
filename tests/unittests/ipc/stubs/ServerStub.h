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

#ifndef SERVER_STUB_H_
#define SERVER_STUB_H_

#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

#include "IIpcServer.h"
#include "testmodule.pb.h"

class ServerStub
{
public:
    ServerStub();
    explicit ServerStub(std::shared_ptr<::firebolt::rialto::TestModule> moduleMock);
    ~ServerStub();

    void clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &client);
    void clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &client);

    void sendSingleVarEvent(int32_t var1);
    void sendMultiVarEvent(int32_t var1, uint32_t var2, firebolt::rialto::TestEventMultiVar_TestType var3,
                           std::string var4);

private:
    std::shared_ptr<::firebolt::rialto::ipc::IServer> m_server;
    std::shared_ptr<::firebolt::rialto::ipc::IClient> m_client;
    std::thread m_serverThread;
    std::atomic<bool> m_running;
    std::shared_ptr<::firebolt::rialto::TestModule> m_testMock;
    std::atomic<bool> m_clientConnected;
    std::mutex m_clientConnectMutex;
    std::condition_variable m_clientConnectCond;

    void init();
};

#endif // SERVER_STUB_H_
