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

#ifndef RIALTO_SERVERMANAGER_IPC_IPC_LOOP_H_
#define RIALTO_SERVERMANAGER_IPC_IPC_LOOP_H_

#include "IBlockingClosure.h"

#include <IIpcChannel.h>
#include <IIpcControllerFactory.h>

#include <memory>
#include <string>
#include <thread>

namespace rialto::servermanager::ipc
{
class Client;
} // namespace rialto::servermanager::ipc

namespace rialto::servermanager::ipc
{
class IpcLoop : public std::enable_shared_from_this<IpcLoop>
{
public:
    ~IpcLoop();

    static std::shared_ptr<IpcLoop> create(int socket, const Client &client);

    ::firebolt::rialto::ipc::IChannel *channel() const;

    std::shared_ptr<firebolt::rialto::ipc::IBlockingClosure> createBlockingClosure();
    std::shared_ptr<google::protobuf::RpcController> createRpcController();

private:
    explicit IpcLoop(std::shared_ptr<::firebolt::rialto::ipc::IChannel> channel, const Client &client);

    void ipcThread();

private:
    std::thread m_ipcThread;

    std::shared_ptr<::firebolt::rialto::ipc::IChannel> m_ipcChannel;
    std::shared_ptr<::firebolt::rialto::ipc::IControllerFactory> m_ipcControllerFactory;
    const Client &m_kClient;
    bool m_isClosing;
};
} // namespace rialto::servermanager::ipc

#endif // RIALTO_SERVERMANAGER_IPC_IPC_LOOP_H_
