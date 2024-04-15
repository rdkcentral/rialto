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

#ifndef FIREBOLT_RIALTO_CLIENT_IPC_CLIENT_MOCK_H_
#define FIREBOLT_RIALTO_CLIENT_IPC_CLIENT_MOCK_H_

#include "IIpcClient.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::client
{
class IpcClientMock : public IIpcClient
{
public:
    IpcClientMock() = default;
    virtual ~IpcClientMock() = default;

    MOCK_METHOD(std::weak_ptr<ipc::IChannel>, getChannel, (), (override, const));
    MOCK_METHOD(std::shared_ptr<ipc::IBlockingClosure>, createBlockingClosure, (), (override));
    MOCK_METHOD(std::shared_ptr<google::protobuf::RpcController>, createRpcController, (), (override));
    MOCK_METHOD(bool, reconnect, (), (override));
    MOCK_METHOD(void, registerConnectionObserver, (const std::weak_ptr<IConnectionObserver> &observer), (override));
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_IPC_CLIENT_MOCK_H_
