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

#ifndef FIREBOLT_RIALTO_IPC_MOCK_IPC_CHANNEL_MOCK_H_
#define FIREBOLT_RIALTO_IPC_MOCK_IPC_CHANNEL_MOCK_H_

#include "IIpcChannel.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>

namespace firebolt::rialto::ipc::mock
{
class ChannelMock : public IChannel
{
public:
    ChannelMock() = default;
    virtual ~ChannelMock() = default;

    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (override, const));
    MOCK_METHOD(int, fd, (), (override, const));
    MOCK_METHOD(bool, wait, (int timeoutMSecs), (override));
    MOCK_METHOD(bool, process, (), (override));
    MOCK_METHOD(bool, unsubscribe, (int eventTag), (override));
    MOCK_METHOD(int, subscribeImpl,
                (const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                 std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler),
                (override));
    MOCK_METHOD(void, CallMethod,
                (const google::protobuf::MethodDescriptor *method, google::protobuf::RpcController *controller,
                 const google::protobuf::Message *request, google::protobuf::Message *response,
                 google::protobuf::Closure *done),
                (override));
};
} // namespace firebolt::rialto::ipc::mock

#endif // FIREBOLT_RIALTO_IPC_MOCK_IPC_CHANNEL_MOCK_H_
