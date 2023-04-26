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

#include "ControlIpc.h"
#include "RialtoClientLogging.h"

namespace
{
firebolt::rialto::ApplicationState
convertApplicationState(const firebolt::rialto::ApplicationStateChangeEvent_ApplicationState &state)
{
    switch (state)
    {
    case firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_RUNNING:
        return firebolt::rialto::ApplicationState::RUNNING;
    case firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_INACTIVE:
        return firebolt::rialto::ApplicationState::INACTIVE;
    case firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_UNKNOWN:
        break;
    }
    return firebolt::rialto::ApplicationState::UNKNOWN;
}
} // namespace

namespace firebolt::rialto::client
{
ControlIpcAccessor &getControlIpcAccessorGeneric()
{
    static ControlIpcAccessor accessor;
    return accessor;
}

ControlIpc &getControlIpcGeneric()
{
    static ControlIpc controlIpc{ipc::IChannelFactory::createFactory(), ipc::IControllerFactory::createFactory(),
                                 ipc::IBlockingClosureFactory::createFactory(),
                                 firebolt::rialto::common::IEventThreadFactory::createFactory()};
    return controlIpc;
}

IControlIpcAccessor &IControlIpcAccessor::instance()
{
    return getControlIpcAccessorGeneric();
}

IIpcClientAccessor &IIpcClientAccessor::instance()
{
    return getControlIpcAccessorGeneric();
}

IControlIpc &ControlIpcAccessor::getControlIpc() const
{
    return getControlIpcGeneric();
}

IIpcClient &ControlIpcAccessor::getIpcClient() const
{
    return getControlIpcGeneric();
}

ControlIpc::ControlIpc(const std::shared_ptr<ipc::IChannelFactory> &ipcChannelFactory,
                       const std::shared_ptr<ipc::IControllerFactory> &ipcControllerFactory,
                       const std::shared_ptr<ipc::IBlockingClosureFactory> &blockingClosureFactory,
                       const std::shared_ptr<common::IEventThreadFactory> &eventThreadFactory)
    : IpcModule(static_cast<IIpcClient &>(*this)), m_ipcControllerFactory(ipcControllerFactory),
      m_ipcChannelFactory(ipcChannelFactory), m_blockingClosureFactory(blockingClosureFactory),
      m_eventThread(eventThreadFactory->createEventThread("rialto-control-events"))
{
    // For now, always connect the client on construction
    if (!connect())
    {
        throw std::runtime_error("Cound not connect client");
    }

    if (!attachChannel())
    {
        throw std::runtime_error("Failed attach to the ipc channel");
    }
}

ControlIpc::~ControlIpc()
{
    // detach the Ipc channel
    detachChannel();

    // destroy the thread processing async notifications
    m_eventThread.reset();

    if (!disconnect())
    {
        RIALTO_CLIENT_LOG_WARN("Could not disconnect client");
    }
}

bool ControlIpc::getSharedMemory(int32_t &fd, uint32_t &size)
{
    // Increase reference incase client disconnects from another thread
    std::shared_ptr<ipc::IChannel> ipcChannel = m_ipcChannel;
    std::shared_ptr<::firebolt::rialto::ControlModule_Stub> controlStub = m_controlStub;

    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::GetSharedMemoryRequest request;

    firebolt::rialto::GetSharedMemoryResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    controlStub->getSharedMemory(ipcController.get(), &request, &response, blockingClosure.get());

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

bool ControlIpc::registerClient(IControlClient *client)
{
    m_controlClient = client;
    return true;
}

bool ControlIpc::unregisterClient(IControlClient *client)
{
    if (m_controlClient == client)
    {
        m_controlClient = nullptr;
        return true;
    }
    return false;
}

bool ControlIpc::connect()
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
    m_ipcThread = std::thread(&ControlIpc::ipcThread, this);
    if (!m_ipcThread.joinable())
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create thread for IPC");
        return false;
    }

    return true;
}

bool ControlIpc::disconnect()
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

void ControlIpc::ipcThread()
{
    pthread_setname_np(pthread_self(), "rialto-ipc");

    RIALTO_CLIENT_LOG_INFO("started ipc thread");

    while (m_ipcChannel->process())
    {
        m_ipcChannel->wait(-1);
    }

    RIALTO_CLIENT_LOG_INFO("exiting ipc thread");
}

std::shared_ptr<::firebolt::rialto::ipc::IChannel> ControlIpc::getChannel() const
{
    return m_ipcChannel;
}

std::shared_ptr<ipc::IBlockingClosure> ControlIpc::createBlockingClosure()
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

std::shared_ptr<google::protobuf::RpcController> ControlIpc::createRpcController()
{
    return m_ipcControllerFactory->create();
}

bool ControlIpc::createRpcStubs()
{
    m_controlStub = std::make_unique<::firebolt::rialto::ControlModule_Stub>(m_ipcChannel.get());
    if (!m_controlStub)
    {
        return false;
    }
    return true;
}

bool ControlIpc::subscribeToEvents()
{
    if (!m_ipcChannel)
    {
        return false;
    }

    int eventTag = m_ipcChannel->subscribe<firebolt::rialto::ApplicationStateChangeEvent>(
        [this](const std::shared_ptr<firebolt::rialto::ApplicationStateChangeEvent> &event)
        { m_eventThread->add(&ControlIpc::onApplicationStateUpdated, this, event); });
    if (eventTag < 0)
        return false;
    m_eventTags.push_back(eventTag);

    eventTag = m_ipcChannel->subscribe<firebolt::rialto::PingEvent>(
        [this](const std::shared_ptr<firebolt::rialto::PingEvent> &event)
        { m_eventThread->add(&ControlIpc::onPing, this, event); });
    if (eventTag < 0)
        return false;
    m_eventTags.push_back(eventTag);

    return true;
}

void ControlIpc::onApplicationStateUpdated(const std::shared_ptr<firebolt::rialto::ApplicationStateChangeEvent> &event)
{
    m_controlClient->notifyApplicationState(convertApplicationState(event->application_state()));
}

void ControlIpc::onPing(const std::shared_ptr<firebolt::rialto::PingEvent> &event)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return;
    }
    firebolt::rialto::AckRequest request;
    request.set_id(event->id());
    firebolt::rialto::AckResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_controlStub->ack(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to ack due to '%s'", ipcController->ErrorText().c_str());
    }
}
}; // namespace firebolt::rialto::client
