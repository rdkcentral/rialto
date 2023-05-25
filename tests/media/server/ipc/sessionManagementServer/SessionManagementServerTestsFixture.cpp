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

#include "SessionManagementServerTestsFixture.h"
#include "ControlModuleServiceFactoryMock.h"
#include "IpcClientMock.h"
#include "IpcServerFactoryMock.h"
#include "MediaKeysCapabilitiesModuleServiceFactoryMock.h"
#include "MediaKeysModuleServiceFactoryMock.h"
#include "MediaPipelineCapabilitiesModuleServiceFactoryMock.h"
#include "MediaPipelineModuleServiceFactoryMock.h"
#include "SessionManagementServer.h"
#include "WebAudioPlayerModuleServiceFactoryMock.h"
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <vector>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;
using testing::SetArgReferee;

namespace
{
const std::string socketName{"/tmp/sessionmanagementservertest-0"};
const RIALTO_DEBUG_LEVEL defaultLogLevels{RIALTO_DEBUG_LEVEL_FATAL};
const RIALTO_DEBUG_LEVEL clientLogLevels{RIALTO_DEBUG_LEVEL_ERROR};
const RIALTO_DEBUG_LEVEL ipcLogLevels{RIALTO_DEBUG_LEVEL_DEBUG};
const RIALTO_DEBUG_LEVEL commonLogLevels{RIALTO_DEBUG_LEVEL_DEFAULT};
} // namespace

MATCHER_P4(SetLogLevelsEventMatcher, defaultLogLevels, clientLogLevels, ipcLogLevels, commonLogLevels, "")
{
    std::shared_ptr<firebolt::rialto::SetLogLevelsEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::SetLogLevelsEvent>(arg);
    return ((defaultLogLevels == event->defaultloglevels()) && (clientLogLevels == event->clientloglevels()) &&
            (ipcLogLevels == event->ipcloglevels()) && (commonLogLevels == event->commonloglevels()));
}

SessionManagementServerTests::SessionManagementServerTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_serverMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ServerMock>>()},
      m_mediaPipelineModuleMock{
          std::make_shared<StrictMock<firebolt::rialto::server::ipc::MediaPipelineModuleServiceMock>>()},
      m_mediaPipelineCapabilitiesModuleMock{
          std::make_shared<StrictMock<firebolt::rialto::server::ipc::MediaPipelineCapabilitiesModuleServiceMock>>()},
      m_mediaKeysModuleMock{std::make_shared<StrictMock<firebolt::rialto::server::ipc::MediaKeysModuleServiceMock>>()},
      m_mediaKeysCapabilitiesModuleMock{
          std::make_shared<StrictMock<firebolt::rialto::server::ipc::MediaKeysCapabilitiesModuleServiceMock>>()},
      m_webAudioPlayerModuleMock{
          std::make_shared<StrictMock<firebolt::rialto::server::ipc::WebAudioPlayerModuleServiceMock>>()},
      m_controlModuleMock{std::make_shared<StrictMock<firebolt::rialto::server::ipc::ControlModuleServiceMock>>()}
{
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ServerFactoryMock>> serverFactoryMock =
        std::make_shared<StrictMock<firebolt::rialto::ipc::ServerFactoryMock>>();
    EXPECT_CALL(*serverFactoryMock, create(_)).WillOnce(Return(m_serverMock));
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::MediaPipelineModuleServiceFactoryMock>>
        mediaPipelineModuleFactoryMock =
            std::make_shared<StrictMock<firebolt::rialto::server::ipc::MediaPipelineModuleServiceFactoryMock>>();
    EXPECT_CALL(*mediaPipelineModuleFactoryMock, create(_)).WillOnce(Return(m_mediaPipelineModuleMock));
    EXPECT_CALL(m_playbackServiceMock, getMediaPipelineService())
        .WillOnce(ReturnRef(m_mediaPipelineServiceMock))
        .RetiresOnSaturation();

    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::MediaPipelineCapabilitiesModuleServiceFactoryMock>>
        mediaPipelineCapabilitiesModuleFactoryMock =
            std::make_shared<StrictMock<firebolt::rialto::server::ipc::MediaPipelineCapabilitiesModuleServiceFactoryMock>>();
    EXPECT_CALL(*mediaPipelineCapabilitiesModuleFactoryMock, create(_))
        .WillOnce(Return(m_mediaPipelineCapabilitiesModuleMock));
    EXPECT_CALL(m_playbackServiceMock, getMediaPipelineService())
        .WillOnce(ReturnRef(m_mediaPipelineServiceMock))
        .RetiresOnSaturation();
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::MediaKeysModuleServiceFactoryMock>> mediaKeysModuleFactoryMock =
        std::make_shared<StrictMock<firebolt::rialto::server::ipc::MediaKeysModuleServiceFactoryMock>>();
    EXPECT_CALL(*mediaKeysModuleFactoryMock, create(_)).WillOnce(Return(m_mediaKeysModuleMock));
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::MediaKeysCapabilitiesModuleServiceFactoryMock>>
        mediaKeysCapabilitiesModuleFactoryMock =
            std::make_shared<StrictMock<firebolt::rialto::server::ipc::MediaKeysCapabilitiesModuleServiceFactoryMock>>();
    EXPECT_CALL(*mediaKeysCapabilitiesModuleFactoryMock, create(_)).WillOnce(Return(m_mediaKeysCapabilitiesModuleMock));
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::WebAudioPlayerModuleServiceFactoryMock>>
        webAudioPlayerModuleFactoryMock =
            std::make_shared<StrictMock<firebolt::rialto::server::ipc::WebAudioPlayerModuleServiceFactoryMock>>();
    EXPECT_CALL(*webAudioPlayerModuleFactoryMock, create(_)).WillOnce(Return(m_webAudioPlayerModuleMock));
    EXPECT_CALL(m_playbackServiceMock, getWebAudioPlayerService()).WillOnce(ReturnRef(m_webAudioPlayerServiceMock));
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::ControlModuleServiceFactoryMock>> controlModuleFactoryMock =
        std::make_shared<StrictMock<firebolt::rialto::server::ipc::ControlModuleServiceFactoryMock>>();
    EXPECT_CALL(*controlModuleFactoryMock, create(_, _)).WillOnce(Return(m_controlModuleMock));
    m_sut =
        std::make_unique<firebolt::rialto::server::ipc::SessionManagementServer>(serverFactoryMock,
                                                                                 mediaPipelineModuleFactoryMock,
                                                                                 mediaPipelineCapabilitiesModuleFactoryMock,
                                                                                 mediaKeysModuleFactoryMock,
                                                                                 mediaKeysCapabilitiesModuleFactoryMock,
                                                                                 webAudioPlayerModuleFactoryMock,
                                                                                 controlModuleFactoryMock,
                                                                                 m_playbackServiceMock,
                                                                                 m_cdmServiceMock, m_controlServiceMock);
}

