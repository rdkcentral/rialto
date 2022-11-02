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
rialto::servermanager::service::SessionServerState convert(const rialto::SessionServerState &sessionServerState)
{
    switch (sessionServerState)
    {
    case rialto::SessionServerState::UNINITIALIZED:
    {
        return rialto::servermanager::service::SessionServerState::UNINITIALIZED;
    }
    case rialto::SessionServerState::INACTIVE:
    {
        return rialto::servermanager::service::SessionServerState::INACTIVE;
    }
    case rialto::SessionServerState::ACTIVE:
    {
        return rialto::servermanager::service::SessionServerState::ACTIVE;
    }
    case rialto::SessionServerState::NOT_RUNNING:
    {
        return rialto::servermanager::service::SessionServerState::NOT_RUNNING;
    }
    case rialto::SessionServerState::ERROR:
    {
        return rialto::servermanager::service::SessionServerState::ERROR;
    }
    }
    return rialto::servermanager::service::SessionServerState::ERROR;
}
rialto::SessionServerState convert(const rialto::servermanager::service::SessionServerState &state)
{
    switch (state)
    {
    case rialto::servermanager::service::SessionServerState::UNINITIALIZED:
    {
        return rialto::SessionServerState::UNINITIALIZED;
    }
    case rialto::servermanager::service::SessionServerState::INACTIVE:
    {
        return rialto::SessionServerState::INACTIVE;
    }
    case rialto::servermanager::service::SessionServerState::ACTIVE:
    {
        return rialto::SessionServerState::ACTIVE;
    }
    case rialto::servermanager::service::SessionServerState::NOT_RUNNING:
    {
        return rialto::SessionServerState::NOT_RUNNING;
    }
    case rialto::servermanager::service::SessionServerState::ERROR:
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
Client::Client(std::unique_ptr<common::ISessionServerAppManager> &sessionServerAppManager, const std::string &appId,
               int socket)
    : m_appId{appId}, m_sessionServerAppManager{sessionServerAppManager}, m_socket{socket}
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Constructing client for '%s'", m_appId.c_str());
}

Client::~Client()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Client for '%s' is destructed", m_appId.c_str());
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
    return true;
}

bool Client::performSetState(const service::SessionServerState &state)
{
    if (!m_ipcLoop || !m_serviceStub)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("failed to set status - client is not active for: %s", m_appId.c_str());
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

bool Client::performSetConfiguration(const service::SessionServerState &initialState, const std::string &socketName,
                                     int maxPlaybackSessions) const
{
    if (!m_ipcLoop || !m_serviceStub)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("failed to set configuration - client is not active for: %s", m_appId.c_str());
        return false;
    }
    rialto::SetConfigurationRequest request;
    rialto::SetConfigurationResponse response;
    request.set_sessionmanagementsocketname(socketName);
    request.mutable_resources()->set_maxplaybacks(maxPlaybackSessions);
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

bool Client::setLogLevels(const service::LoggingLevels &logLevels) const
{
    if (!m_ipcLoop || !m_serviceStub)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("failed to change log levels - client is not active for: %s", m_appId.c_str());
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
    RIALTO_SERVER_MANAGER_LOG_WARN("Connection to %s broken!", m_appId.c_str());
    m_sessionServerAppManager->onSessionServerStateChanged(m_appId, service::SessionServerState::NOT_RUNNING);
}

void Client::onStateChangedEvent(const std::shared_ptr<rialto::StateChangedEvent> &event) const
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("StateChangedEvent received for %s", m_appId.c_str());
    if (!m_sessionServerAppManager || !event)
    {
        RIALTO_SERVER_MANAGER_LOG_WARN("Problem during StateChangedEvent processing");
        return;
    }
    m_sessionServerAppManager->onSessionServerStateChanged(m_appId, convert(event->sessionserverstate()));
}
} // namespace rialto::servermanager::ipc
