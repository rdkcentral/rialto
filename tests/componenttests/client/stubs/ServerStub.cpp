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

#include "ServerStub.h"
#include <IIpcServer.h>
#include <IIpcServerFactory.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
firebolt::rialto::ApplicationStateChangeEvent_ApplicationState
convertApplicationState(const firebolt::rialto::ApplicationState &state)
{
    switch (state)
    {
    case firebolt::rialto::ApplicationState::RUNNING:
        return firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_RUNNING;
    case firebolt::rialto::ApplicationState::INACTIVE:
        return firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_INACTIVE;
    case firebolt::rialto::ApplicationState::UNKNOWN:
        break;
        // do nothing
    }
    return firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_UNKNOWN;
}
} // namespace

namespace firebolt::rialto::componenttests
{
void ServerStub::clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &client)
{
    printf("Client disconnected, pid:%d, uid:%d gid:%d\n", client->getClientPid(), client->getClientUserId(),
           client->getClientGroupId());

    // remove client
    m_clientConnected.store(false);
    m_client.reset();

    // Notify listening thread
    m_clientConnectCond.notify_one();
}

void ServerStub::clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &client)
{
    printf("Client connected, pid:%d, uid:%d gid:%d\n", client->getClientPid(), client->getClientUserId(),
           client->getClientGroupId());

    // store client
    m_client = client;
    m_clientConnected.store(true);

    // export the RPC services to the m_client if it has been set in the stub
    if (m_controlModuleMock)
        client->exportService(m_controlModuleMock);

    // Notify listening thread
    m_clientConnectCond.notify_one();
}

ServerStub::ServerStub()
{
    init();
}

ServerStub::ServerStub(std::shared_ptr<::firebolt::rialto::ControlModule> controlModuleMock)
    : m_controlModuleMock{controlModuleMock}
{
    init();
}

void ServerStub::init()
{
    m_clientConnected = false;
    m_running = true;
    auto factory = ::firebolt::rialto::ipc::IServerFactory::createFactory();
    m_server = factory->create();

    const char *rialtoPath = getenv("RIALTO_SOCKET_PATH");
    if (!rialtoPath)
    {
        printf("RIALTO_SOCKET_PATH not defined\n");
    }
    m_server->addSocket(rialtoPath, std::bind(&ServerStub::clientConnected, this, std::placeholders::_1),
                        std::bind(&ServerStub::clientDisconnected, this, std::placeholders::_1));

    m_serverThread = std::thread(
        [this]()
        {
            while (m_server->process() && m_running)
            {
                m_server->wait(1);
            }
        });
}

ServerStub::~ServerStub()
{
    if (m_serverThread.joinable())
    {
        m_running = false;
        m_serverThread.join();
    }
}

void ServerStub::notifyApplicationStateEvent(const int32_t controlId, const firebolt::rialto::ApplicationState &state)
{
    auto event = std::make_shared<firebolt::rialto::ApplicationStateChangeEvent>();
    event->set_control_handle(controlId);
    event->set_application_state(convertApplicationState(state));

    if (!m_clientConnected.load())
    {
        std::unique_lock<std::mutex> clientConnectedLock(m_clientConnectMutex);
        std::cv_status status = m_clientConnectCond.wait_for(clientConnectedLock, std::chrono::milliseconds(100));
        ASSERT_NE(std::cv_status::timeout, status);
    }

    m_client->sendEvent(event);
}

} // namespace firebolt::rialto::componenttests
