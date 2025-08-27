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

#include "ServerManagerModuleServiceTestsFixture.h"
#include "ServerManagerModuleService.h"
#include <string>

using testing::_;
using testing::Return;

namespace
{
constexpr int kSocketFd{17};
const std::string kSocketName{"/tmp/rialtotest-0"};
const std::string kClientDisplayName{"westeros-rialto"};
constexpr unsigned int kSocketPermissions{0666};
constexpr int kMaxSessions{5};
constexpr int kMaxWebAudioPlayers{3};
constexpr int kPingId{29};
// Empty strings for kSocketOwner and kSocketGroup means that chown() won't be called. This will leave the created
// socket being owned by the user executing the code (and the group would be their primary group)
const std::string kSocketOwner{};
const std::string kSocketGroup{};
const std::string kAppId{"app"};
const std::chrono::seconds kSubtitleResyncInterval{30};

rialto::SessionServerState convertSessionServerState(const firebolt::rialto::common::SessionServerState &state)
{
    switch (state)
    {
    case firebolt::rialto::common::SessionServerState::UNINITIALIZED:
        return rialto::SessionServerState::UNINITIALIZED;
    case firebolt::rialto::common::SessionServerState::INACTIVE:
        return rialto::SessionServerState::INACTIVE;
    case firebolt::rialto::common::SessionServerState::ACTIVE:
        return rialto::SessionServerState::ACTIVE;
    case firebolt::rialto::common::SessionServerState::NOT_RUNNING:
        return rialto::SessionServerState::NOT_RUNNING;
    case firebolt::rialto::common::SessionServerState::ERROR:
        return rialto::SessionServerState::ERROR;
    }
    return rialto::SessionServerState::ERROR;
}
} // namespace

MATCHER_P2(MaxResourceMatcher, maxPlaybacks, maxWebAudioPlayers, "")
{
    return ((maxPlaybacks == arg.maxPlaybacks) && (maxWebAudioPlayers == arg.maxWebAudioPlayers));
}

ServerManagerModuleServiceTests::ServerManagerModuleServiceTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_serverMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ServerMock>>()},
      m_closureMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClosureMock>>()},
      m_controllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ControllerMock>>()},
      m_invalidControllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::RpcControllerMock>>()}
{
    auto sutFactory = firebolt::rialto::server::ipc::IServerManagerModuleServiceFactory::createFactory();
    m_sut = sutFactory->create(m_sessionServerManagerMock);
}

ServerManagerModuleServiceTests::~ServerManagerModuleServiceTests() {}

void ServerManagerModuleServiceTests::sessionServerManagerWillSetConfiguration(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerManagerMock, configureIpc(kSocketName, kSocketPermissions, kSocketOwner, kSocketGroup))
        .WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerManagerMock,
                configureServices(state, MaxResourceMatcher(kMaxSessions, kMaxWebAudioPlayers), kClientDisplayName,
                                  kAppId, kSubtitleResyncInterval))
        .WillOnce(Return(true));
}

void ServerManagerModuleServiceTests::sessionServerManagerWillSetConfigurationWithFd(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerManagerMock, configureIpc(kSocketFd)).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerManagerMock,
                configureServices(state, MaxResourceMatcher(kMaxSessions, kMaxWebAudioPlayers), kClientDisplayName,
                                  kAppId, kSubtitleResyncInterval))
        .WillOnce(Return(true));
}

void ServerManagerModuleServiceTests::sessionServerManagerWillSetState(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerManagerMock, setState(state)).WillOnce(Return(true));
}

void ServerManagerModuleServiceTests::sessionServerManagerWillSetLogLevels()
{
    EXPECT_CALL(m_sessionServerManagerMock,
                setLogLevels(RIALTO_DEBUG_LEVEL_DEBUG, RIALTO_DEBUG_LEVEL_DEBUG, RIALTO_DEBUG_LEVEL_DEBUG,
                             RIALTO_DEBUG_LEVEL_DEBUG, RIALTO_DEBUG_LEVEL_DEBUG, RIALTO_DEBUG_LEVEL_DEBUG));
}

void ServerManagerModuleServiceTests::sessionServerManagerWillFailToSetConfiguration(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerManagerMock, configureIpc(kSocketName, kSocketPermissions, kSocketOwner, kSocketGroup))
        .WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerManagerMock,
                configureServices(state, MaxResourceMatcher(kMaxSessions, kMaxWebAudioPlayers), kClientDisplayName,
                                  kAppId, kSubtitleResyncInterval))
        .WillOnce(Return(false));
}

