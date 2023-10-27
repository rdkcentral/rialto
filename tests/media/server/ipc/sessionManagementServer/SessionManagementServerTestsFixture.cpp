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
#include "LinuxWrapperMock.h"
#include "MediaKeysCapabilitiesModuleServiceFactoryMock.h"
#include "MediaKeysModuleServiceFactoryMock.h"
#include "MediaPipelineCapabilitiesModuleServiceFactoryMock.h"
#include "MediaPipelineModuleServiceFactoryMock.h"
#include "SessionManagementServer.h"
#include "WebAudioPlayerModuleServiceFactoryMock.h"
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;
using testing::SetArgReferee;

namespace
{
const std::string socketName{"/tmp/sessionmanagementservertest-0"};
constexpr unsigned int socketPermissions{0666};
const std::string socketOwner{};
const std::string socketGroup{};
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
    EXPECT_CALL(*serverFactoryMock, create()).WillOnce(Return(m_serverMock));
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
    std::unique_ptr<firebolt::rialto::common::ILinuxWrapper> linuxWrapper =
        firebolt::rialto::common::ILinuxWrapperFactory::createFactory()->createLinuxWrapper();
    EXPECT_TRUE(m_sut->initialize(linuxWrapper, socketName, socketPermissions, socketOwner, socketGroup));
}

void SessionManagementServerTests::sendServerInitializeAndExpectFailure()
{
    std::unique_ptr<firebolt::rialto::common::ILinuxWrapper> linuxWrapper =
        firebolt::rialto::common::ILinuxWrapperFactory::createFactory()->createLinuxWrapper();
    EXPECT_FALSE(m_sut->initialize(linuxWrapper, socketName, socketPermissions, socketOwner, socketGroup));
}

void SessionManagementServerTests::testSocketOwnership(bool testOwnerIsValid, bool testGroupIsValid)
{
    std::unique_ptr<testing::StrictMock<firebolt::rialto::common::LinuxWrapperMock>> linuxWrapperMock{
        std::make_unique<testing::StrictMock<firebolt::rialto::common::LinuxWrapperMock>>()};

    const std::string kTestOwner{"own1"}; // any test string is ok
    const std::string kTestGroup{"grp1"}; // any test string is ok
    const uid_t kTestOwnerId = 12;        // any +ve test number is ok
    const gid_t kTestGroupId = 14;        // any +ve test number is ok
    const uid_t kDontChangeOwner = -1;
    const gid_t kDontChangeGroup = -1;

    serverWillInitialize();
    EXPECT_CALL(*linuxWrapperMock, chmod(testing::StrEq(socketName), socketPermissions)).WillOnce(Return(0));

    struct passwd passwordStruct
    {
    };
    passwordStruct.pw_uid = kTestOwnerId;
    EXPECT_CALL(*linuxWrapperMock, getpwnam_r(testing::StrEq(kTestOwner), _, _, _, _))
        .WillOnce(testing::DoAll(testing::SetArgPointee<4>(&passwordStruct), Return(testOwnerIsValid ? 0 : 1)));

    struct group groupStruct
    {
    };
    groupStruct.gr_gid = kTestGroupId;
    EXPECT_CALL(*linuxWrapperMock, getgrnam_r(testing::StrEq(kTestGroup), _, _, _, _))
        .WillOnce(testing::DoAll(testing::SetArgPointee<4>(&groupStruct), Return(testGroupIsValid ? 0 : 1)));

    if (testOwnerIsValid || testGroupIsValid)
    {
        EXPECT_CALL(*linuxWrapperMock,
                    chown(testing::StrEq(socketName), testOwnerIsValid ? kTestOwnerId : kDontChangeOwner,
                          testGroupIsValid ? kTestGroupId : kDontChangeGroup))
            .WillOnce(Return(0));
    }

    std::unique_ptr<firebolt::rialto::common::ILinuxWrapper> linuxWrapper = std::move(linuxWrapperMock);
    EXPECT_TRUE(m_sut->initialize(linuxWrapper, socketName, socketPermissions, kTestOwner, kTestGroup));
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
