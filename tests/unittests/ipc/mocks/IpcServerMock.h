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

#ifndef FIREBOLT_RIALTO_IPC_IPC_SERVER_MOCK_H_
#define FIREBOLT_RIALTO_IPC_IPC_SERVER_MOCK_H_

#include "IIpcServer.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>

namespace firebolt::rialto::ipc
{
class ServerMock : public IServer
{
public:
    ServerMock() = default;
    virtual ~ServerMock() = default;

    MOCK_METHOD(bool, addSocket,
                (const std::string &socketPath, std::function<void(const std::shared_ptr<IClient> &)> clientConnectedCb,
                 std::function<void(const std::shared_ptr<IClient> &)> clientDisconnectedCb),
                (override));
    MOCK_METHOD(bool, addSocket,
                (int fd, std::function<void(const std::shared_ptr<IClient> &)> clientConnectedCb,
                 std::function<void(const std::shared_ptr<IClient> &)> clientDisconnectedCb),
                (override));
    MOCK_METHOD(std::shared_ptr<IClient>, addClient,
                (int socketFd, std::function<void(const std::shared_ptr<IClient> &)> clientDisconnectedCb), (override));
    MOCK_METHOD(int, fd, (), (override, const));
    MOCK_METHOD(bool, wait, (int timeoutMSecs), (override));
    MOCK_METHOD(bool, process, (), (override));
};
} // namespace firebolt::rialto::ipc

#endif // FIREBOLT_RIALTO_IPC_IPC_SERVER_MOCK_H_
