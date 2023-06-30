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

#include "Client.h"
#include "IIpcChannel.h"
#include "ISessionServerAppManager.h"
#include "IpcLoop.h"
#include "RialtoServerManagerLogging.h"
#include "Utils.h"
#include "servermanagermodule.pb.h"
#include <cstring>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
firebolt::rialto::common::SessionServerState convert(const rialto::SessionServerState &sessionServerState)
{
    switch (sessionServerState)
    {
    case rialto::SessionServerState::UNINITIALIZED:
    {
        return firebolt::rialto::common::SessionServerState::UNINITIALIZED;
    }
    case rialto::SessionServerState::INACTIVE:
    {
        return firebolt::rialto::common::SessionServerState::INACTIVE;
    }
    case rialto::SessionServerState::ACTIVE:
    {
        return firebolt::rialto::common::SessionServerState::ACTIVE;
    }
    case rialto::SessionServerState::NOT_RUNNING:
    {
        return firebolt::rialto::common::SessionServerState::NOT_RUNNING;
    }
    case rialto::SessionServerState::ERROR:
    {
        return firebolt::rialto::common::SessionServerState::ERROR;
    }
    }
    return firebolt::rialto::common::SessionServerState::ERROR;
}
rialto::SessionServerState convert(const firebolt::rialto::common::SessionServerState &state)
{
    switch (state)
    {
    case firebolt::rialto::common::SessionServerState::UNINITIALIZED:
    {
        return rialto::SessionServerState::UNINITIALIZED;
    }
    case firebolt::rialto::common::SessionServerState::INACTIVE:
    {
        return rialto::SessionServerState::INACTIVE;
    }
    case firebolt::rialto::common::SessionServerState::ACTIVE:
    {
        return rialto::SessionServerState::ACTIVE;
    }
    case firebolt::rialto::common::SessionServerState::NOT_RUNNING:
    {
        return rialto::SessionServerState::NOT_RUNNING;
    }
    case firebolt::rialto::common::SessionServerState::ERROR:
    {
        return rialto::SessionServerState::ERROR;
    }
    }
    return rialto::SessionServerState::ERROR;
}
rialto::LogLevels convert(const rialto::servermanager::service::LoggingLevels &levels)
{
    rialto::LogLevels logLevels;
    if (rialto::servermanager::service::LoggingLevel::UNCHANGED != levels.defaultLoggingLevel)
    {
        logLevels.set_defaultloglevels(rialto::servermanager::common::convert(levels.defaultLoggingLevel));
    }
    if (rialto::servermanager::service::LoggingLevel::UNCHANGED != levels.clientLoggingLevel)
    {
        logLevels.set_clientloglevels(rialto::servermanager::common::convert(levels.clientLoggingLevel));
    }
    if (rialto::servermanager::service::LoggingLevel::UNCHANGED != levels.sessionServerLoggingLevel)
    {
        logLevels.set_sessionserverloglevels(rialto::servermanager::common::convert(levels.sessionServerLoggingLevel));
    }
    if (rialto::servermanager::service::LoggingLevel::UNCHANGED != levels.ipcLoggingLevel)
    {
        logLevels.set_ipcloglevels(rialto::servermanager::common::convert(levels.ipcLoggingLevel));
    }
    if (rialto::servermanager::service::LoggingLevel::UNCHANGED != levels.serverManagerLoggingLevel)
    {
        logLevels.set_servermanagerloglevels(rialto::servermanager::common::convert(levels.serverManagerLoggingLevel));
    }
    if (rialto::servermanager::service::LoggingLevel::UNCHANGED != levels.commonLoggingLevel)
    {
        logLevels.set_commonloglevels(rialto::servermanager::common::convert(levels.commonLoggingLevel));
    }
    return logLevels;
}
rialto::LogLevels getCurrentLogLevels()
{
    rialto::LogLevels logLevels;
    logLevels.set_defaultloglevels(firebolt::rialto::logging::getLogLevels(RIALTO_COMPONENT_DEFAULT));
    logLevels.set_clientloglevels(firebolt::rialto::logging::getLogLevels(RIALTO_COMPONENT_CLIENT));
    logLevels.set_sessionserverloglevels(firebolt::rialto::logging::getLogLevels(RIALTO_COMPONENT_SERVER));
    logLevels.set_ipcloglevels(firebolt::rialto::logging::getLogLevels(RIALTO_COMPONENT_IPC));
    logLevels.set_servermanagerloglevels(firebolt::rialto::logging::getLogLevels(RIALTO_COMPONENT_SERVER_MANAGER));
    logLevels.set_commonloglevels(firebolt::rialto::logging::getLogLevels(RIALTO_COMPONENT_COMMON));
    return logLevels;
}
} // namespace

namespace rialto::servermanager::ipc
{
Client::Client(std::unique_ptr<common::ISessionServerAppManager> &sessionServerAppManager, int serverId, int socket)
    : m_serverId{serverId}, m_sessionServerAppManager{sessionServerAppManager}, m_socket{socket}
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Constructing client for serverId: %d", m_serverId);
}

