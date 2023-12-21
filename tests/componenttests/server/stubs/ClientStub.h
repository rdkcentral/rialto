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

#ifndef FIREBOLT_RIALTO_SERVER_CT_CLIENT_STUB_H_
#define FIREBOLT_RIALTO_SERVER_CT_CLIENT_STUB_H_

#include <memory>
#include <thread>

#include "IStub.h"

namespace firebolt::rialto::server::ct
{
class ClientStub : public IStub
{
public:
    ClientStub() = default;
    ~ClientStub() override;

    std::shared_ptr<::firebolt::rialto::ipc::IChannel> getChannel() override;
    bool connect();

    std::shared_ptr<::firebolt::rialto::ipc::IChannel> getIpcChannel();

private:
    void ipcThread();

private:
    std::shared_ptr<::firebolt::rialto::ipc::IChannel> m_ipcChannel;
    std::thread m_ipcThread;
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_CLIENT_STUB_H_
