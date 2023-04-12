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

#include "IpcClient.h"
#include "RialtoClientLogging.h"

namespace firebolt::rialto::client
{
IpcClient::IpcClient(const std::shared_ptr<ipc::IChannelFactory> &ipcChannelFactory,
                     const std::shared_ptr<ipc::IControllerFactory> &ipcControllerFactory,
                     const std::shared_ptr<ipc::IBlockingClosureFactory> &blockingClosureFactory)
    : m_ipcControllerFactory(ipcControllerFactory), m_ipcChannelFactory(ipcChannelFactory),
      m_blockingClosureFactory(blockingClosureFactory)
{
    // For now, always connect the client on construction
    if (!connect())
    {
        throw std::runtime_error("Cound not connect client");
    }
}

IpcClient::~IpcClient()
{
    if (!disconnect())
    {
        RIALTO_CLIENT_LOG_WARN("Could not disconnect client");
    }
}

IpcClient &IpcClient::instance()
{
    static IpcClient ipcClient{ipc::IChannelFactory::createFactory(), ipc::IControllerFactory::createFactory(),
                               ipc::IBlockingClosureFactory::createFactory()};
    return ipcClient;
}

bool IpcClient::connect()
{
    if (m_ipcChannel)
    {
        RIALTO_CLIENT_LOG_INFO("Client already connected");
        return true;
    }

    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // check if either of following env vars are set to determine the location of the rialto socket
    //  - RIALTO_SOCKET_PATH should specify the absolute path to the socket to connect to
    //  - RIALTO_SOCKET_FD should specify the number of a file descriptor of the socket to connect to
    const char *rialtoPath = getenv("RIALTO_SOCKET_PATH");
    const char *rialtoFd = getenv("RIALTO_SOCKET_FD");
    if (rialtoFd)
    {
        char *end = nullptr;
        int fd = strtol(rialtoFd, &end, 10);
        if ((errno != 0) || (rialtoFd == end) || (*end != '\0'))
        {
            RIALTO_CLIENT_LOG_SYS_ERROR(errno, "Invalid value set in RIALTO_SOCKET_FD env var");
            return false;
        }

        m_ipcChannel = m_ipcChannelFactory->createChannel(fd);
    }
    else if (rialtoPath)
    {
        m_ipcChannel = m_ipcChannelFactory->createChannel(rialtoPath);
    }
    else
    {
        RIALTO_CLIENT_LOG_ERROR("No rialto socket specified");
        return false;
    }

    // check if the channel was opened
    if (!m_ipcChannel)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to open a connection to the ipc socket");
        return false;
    }

    // spin up the thread that runs the IPC event loop
    m_ipcThread = std::thread(&IpcClient::ipcThread, this);
    if (!m_ipcThread.joinable())
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create thread for IPC");
        return false;
    }

    return true;
}

bool IpcClient::disconnect()
{
    if (!m_ipcChannel)
    {
        RIALTO_CLIENT_LOG_INFO("Client already disconnect");
        return true;
    }

    RIALTO_CLIENT_LOG_INFO("closing IPC channel");
    // disconnect from the server, this should terminate the thread so join that too
    if (m_ipcChannel)
        m_ipcChannel->disconnect();

    if (m_ipcThread.joinable())
        m_ipcThread.join();

    // destroy the IPC channel
    m_ipcChannel.reset();

    return true;
}

void IpcClient::ipcThread()
{
    pthread_setname_np(pthread_self(), "rialto-ipc");

    RIALTO_CLIENT_LOG_INFO("started ipc thread");

    while (m_ipcChannel->process())
    {
        m_ipcChannel->wait(-1);
    }

    RIALTO_CLIENT_LOG_INFO("exiting ipc thread");
}

std::shared_ptr<::firebolt::rialto::ipc::IChannel> IpcClient::getChannel() const
{
    return m_ipcChannel;
}

std::shared_ptr<ipc::IBlockingClosure> IpcClient::createBlockingClosure()
{
    if (!m_ipcChannel)
    {
        RIALTO_CLIENT_LOG_ERROR("ipc channel not connected");
        return nullptr;
    }

    // check which thread we're being called from, this determines if we pump
    // event loop from within the wait() method or not
    if (m_ipcThread.get_id() == std::this_thread::get_id())
        return m_blockingClosureFactory->createBlockingClosurePoll(m_ipcChannel);
    else
        return m_blockingClosureFactory->createBlockingClosureSemaphore();
}

std::shared_ptr<google::protobuf::RpcController> IpcClient::createRpcController()
{
    return m_ipcControllerFactory->create();
}

}; // namespace firebolt::rialto::client