Client::~Client()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Client for serverId: %d is destructed", m_serverId);
    m_serviceStub.reset();
    m_ipcLoop.reset();
}

bool Client::connect()
{
    m_ipcLoop = IpcLoop::create(m_socket, *this);
    if (!m_ipcLoop)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to connect to rialto session server @ '%d'", m_socket);
        return false;
    }
    m_serviceStub = std::make_unique<::rialto::ServerManagerModule_Stub>(m_ipcLoop->channel());
    m_ipcLoop->channel()->subscribe<rialto::StateChangedEvent>(
        std::bind(&Client::onStateChangedEvent, this, std::placeholders::_1));
    m_ipcLoop->channel()->subscribe<rialto::AckEvent>(std::bind(&Client::onAckEvent, this, std::placeholders::_1));
    return true;
}

bool Client::performSetState(const firebolt::rialto::common::SessionServerState &state)
{
    if (!m_ipcLoop || !m_serviceStub)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("failed to set status - client is not active for serverId: %d", m_serverId);
        return false;
    }
    rialto::SetStateRequest request;
    rialto::SetStateResponse response;
    request.set_sessionserverstate(convert(state));
    auto ipcController = m_ipcLoop->createRpcController();
    auto blockingClosure = m_ipcLoop->createBlockingClosure();
    m_serviceStub->setState(ipcController.get(), &request, &response, blockingClosure.get());
    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("failed to set status due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }
    return true;
}

bool Client::performSetConfiguration(const firebolt::rialto::common::SessionServerState &initialState,
                                     const std::string &socketName, const std::string &clientDisplayName,
                                     const firebolt::rialto::common::MaxResourceCapabilitites &maxResource) const
{
    if (!m_ipcLoop || !m_serviceStub)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("failed to set configuration - client is not active for serverId: %d",
                                        m_serverId);
        return false;
    }
    rialto::SetConfigurationRequest request;
    rialto::SetConfigurationResponse response;
    request.set_sessionmanagementsocketname(socketName);
    request.set_clientdisplayname(clientDisplayName);
    request.mutable_resources()->set_maxplaybacks(maxResource.maxPlaybacks);
    request.mutable_resources()->set_maxwebaudioplayers(maxResource.maxWebAudioPlayers);
    *(request.mutable_loglevels()) = getCurrentLogLevels();
    request.set_initialsessionserverstate(convert(initialState));
    auto ipcController = m_ipcLoop->createRpcController();
    auto blockingClosure = m_ipcLoop->createBlockingClosure();
    m_serviceStub->setConfiguration(ipcController.get(), &request, &response, blockingClosure.get());
    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("failed to set configuration due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }
    return true;
}

bool Client::performPing(int pingId) const
{
    if (!m_ipcLoop || !m_serviceStub)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("failed to ping - client is not active for serverId: %d", m_serverId);
        return false;
    }
    rialto::PingRequest request;
    rialto::PingResponse response;
    request.set_id(pingId);
    auto ipcController = m_ipcLoop->createRpcController();
    auto blockingClosure = m_ipcLoop->createBlockingClosure();
    m_serviceStub->ping(ipcController.get(), &request, &response, blockingClosure.get());
    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("failed to ping due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }
    return true;
}

bool Client::setLogLevels(const service::LoggingLevels &logLevels) const
{
    if (!m_ipcLoop || !m_serviceStub)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("failed to change log levels - client is not active for serverId: %d", m_serverId);
        return false;
    }
    rialto::SetLogLevelsRequest request;
    rialto::SetLogLevelsResponse response;
    *(request.mutable_loglevels()) = convert(logLevels);
    auto ipcController = m_ipcLoop->createRpcController();
    auto blockingClosure = m_ipcLoop->createBlockingClosure();
    m_serviceStub->setLogLevels(ipcController.get(), &request, &response, blockingClosure.get());
    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("failed to change log levels due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }
    return true;
}

void Client::onDisconnected() const
{
    RIALTO_SERVER_MANAGER_LOG_WARN("Connection to serverId: %d broken!", m_serverId);
    m_sessionServerAppManager->onSessionServerStateChanged(m_serverId,
                                                           firebolt::rialto::common::SessionServerState::NOT_RUNNING);
}

void Client::onStateChangedEvent(const std::shared_ptr<rialto::StateChangedEvent> &event) const
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("StateChangedEvent received for serverId: %d", m_serverId);
    if (!m_sessionServerAppManager || !event)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("Problem during StateChangedEvent processing");
        return;
    }
    m_sessionServerAppManager->onSessionServerStateChanged(m_serverId, convert(event->sessionserverstate()));
}

void Client::onAckEvent(const std::shared_ptr<rialto::AckEvent> &event) const
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("AckEvent received for serverId: %d", m_serverId);
    if (!m_sessionServerAppManager || !event)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("Problem during AckEvent processing");
        return;
    }
    m_sessionServerAppManager->onAck(m_serverId, event->id(), event->success());
}
} // namespace rialto::servermanager::ipc
