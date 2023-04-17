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
std::shared_ptr<IControlIpcFactory> IControlIpcFactory::createFactory()
{
    return ControlIpcFactory::createFactory();
}

std::shared_ptr<ControlIpcFactory> ControlIpcFactory::createFactory()
{
    std::shared_ptr<ControlIpcFactory> factory;

    try
    {
        factory = std::make_shared<ControlIpcFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the rialto control ipc factory, reason: %s", e.what());
    }

    return factory;
}

std::shared_ptr<IControlIpc> ControlIpcFactory::getControlIpc(IControlClient *controlClient)
{
    return std::make_shared<ControlIpc>(controlClient, IIpcClientAccessor::instance().getIpcClient(),
                                        firebolt::rialto::common::IEventThreadFactory::createFactory());
}

ControlIpc::ControlIpc(IControlClient *controlClient, IIpcClient &ipcClient,
                       const std::shared_ptr<common::IEventThreadFactory> &eventThreadFactory)
    : IpcModule(ipcClient), m_controlClient(controlClient),
      m_eventThread(eventThreadFactory->createEventThread("rialto-control-events"))
{
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
