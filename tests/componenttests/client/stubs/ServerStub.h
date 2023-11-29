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

#ifndef FIREBOLT_RIALTO_COMPONENTTESTS_SERVER_STUB_H_
#define FIREBOLT_RIALTO_COMPONENTTESTS_SERVER_STUB_H_

#include "ControlModuleStub.h"
#include "MediaPipelineModuleStub.h"
#include "IIpcServer.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>

namespace firebolt::rialto::ct::stub
{
class ServerStub : public ControlModuleStub, MediaPipelineModuleStub
{
public:
    explicit ServerStub(const std::shared_ptr<::firebolt::rialto::ControlModule>& controlModuleMock,
                        const std::shared_ptr<::firebolt::rialto::MediaPipelineModule>& mediaPipelineModuleMock);
    ~ServerStub();

    void clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &client);
    void clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &client);

private:
    std::shared_ptr<::firebolt::rialto::ipc::IServer> m_server;
    std::shared_ptr<::firebolt::rialto::ipc::IClient> m_client;
    std::thread m_serverThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_clientConnected;
    std::mutex m_clientConnectMutex;
    std::condition_variable m_clientConnectCond;

    void init();
    void waitForClientConnect() override;
    std::shared_ptr<::firebolt::rialto::ipc::IClient>& getClient() override;
};
} // namespace firebolt::rialto::ct::stub

#endif // FIREBOLT_RIALTO_COMPONENTTESTS_SERVER_STUB_H_
