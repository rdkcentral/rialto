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
const std::string SOCKET_NAME{"/tmp/rialtotest-0"};
constexpr int MAX_SESSIONS{5};
constexpr int MAX_WEB_AUDIO_PLAYERS{3};
constexpr int socket{2};
constexpr int pingId{29};

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
      m_controllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ControllerMock>>()}
{
    auto sutFactory = firebolt::rialto::server::ipc::IServerManagerModuleServiceFactory::createFactory();
    m_sut = sutFactory->create(m_sessionServerManagerMock);
}

ServerManagerModuleServiceTests::~ServerManagerModuleServiceTests() {}

void ServerManagerModuleServiceTests::sessionServerManagerWillSetConfiguration(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerManagerMock,
                setConfiguration(SOCKET_NAME, state, MaxResourceMatcher(MAX_SESSIONS, MAX_WEB_AUDIO_PLAYERS)))
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
    EXPECT_CALL(m_sessionServerManagerMock,
                setConfiguration(SOCKET_NAME, state, MaxResourceMatcher(MAX_SESSIONS, MAX_WEB_AUDIO_PLAYERS)))
        .WillOnce(Return(false));
}

void ServerManagerModuleServiceTests::sessionServerManagerWillFailToSetState(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerManagerMock, setState(state)).WillOnce(Return(false));
}

void ServerManagerModuleServiceTests::sessionServerManagerWillPing()
{
    EXPECT_CALL(m_sessionServerManagerMock, ping(pingId)).WillOnce(Return(true));
}

void ServerManagerModuleServiceTests::sessionServerManagerWillFailToPing()
{
    EXPECT_CALL(m_sessionServerManagerMock, ping(pingId)).WillOnce(Return(false));
}

void ServerManagerModuleServiceTests::sendSetConfiguration(const firebolt::rialto::common::SessionServerState &state)
{
    rialto::SetConfigurationRequest request;
    rialto::SetConfigurationResponse response;

    request.set_sessionmanagementsocketname(SOCKET_NAME);
    request.mutable_resources()->set_maxplaybacks(MAX_SESSIONS);
    request.mutable_resources()->set_maxwebaudioplayers(MAX_WEB_AUDIO_PLAYERS);
    request.mutable_loglevels()->set_defaultloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_clientloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_sessionserverloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_ipcloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_servermanagerloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.mutable_loglevels()->set_commonloglevels(static_cast<uint32_t>(RIALTO_DEBUG_LEVEL_DEBUG));
    request.set_initialsessionserverstate(convertSessionServerState(state));

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

    request.set_id(pingId);

    m_sut->ping(m_controllerMock.get(), &request, &response, m_closureMock.get());
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
