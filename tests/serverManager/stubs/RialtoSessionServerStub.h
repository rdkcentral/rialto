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

#ifndef RIALTO_SESSION_SERVER_STUB_H_
#define RIALTO_SESSION_SERVER_STUB_H_

#include "IIpcServer.h"
#include <array>
#include <memory>
#include <thread>

enum class StubResponse
{
    OK,
    Fail
};

class RialtoSessionServerStub
{
public:
    RialtoSessionServerStub();
    ~RialtoSessionServerStub();

    void start(StubResponse stubResponse);
    int getClientSocket() const;
    void sendStateChangedEvent();
    void disconnectClient();

private:
    std::shared_ptr<::firebolt::rialto::ipc::IServer> m_server;
    std::shared_ptr<::firebolt::rialto::ipc::IClient> m_client;
    std::array<int, 2> m_socks;
    std::thread m_serverThread;
};

#endif // RIALTO_SESSION_SERVER_STUB_H_
