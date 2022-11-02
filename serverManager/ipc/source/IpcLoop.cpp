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

#include "IpcLoop.h"
#include "Client.h"
#include "RialtoServerManagerLogging.h"
#include <utility>

namespace rialto::servermanager::ipc
{
std::shared_ptr<IpcLoop> IpcLoop::create(int socket, const Client &client)
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    auto factory = firebolt::rialto::ipc::IChannelFactory::createFactory();
    auto channel = factory->createChannel(socket);
    if (!channel)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Connection failed");
        return nullptr;
    }

    // Wrap the channel in an object that runs the thread loop
    return std::shared_ptr<IpcLoop>(new IpcLoop(std::move(channel), client));
}

IpcLoop::IpcLoop(std::shared_ptr<::firebolt::rialto::ipc::IChannel> channel, const Client &client)
    : m_ipcChannel(std::move(channel)),
      m_ipcControllerFactory(firebolt::rialto::ipc::IControllerFactory::createFactory()),
      m_kClient(client), m_isClosing{false}
{
    // spin up the thread that runs the IPC event loop
    m_ipcThread = std::thread(&IpcLoop::ipcThread, this);
    if (!m_ipcThread.joinable())
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to create thread for IPC");
        return;
    }
}

IpcLoop::~IpcLoop()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("closing IPC channel");
    m_isClosing = true;

    // disconnect from the server, this should terminate the thread so join that too
    if (m_ipcChannel)
        m_ipcChannel->disconnect();

    RIALTO_SERVER_MANAGER_LOG_INFO("terminating IPC thread");

    if (m_ipcThread.joinable())
        m_ipcThread.join();

    // destroy the IPC channel
    m_ipcChannel.reset();
}

/**
 * @brief Runs the poll loop for reading events and replies from the
 * IPC channel / socket.
 *
 *
 */
void IpcLoop::ipcThread()
{
    pthread_setname_np(pthread_self(), "rialtoservermanager-ipc");

    RIALTO_SERVER_MANAGER_LOG_INFO("started ipc thread");

    while (m_ipcChannel->process())
    {
        m_ipcChannel->wait(-1);
    }

    RIALTO_SERVER_MANAGER_LOG_INFO("exiting ipc thread");
    if (!m_isClosing)
    {
        m_kClient.onDisconnected();
    }
}

::firebolt::rialto::ipc::IChannel *IpcLoop::channel() const
{
    return m_ipcChannel.get();
}

/**
 * @brief Returns either a polling or semaphore based blocking closure
 * depending on the thread the function is called from.
 *
 * @param[in] channel  : the channel that the closure is used for
 *
 */
std::shared_ptr<firebolt::rialto::ipc::IBlockingClosure> IpcLoop::createBlockingClosure()
{
    if (!m_ipcChannel)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("ipc channel not connected");
        return nullptr;
    }

    // TODO(LLDEV-27303): replace with a pool of closures rather than allocating new one each time

    // check which thread we're being called from, this determines if we pump
    // event loop from within the wait() method or not
    auto factory = firebolt::rialto::ipc::IBlockingClosureFactory::createFactory();
    if (m_ipcThread.get_id() == std::this_thread::get_id())
        return factory->createBlockingClosurePoll(m_ipcChannel);
    else
        return factory->createBlockingClosureSemaphore();
}

/**
 * @brief Returns a RpcController object that can be used for sending
 * RPC requests.
 *
 * This is just a wrapper around the ::firebolt::rialto::ipc::ControllerFactory
 * object.
 *
 * @param[in] channel  : the channel that the closure is used for
 *
 */
std::shared_ptr<google::protobuf::RpcController> IpcLoop::createRpcController()
{
    // TODO(LLDEV-27303): replace with a pool of controllers rather than allocating new one each time

    return m_ipcControllerFactory->create();
}

} // namespace rialto::servermanager::ipc
