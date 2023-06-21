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

#include "RialtoSessionServerStub.h"
#include "servermanagermodule.pb.h"
#include <IIpcServer.h>
#include <IIpcServerFactory.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
class Service : public ::rialto::ServerManagerModule
{
    StubResponse m_programmedResponse;

public:
    explicit Service(StubResponse resp) : m_programmedResponse{resp} {}

    virtual ~Service() = default;

    void setState(::google::protobuf::RpcController *controller, const ::rialto::SetStateRequest *request,
                  ::rialto::SetStateResponse *response, ::google::protobuf::Closure *done) override
    {
        if (m_programmedResponse != StubResponse::OK)
        {
            controller->SetFailed("Failed for some reason ...");
        }
        done->Run();
    }

    void setLogLevels(::google::protobuf::RpcController *controller, const ::rialto::SetLogLevelsRequest *request,
                      ::rialto::SetLogLevelsResponse *response, ::google::protobuf::Closure *done) override
    {
        if (m_programmedResponse != StubResponse::OK)
        {
            controller->SetFailed("Failed for some reason ...");
        }
        done->Run();
    }

    void setConfiguration(::google::protobuf::RpcController *controller, const ::rialto::SetConfigurationRequest *request,
                          ::rialto::SetConfigurationResponse *response, ::google::protobuf::Closure *done) override
    {
        if (m_programmedResponse != StubResponse::OK)
        {
            controller->SetFailed("Failed for some reason ...");
        }
        done->Run();
    }

    void ping(::google::protobuf::RpcController *controller, const ::rialto::PingRequest *request,
              ::rialto::PingResponse *response, ::google::protobuf::Closure *done) override
    {
        if (m_programmedResponse != StubResponse::OK)
        {
            controller->SetFailed("Failed for some reason ...");
        }
        done->Run();
    }
};

void clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &client)
{
    printf("Client disconnected, pid:%d, uid:%d gid:%d\n", client->getClientPid(), client->getClientUserId(),
           client->getClientGroupId());
}
} // namespace

RialtoSessionServerStub::RialtoSessionServerStub() : m_socks{-1, -1}
{
    auto factory = ::firebolt::rialto::ipc::IServerFactory::createFactory();
    m_server = factory->create();

    EXPECT_EQ(socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, m_socks.data()), 0);
}

RialtoSessionServerStub::~RialtoSessionServerStub()
{
    if (m_client && m_client->isConnected())
    {
        m_client->disconnect();
    }
    if (m_serverThread.joinable())
    {
        m_serverThread.join();
    }
    m_client.reset();
}

void RialtoSessionServerStub::start(StubResponse stubResponse)
{
    // give one socket to the server
    m_client = m_server->addClient(m_socks[0], &clientDisconnected);
    EXPECT_TRUE(m_client);
    // export the example service to the m_client
    m_client->exportService(std::make_shared<Service>(stubResponse));
    m_serverThread = std::thread(
        [this]()
        {
            while (m_server->process() && m_client->isConnected())
            {
                m_server->wait(-1);
            }
        });
}

int RialtoSessionServerStub::getClientSocket() const
{
    return m_socks[1];
}

void RialtoSessionServerStub::sendStateChangedEvent()
{
    EXPECT_TRUE(m_client);
    EXPECT_TRUE(m_client->isConnected());
    auto stateChangedEvent = std::make_shared<rialto::StateChangedEvent>();
    stateChangedEvent->set_sessionserverstate(::rialto::SessionServerState::INACTIVE);
    m_client->sendEvent(stateChangedEvent);
}

void RialtoSessionServerStub::sendAckEvent(int pingId, bool success)
{
    EXPECT_TRUE(m_client);
    EXPECT_TRUE(m_client->isConnected());
    auto ackEvent = std::make_shared<rialto::AckEvent>();
    ackEvent->set_id(pingId);
    ackEvent->set_success(success);
    m_client->sendEvent(ackEvent);
}

void RialtoSessionServerStub::disconnectClient()
{
    EXPECT_TRUE(m_client);
    EXPECT_TRUE(m_client->isConnected());
    m_client->disconnect();
}
