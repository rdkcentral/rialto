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

#ifndef SESSION_MANAGEMENT_SERVER_TESTS_FIXTURE_H_
#define SESSION_MANAGEMENT_SERVER_TESTS_FIXTURE_H_

#include "CdmServiceMock.h"
#include "ClientMock.h"
#include "ControlModuleServiceMock.h"
#include "ControlServiceMock.h"
#include "ISessionManagementServer.h"
#include "IpcServerMock.h"
#include "MediaKeysCapabilitiesModuleServiceMock.h"
#include "MediaKeysModuleServiceMock.h"
#include "MediaPipelineCapabilitiesModuleServiceMock.h"
#include "MediaPipelineModuleServiceMock.h"
#include "MediaPipelineServiceMock.h"
#include "PlaybackServiceMock.h"
#include "WebAudioPlayerModuleServiceMock.h"
#include "WebAudioPlayerServiceMock.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class SessionManagementServerTests : public testing::Test
{
public:
    SessionManagementServerTests();
    ~SessionManagementServerTests() override;

    void serverWillInitialize();
    void serverWillFailToInitialize();
    void serverWillInitializeWithFd();
    void serverWillFailToInitializeWithFd();
    void serverWillStart();
    void clientWillConnect();
    void clientWillDisconnect();
    void serverWillSetLogLevels();

    void sendServerInitialize();
    void sendServerInitializeWithTestSocketOwnerAndGroup();
    void sendServerInitializeAndExpectFailure();
    void sendServerInitializeWithFd();
    void sendServerInitializeWithFdAndExpectFailure();
    void sendServerStart();
    void sendConnectClient();
    void sendDisconnectClient();
    void sendSetLogLevels();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_clientMock;
    StrictMock<firebolt::rialto::server::service::PlaybackServiceMock> m_playbackServiceMock;
    StrictMock<firebolt::rialto::server::service::MediaPipelineServiceMock> m_mediaPipelineServiceMock;
    StrictMock<firebolt::rialto::server::service::WebAudioPlayerServiceMock> m_webAudioPlayerServiceMock;
    StrictMock<firebolt::rialto::server::service::CdmServiceMock> m_cdmServiceMock;
    StrictMock<firebolt::rialto::server::service::ControlServiceMock> m_controlServiceMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ServerMock>> m_serverMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::MediaPipelineModuleServiceMock>> m_mediaPipelineModuleMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::MediaPipelineCapabilitiesModuleServiceMock>>
        m_mediaPipelineCapabilitiesModuleMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::MediaKeysModuleServiceMock>> m_mediaKeysModuleMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::MediaKeysCapabilitiesModuleServiceMock>>
        m_mediaKeysCapabilitiesModuleMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::WebAudioPlayerModuleServiceMock>> m_webAudioPlayerModuleMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::ipc::ControlModuleServiceMock>> m_controlModuleMock;
    std::unique_ptr<firebolt::rialto::server::ipc::ISessionManagementServer> m_sut;

    std::function<void(const std::shared_ptr<firebolt::rialto::ipc::IClient> &)> m_clientConnectedCb;
    std::function<void(const std::shared_ptr<firebolt::rialto::ipc::IClient> &)> m_clientDisconnectedCb;
};

#endif // SESSION_MANAGEMENT_SERVER_TESTS_FIXTURE_H_
