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

#ifndef FIREBOLT_RIALTO_SERVER_CT_SERVER_MANAGER_STUB_H_
#define FIREBOLT_RIALTO_SERVER_CT_SERVER_MANAGER_STUB_H_

#include "IStub.h"
#include <IIpcChannel.h>
#include <array>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace firebolt::rialto::server::ct
{
class ServerManagerStub : public IStub
{
public:
    ServerManagerStub();
    ~ServerManagerStub() override;

    int getServerSocket() const;
    std::shared_ptr<::firebolt::rialto::ipc::IChannel> getChannel() override;

private:
    void ipcThread();

private:
    std::thread m_ipcThread;
    std::shared_ptr<::firebolt::rialto::ipc::IChannel> m_ipcChannel;
    std::array<int, 2> m_socks{-1, -1};

    // Its possible that getChannel can be called before the ipcThread creates the channel
    std::mutex m_channelLock;
    std::condition_variable m_channelCond;
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_SERVER_MANAGER_STUB_H_
