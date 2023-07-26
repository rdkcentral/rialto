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

#include "SessionManagementServer.h"
#include "IControlModuleService.h"
#include "IMediaKeysCapabilitiesModuleService.h"
#include "IMediaKeysModuleService.h"
#include "IMediaPipelineModuleService.h"
#include "IWebAudioPlayerModuleService.h"
#include "RialtoServerLogging.h"
#include <IIpcServerFactory.h>
#include <sys/stat.h>

namespace firebolt::rialto::server::ipc
{
SessionManagementServer::SessionManagementServer(
    const std::shared_ptr<firebolt::rialto::ipc::IServerFactory> &ipcFactory,
    const std::shared_ptr<IMediaPipelineModuleServiceFactory> &mediaPipelineModuleFactory,
    const std::shared_ptr<IMediaPipelineCapabilitiesModuleServiceFactory> &mediaPipelineCapabilitiesModuleFactory,
    const std::shared_ptr<IMediaKeysModuleServiceFactory> &mediaKeysModuleFactory,
    const std::shared_ptr<IMediaKeysCapabilitiesModuleServiceFactory> &mediaKeysCapabilitiesModuleFactory,
    const std::shared_ptr<IWebAudioPlayerModuleServiceFactory> &webAudioPlayerModuleFactory,
    const std::shared_ptr<IControlModuleServiceFactory> &controlModuleFactory, service::IPlaybackService &playbackService,
    service::ICdmService &cdmService, service::IControlService &controlService)
    : m_isRunning{false}, m_mediaPipelineModule{mediaPipelineModuleFactory->create(
                              playbackService.getMediaPipelineService())},
      m_mediaPipelineCapabilitiesModule{
          mediaPipelineCapabilitiesModuleFactory->create(playbackService.getMediaPipelineService())},
      m_mediaKeysModule{mediaKeysModuleFactory->create(cdmService)},
      m_mediaKeysCapabilitiesModule{mediaKeysCapabilitiesModuleFactory->create(cdmService)},
      m_webAudioPlayerModule{webAudioPlayerModuleFactory->create(playbackService.getWebAudioPlayerService())},
      m_controlModule{controlModuleFactory->create(playbackService, controlService)}
{
    m_ipcServer = ipcFactory->create();
}

SessionManagementServer::~SessionManagementServer()
{
    stop();
    if (m_ipcServerThread.joinable())
    {
        m_ipcServerThread.join();
    }
}

bool SessionManagementServer::initialize(const std::string &socketName, unsigned int socketPermissions)
{
    RIALTO_SERVER_LOG_INFO("Initializing Session Management Server. Socket name: %s", socketName.c_str());
    if (!m_ipcServer)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to initialize SessionManagementServer - Ipc server instance is NULL");
        return false;
    }

    // add a socket for clients and associate with a streamer object
    if (!m_ipcServer->addSocket(socketName,
                                std::bind(&SessionManagementServer::onClientConnected, this, std::placeholders::_1),
                                std::bind(&SessionManagementServer::onClientDisconnected, this, std::placeholders::_1)))
    {
        RIALTO_SERVER_LOG_ERROR("Failed to initialize SessionManagementServer - can't add socket %s to the ipc server",
                                socketName.c_str());
        return false;
    }

    // set full read / write access for everyone on the socket for now
    if (chmod(socketName.c_str(), 0777) != 0)
    {
        RIALTO_SERVER_LOG_SYS_WARN(errno, "Failed to change the permissions on the IPC socket");
    }
    return true;
}

void SessionManagementServer::start()
{
    if (m_isRunning.load())
    {
        RIALTO_SERVER_LOG_DEBUG("Server is already in running state");
        return;
    }
    RIALTO_SERVER_LOG_DEBUG("Starting Session Management Server event loop");
    m_isRunning.store(true);
    m_ipcServerThread = std::thread(
        [this]()
        {
            constexpr int pollInterval{100};
            while (m_ipcServer->process() && m_isRunning.load())
            {
                m_ipcServer->wait(pollInterval);
            }
            RIALTO_SERVER_LOG_DEBUG("Session Management Server event loop finished.");
        });
}

void SessionManagementServer::stop()
{
    m_isRunning.store(false);
}

void SessionManagementServer::setLogLevels(RIALTO_DEBUG_LEVEL defaultLogLevels, RIALTO_DEBUG_LEVEL clientLogLevels,
                                           RIALTO_DEBUG_LEVEL ipcLogLevels, RIALTO_DEBUG_LEVEL commonLogLevels)
{
    m_setLogLevelsService.setLogLevels(defaultLogLevels, clientLogLevels, ipcLogLevels, commonLogLevels);
}

void SessionManagementServer::onClientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &client)
{
    m_controlModule->clientConnected(client);
    m_mediaPipelineModule->clientConnected(client);
    m_mediaPipelineCapabilitiesModule->clientConnected(client);
    m_mediaKeysModule->clientConnected(client);
    m_mediaKeysCapabilitiesModule->clientConnected(client);
    m_webAudioPlayerModule->clientConnected(client);
    m_setLogLevelsService.clientConnected(client);
}

void SessionManagementServer::onClientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &client)
{
    m_setLogLevelsService.clientDisconnected(client);
    m_mediaKeysCapabilitiesModule->clientDisconnected(client);
    m_mediaKeysModule->clientDisconnected(client);
    m_mediaPipelineCapabilitiesModule->clientDisconnected(client);
    m_mediaPipelineModule->clientDisconnected(client);
    m_webAudioPlayerModule->clientDisconnected(client);
    m_controlModule->clientDisconnected(client);
}
} // namespace firebolt::rialto::server::ipc