SessionManagementServerTests::~SessionManagementServerTests() {}

void SessionManagementServerTests::serverWillInitialize()
{
    EXPECT_CALL(*m_serverMock, addSocket(socketName, _, _))
        .WillOnce(Invoke(
            [this](const std::string &socketPath,
                   std::function<void(const std::shared_ptr<firebolt::rialto::ipc::IClient> &)> clientConnectedCb,
                   std::function<void(const std::shared_ptr<firebolt::rialto::ipc::IClient> &)> clientDisconnectedCb)
            {
                m_clientConnectedCb = clientConnectedCb;
                m_clientDisconnectedCb = clientDisconnectedCb;
                return true;
            }));
}

void SessionManagementServerTests::serverWillFailToInitialize()
{
    EXPECT_CALL(*m_serverMock, addSocket(socketName, _, _)).WillOnce(Return(false));
}

void SessionManagementServerTests::serverWillStart()
{
    EXPECT_CALL(*m_serverMock, process()).WillOnce(Return(false));
}

void SessionManagementServerTests::clientWillConnect()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                clientConnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
    EXPECT_CALL(*m_mediaPipelineCapabilitiesModuleMock,
                clientConnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
    EXPECT_CALL(*m_mediaKeysModuleMock,
                clientConnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
    EXPECT_CALL(*m_mediaKeysCapabilitiesModuleMock,
                clientConnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                clientConnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
    EXPECT_CALL(*m_controlModuleMock,
                clientConnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
}

void SessionManagementServerTests::clientWillDisconnect()
{
    EXPECT_CALL(*m_mediaKeysCapabilitiesModuleMock,
                clientDisconnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
    EXPECT_CALL(*m_mediaKeysModuleMock,
                clientDisconnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
    EXPECT_CALL(*m_mediaPipelineCapabilitiesModuleMock,
                clientDisconnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                clientDisconnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                clientDisconnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
    EXPECT_CALL(*m_controlModuleMock,
                clientDisconnected(std::dynamic_pointer_cast<::firebolt::rialto::ipc::IClient>(m_clientMock)));
}

void SessionManagementServerTests::serverWillSetLogLevels()
{
    EXPECT_CALL(*m_clientMock,
                sendEvent(SetLogLevelsEventMatcher(defaultLogLevels, clientLogLevels, ipcLogLevels, commonLogLevels)));
}

void SessionManagementServerTests::sendServerInitialize()
{
    EXPECT_TRUE(m_sut->initialize(socketName));
}

void SessionManagementServerTests::sendServerInitializeAndExpectFailure()
{
    EXPECT_FALSE(m_sut->initialize(socketName));
}

void SessionManagementServerTests::sendServerStart()
{
    m_sut->start();
}

void SessionManagementServerTests::sendConnectClient()
{
    m_clientConnectedCb(m_clientMock);
}

void SessionManagementServerTests::sendDisconnectClient()
{
    m_clientDisconnectedCb(m_clientMock);
}

void SessionManagementServerTests::sendSetLogLevels()
{
    m_sut->setLogLevels(defaultLogLevels, clientLogLevels, ipcLogLevels, commonLogLevels);
}