void ServerManagerModuleServiceTests::sessionServerManagerWillFailToSetConfigurationWithFd(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerManagerMock, configureIpc(kSocketFd)).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerManagerMock,
                configureServices(state, MaxResourceMatcher(kMaxSessions, kMaxWebAudioPlayers), kClientDisplayName,
                                  kAppId, kSubtitleResyncInterval))
        .WillOnce(Return(false));
}

void ServerManagerModuleServiceTests::sessionServerManagerWillFailToSetState(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerManagerMock, setState(state)).WillOnce(Return(false));
}

void ServerManagerModuleServiceTests::sessionServerManagerWillPing()
{
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_sessionServerManagerMock, ping(kPingId, _)).WillOnce(Return(true));
}

void ServerManagerModuleServiceTests::sessionServerManagerWillFailToPing()
{
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_sessionServerManagerMock, ping(kPingId, _)).WillOnce(Return(false));
}

void ServerManagerModuleServiceTests::sendSetConfiguration(const firebolt::rialto::common::SessionServerState &state)
{
    rialto::SetConfigurationRequest request;
    rialto::SetConfigurationResponse response;

    request.set_sessionmanagementsocketname(kSocketName);
    request.mutable_resources()->set_maxplaybacks(kMaxSessions);
    request.mutable_resources()->set_maxwebaudioplayers(kMaxWebAudioPlayers);
    request.mutable_loglevels()->set_defaultloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_clientloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_sessionserverloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_ipcloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_servermanagerloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_commonloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.set_initialsessionserverstate(convertSessionServerState(state));
    request.set_clientdisplayname(kClientDisplayName);
    request.set_socketpermissions(kSocketPermissions);
    request.set_socketowner(kSocketOwner);
    request.set_socketgroup(kSocketGroup);
    request.set_appname(kAppId);

    m_sut->setConfiguration(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void ServerManagerModuleServiceTests::sendSetConfigurationWithFd(const firebolt::rialto::common::SessionServerState &state)
{
    rialto::SetConfigurationRequest request;
    rialto::SetConfigurationResponse response;

    request.set_sessionmanagementsocketfd(kSocketFd);
    request.mutable_resources()->set_maxplaybacks(kMaxSessions);
    request.mutable_resources()->set_maxwebaudioplayers(kMaxWebAudioPlayers);
    request.mutable_loglevels()->set_defaultloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_clientloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_sessionserverloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_ipcloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_servermanagerloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_commonloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.set_initialsessionserverstate(convertSessionServerState(state));
    request.set_clientdisplayname(kClientDisplayName);
    request.set_appname(kAppId);

    m_sut->setConfiguration(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void ServerManagerModuleServiceTests::sendSetState(const firebolt::rialto::common::SessionServerState &state)
{
    rialto::SetStateRequest request;
    rialto::SetStateResponse response;

    request.set_sessionserverstate(convertSessionServerState(state));

    m_sut->setState(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void ServerManagerModuleServiceTests::sendSetLogLevels()
{
    rialto::SetLogLevelsRequest request;
    rialto::SetLogLevelsResponse response;

    request.mutable_loglevels()->set_defaultloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_clientloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_sessionserverloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_ipcloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_servermanagerloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_commonloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));

    m_sut->setLogLevels(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void ServerManagerModuleServiceTests::sendPing()
{
    rialto::PingRequest request;
    rialto::PingResponse response;

    request.set_id(kPingId);

    m_sut->ping(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void ServerManagerModuleServiceTests::sendPingWithInvalidController()
{
    rialto::PingRequest request;
    rialto::PingResponse response;

    request.set_id(kPingId);

    m_sut->ping(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
}

void ServerManagerModuleServiceTests::sessionServerManagerWillHandleRequestSuccess()
{
    EXPECT_CALL(*m_closureMock, Run());
}

void ServerManagerModuleServiceTests::sessionServerManagerWillHandleRequestFailure()
{
    EXPECT_CALL(*m_controllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
}

void ServerManagerModuleServiceTests::sessionServerManagerWillHandleRequestFailureWithInvalidController()
{
    EXPECT_CALL(*m_invalidControllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
}
