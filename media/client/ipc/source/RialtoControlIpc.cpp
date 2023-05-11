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

#include "RialtoControlIpc.h"
#include "RialtoClientLogging.h"

namespace firebolt::rialto::client
{
std::weak_ptr<RialtoControlIpc> RialtoControlIpcFactory::m_rialtoControlIpc;
std::mutex RialtoControlIpcFactory::m_creationMutex;

std::shared_ptr<IRialtoControlIpcFactory> IRialtoControlIpcFactory::createFactory()
{
    return RialtoControlIpcFactory::createFactory();
}

std::shared_ptr<IIpcClientFactory> IIpcClientFactory::createFactory()
{
    return RialtoControlIpcFactory::createFactory();
}

std::shared_ptr<RialtoControlIpcFactory> RialtoControlIpcFactory::createFactory()
{
    std::shared_ptr<RialtoControlIpcFactory> factory;

    try
    {
        factory = std::make_shared<RialtoControlIpcFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the rialto control ipc factory, reason: %s", e.what());
    }

    return factory;
}

std::shared_ptr<IRialtoControlIpc> RialtoControlIpcFactory::getRialtoControlIpc()
{
    return getGeneric();
}

std::shared_ptr<IIpcClient> RialtoControlIpcFactory::getIpcClient()
{
    return getGeneric();
}

std::shared_ptr<RialtoControlIpc> RialtoControlIpcFactory::getGeneric()
{
    std::lock_guard<std::mutex> lock{m_creationMutex};

    std::shared_ptr<RialtoControlIpc> rialtoControlIpc = m_rialtoControlIpc.lock();

    if (!rialtoControlIpc)
    {
        try
        {
            rialtoControlIpc = std::make_unique<RialtoControlIpc>(ipc::IChannelFactory::createFactory(),
                                                                  ipc::IControllerFactory::createFactory(),
                                                                  ipc::IBlockingClosureFactory::createFactory());
        }
        catch (const std::exception &e)
        {
            RIALTO_CLIENT_LOG_ERROR("Failed to create the rialto control ipc, reason: %s", e.what());
        }

        m_rialtoControlIpc = rialtoControlIpc;
    }

    return rialtoControlIpc;
}

RialtoControlIpc::RialtoControlIpc(const std::shared_ptr<ipc::IChannelFactory> &ipcChannelFactory,
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

RialtoControlIpc::~RialtoControlIpc()
{
    if (!disconnect())
    {
        RIALTO_CLIENT_LOG_WARN("Could not disconnect client");
    }
}

bool RialtoControlIpc::connect()
{
    if (m_ipcChannel)
    {
        RIALTO_CLIENT_LOG_WARN("Client already connected");
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

    // Increase reference incase client disconnects from another thread
    std::shared_ptr<ipc::IChannel> ipcChannel = m_ipcChannel;

    // spin up the thread that runs the IPC event loop
    m_ipcThread = std::thread(&RialtoControlIpc::ipcThread, this);
    if (!m_ipcThread.joinable())
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create thread for IPC");
        return false;
    }

    // create the RPC stubs
    m_rialtoControlStub = std::make_shared<::firebolt::rialto::RialtoControlModule_Stub>(ipcChannel.get());
    if (!m_rialtoControlStub)
    {
        RIALTO_CLIENT_LOG_ERROR("Could not create the rialto control ipc stubs");
        m_ipcChannel.reset();
        return false;
    }

    return true;
}

bool RialtoControlIpc::disconnect()
{
    // Increase reference incase client disconnects from another thread
    std::shared_ptr<ipc::IChannel> ipcChannel = m_ipcChannel;
    if (!ipcChannel)
    {
        // The ipc channel may have disconnected unexpectadly, join the ipc thread if required
        if (m_ipcThread.joinable())
            m_ipcThread.join();

        RIALTO_CLIENT_LOG_INFO("Client already disconnect");
        return true;
    }

    m_disconnecting = true;

   // release the RPC stubs
    m_rialtoControlStub.reset();

    // disconnect from the server, this should terminate the thread so join that too
    if (ipcChannel)
        ipcChannel->disconnect();

    if (m_ipcThread.joinable())
        m_ipcThread.join();

    // destroy the IPC channel
    m_ipcChannel.reset();

    m_disconnecting = false;

    return true;
}

bool RialtoControlIpc::getSharedMemory(int32_t &fd, uint32_t &size)
{
    // Increase reference incase client disconnects from another thread
    std::shared_ptr<ipc::IChannel> ipcChannel = m_ipcChannel;
    std::shared_ptr<::firebolt::rialto::RialtoControlModule_Stub> rialtoControlStub = m_rialtoControlStub;

    if ((nullptr == ipcChannel) || (nullptr == rialtoControlStub))
    {
        RIALTO_CLIENT_LOG_ERROR("Client disconnected");
        return false;
    }

    firebolt::rialto::GetSharedMemoryRequest request;
    firebolt::rialto::GetSharedMemoryResponse response;
    auto ipcController = createRpcController();
    auto blockingClosure = createBlockingClosure();
    rialtoControlStub->getSharedMemory(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to get the shared memory due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    fd = response.fd();
    size = response.size();

    return true;
}

void RialtoControlIpc::ipcThread()
{
    pthread_setname_np(pthread_self(), "rialto-ipc");

    RIALTO_CLIENT_LOG_ERROR("started ipc thread");

    while (m_ipcChannel->process())
    {
        RIALTO_CLIENT_LOG_ERROR("Client disconnected");
        m_ipcChannel->wait(-1);
    }

    if (!m_disconnecting)
    {
        RIALTO_CLIENT_LOG_ERROR("The ipc channel unexpectedly disconnected, destroying the channel");

        // Safe to destroy the ipc objects in the ipc thread as the client has already disconnected.
        // This ensures the channel is destructed and that all ongoing ipc calls are unblocked.
        m_rialtoControlStub.reset();
        m_ipcChannel.reset();
    }

    RIALTO_CLIENT_LOG_ERROR("exiting ipc thread");
}

std::weak_ptr<::firebolt::rialto::ipc::IChannel> RialtoControlIpc::getChannel() const
{
    RIALTO_CLIENT_LOG_ERROR("here1");
    return m_ipcChannel;
}

std::shared_ptr<ipc::IBlockingClosure> RialtoControlIpc::createBlockingClosure()
{
    // Increase reference incase client disconnects from another thread
    std::shared_ptr<ipc::IChannel> ipcChannel = m_ipcChannel;
    if (!ipcChannel)
    {
        RIALTO_CLIENT_LOG_ERROR("ipc channel not connected");
        return nullptr;
    }

    // check which thread we're being called from, this determines if we pump
    // event loop from within the wait() method or not
    if (m_ipcThread.get_id() == std::this_thread::get_id())
        return m_blockingClosureFactory->createBlockingClosurePoll(ipcChannel);
    else
        return m_blockingClosureFactory->createBlockingClosureSemaphore();
}

std::shared_ptr<google::protobuf::RpcController> RialtoControlIpc::createRpcController()
{
    return m_ipcControllerFactory->create();
}

}; // namespace firebolt::rialto::client
