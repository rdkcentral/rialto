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

#include "SessionServerManagerTestsFixture.h"
#include "SessionServerManager.h"
#include <string>
#include <utility>

using firebolt::rialto::ApplicationState;
using firebolt::rialto::common::SessionServerState;
using firebolt::rialto::server::AckSenderMock;
using firebolt::rialto::server::HeartbeatProcedureFactoryMock;
using firebolt::rialto::server::HeartbeatProcedureMock;
using firebolt::rialto::server::ipc::ApplicationManagementServerMock;
using firebolt::rialto::server::ipc::SessionManagementServerMock;
using firebolt::rialto::server::service::SessionServerManager;
using testing::_;
using testing::ByMove;
using testing::Return;
using testing::Throw;

namespace
{
constexpr int kAppManagementSocket{3};
constexpr int kMaxPlaybacks{2};
constexpr int kMaxWebAudioPlayers{1};
constexpr int kPingId{12};
constexpr firebolt::rialto::common::MaxResourceCapabilitites kMaxResource{kMaxPlaybacks, kMaxWebAudioPlayers};
constexpr RIALTO_DEBUG_LEVEL kLogLvl{RIALTO_DEBUG_LEVEL_DEFAULT};
const std::string kSessionManagementSocket{"/tmp/rialtosessionservermanagertests-0"};
constexpr unsigned int kSessionManagementSocketPermissions{0777};
// Empty strings for kSocketOwner and kSocketGroup means that chown() won't be called. This will leave the created
// socket being owned by the user executing the code (and the group would be their primary group)
const std::string kSocketOwner{};
const std::string kSocketGroup{};
const std::string kClientDisplayName{"westeros-rialto"};
} // namespace

SessionServerManagerTests::SessionServerManagerTests()
    : m_applicationManagementServer{std::make_unique<StrictMock<ApplicationManagementServerMock>>()},
      m_applicationManagementServerMock{
          dynamic_cast<StrictMock<ApplicationManagementServerMock> &>(*m_applicationManagementServer)},
      m_sessionManagementServer{std::make_unique<StrictMock<SessionManagementServerMock>>()},
      m_sessionManagementServerMock{dynamic_cast<StrictMock<SessionManagementServerMock> &>(*m_sessionManagementServer)},
      m_ackSenderMock{std::make_shared<StrictMock<AckSenderMock>>()},
      m_heartbeatProcedureFactory{std::make_unique<StrictMock<HeartbeatProcedureFactoryMock>>()},
      m_heartbeatProcedureFactoryMock{*m_heartbeatProcedureFactory},
      m_heartbeatProcedureMock{std::make_shared<StrictMock<HeartbeatProcedureMock>>()}
{
    EXPECT_CALL(m_ipcFactoryMock, createApplicationManagementServer(_))
        .WillOnce(Return(ByMove(std::move(m_applicationManagementServer))));
    EXPECT_CALL(m_ipcFactoryMock, createSessionManagementServer(_, _, _))
        .WillOnce(Return(ByMove(std::move(m_sessionManagementServer))));
    m_sut = std::make_unique<SessionServerManager>(m_ipcFactoryMock, m_playbackServiceMock, m_cdmServiceMock,
                                                   m_controlServiceMock, std::move(m_heartbeatProcedureFactory));
}

SessionServerManagerTests::~SessionServerManagerTests()
{
    if (m_serviceThread.joinable())
    {
        m_serviceThread.join();
    }
}

void SessionServerManagerTests::willNotInitializeWithWrongNumberOfArgs()
{
    EXPECT_TRUE(m_sut);
    std::string arg1{"arg1"};
    std::string arg2{"arg2"};
    std::string arg3{"arg3"};
    char *args3[]{&arg1[0], &arg2[0], &arg3[0]};
    EXPECT_FALSE(m_sut->initialize(3, args3));
    char *args1[]{&arg1[0]};
    EXPECT_FALSE(m_sut->initialize(1, args1));
}

void SessionServerManagerTests::willNotInitializeWithWrongSocket()
{
    EXPECT_TRUE(m_sut);
    std::string arg1{"AppName"};
    std::string arg2{"socketStr"};
    char *args[]{&arg1[0], &arg2[0]};
    EXPECT_FALSE(m_sut->initialize(2, args));
}

void SessionServerManagerTests::willNotInitializeWhenApplicationManagementServerFailsToInit()
{
    EXPECT_CALL(m_applicationManagementServerMock, initialize(kAppManagementSocket)).WillOnce(Return(false));
    EXPECT_TRUE(m_sut);
    std::string arg1{"AppName"};
    std::string arg2{std::to_string(kAppManagementSocket)};
    char *args[]{&arg1[0], &arg2[0]};
    EXPECT_FALSE(m_sut->initialize(2, args));
}

void SessionServerManagerTests::willNotInitializeWhenApplicationManagementServerThrows()
{
    EXPECT_CALL(m_applicationManagementServerMock, initialize(kAppManagementSocket))
        .WillOnce(Throw(std::logic_error("some error")));
    EXPECT_TRUE(m_sut);
    std::string arg1{"AppName"};
    std::string arg2{std::to_string(kAppManagementSocket)};
    char *args[]{&arg1[0], &arg2[0]};
    EXPECT_FALSE(m_sut->initialize(2, args));
}

void SessionServerManagerTests::willNotInitializeWhenApplicationManagementServerFailsToSendEvent()
{
    EXPECT_CALL(m_applicationManagementServerMock, initialize(kAppManagementSocket)).WillOnce(Return(true));
    EXPECT_CALL(m_applicationManagementServerMock, start());
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::UNINITIALIZED))
        .WillOnce(Return(false));
    EXPECT_TRUE(m_sut);
    std::string arg1{"AppName"};
    std::string arg2{std::to_string(kAppManagementSocket)};
    char *args[]{&arg1[0], &arg2[0]};
    EXPECT_FALSE(m_sut->initialize(2, args));
}

void SessionServerManagerTests::willInitialize()
{
    EXPECT_CALL(m_applicationManagementServerMock, initialize(kAppManagementSocket)).WillOnce(Return(true));
    EXPECT_CALL(m_applicationManagementServerMock, start());
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::UNINITIALIZED))
        .WillOnce(Return(true));
    EXPECT_TRUE(m_sut);
    std::string arg1{"AppName"};
    std::string arg2{std::to_string(kAppManagementSocket)};
    char *args[]{&arg1[0], &arg2[0]};
    EXPECT_TRUE(m_sut->initialize(2, args));
}

void SessionServerManagerTests::willFailToSetConfigurationWhenSessionManagementServerFailsToInit()
{
    EXPECT_CALL(m_sessionManagementServerMock,
                initialize(kSessionManagementSocket, kSessionManagementSocketPermissions, kSocketOwner, kSocketGroup))
        .WillOnce(Return(false));
    EXPECT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->setConfiguration(kSessionManagementSocket, SessionServerState::INACTIVE, kMaxResource,
                                         kClientDisplayName, kSessionManagementSocketPermissions, kSocketOwner,
                                         kSocketGroup));
}

void SessionServerManagerTests::willFailToSetConfigurationWhenSessionManagementServerFailsToSetInitialState()
{
    EXPECT_CALL(m_sessionManagementServerMock,
                initialize(kSessionManagementSocket, kSessionManagementSocketPermissions, kSocketOwner, kSocketGroup))
        .WillOnce(Return(true));
    EXPECT_CALL(m_sessionManagementServerMock, start());
    EXPECT_CALL(m_playbackServiceMock, setMaxPlaybacks(kMaxPlaybacks));
    EXPECT_CALL(m_playbackServiceMock, setMaxWebAudioPlayers(kMaxWebAudioPlayers));
    EXPECT_CALL(m_playbackServiceMock, setClientDisplayName(kClientDisplayName));
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::INACTIVE))
        .WillOnce(Return(false));
    EXPECT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->setConfiguration(kSessionManagementSocket, SessionServerState::INACTIVE, kMaxResource,
                                         kClientDisplayName, kSessionManagementSocketPermissions, kSocketOwner,
                                         kSocketGroup));
}

void SessionServerManagerTests::willSetConfiguration()
{
    EXPECT_CALL(m_sessionManagementServerMock,
                initialize(kSessionManagementSocket, kSessionManagementSocketPermissions, kSocketOwner, kSocketGroup))
        .WillOnce(Return(true));
    EXPECT_CALL(m_sessionManagementServerMock, start());
    EXPECT_CALL(m_playbackServiceMock, setMaxPlaybacks(kMaxPlaybacks));
    EXPECT_CALL(m_playbackServiceMock, setMaxWebAudioPlayers(kMaxWebAudioPlayers));
    EXPECT_CALL(m_playbackServiceMock, setClientDisplayName(kClientDisplayName));
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_controlServiceMock, setApplicationState(ApplicationState::INACTIVE));
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::INACTIVE))
        .WillOnce(Return(true));
    EXPECT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->setConfiguration(kSessionManagementSocket, SessionServerState::INACTIVE, kMaxResource,
                                        kClientDisplayName, kSessionManagementSocketPermissions, kSocketOwner,
                                        kSocketGroup));
}

void SessionServerManagerTests::willFailToSetUnsupportedState()
{
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::ERROR))
        .Times(2)
        .WillRepeatedly(Return(true));
}

void SessionServerManagerTests::willFailToSetStateActiveDueToPlaybackServiceError()
{
    EXPECT_CALL(m_playbackServiceMock, switchToActive()).WillOnce(Return(false));
}

void SessionServerManagerTests::willFailToSetStateActiveDueToCdmServiceError()
{
    EXPECT_CALL(m_playbackServiceMock, switchToActive()).WillOnce(Return(true));
    EXPECT_CALL(m_cdmServiceMock, switchToActive()).WillOnce(Return(false));
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
}

void SessionServerManagerTests::willFailToSetStateActiveDueToSessionServerError()
{
    EXPECT_CALL(m_playbackServiceMock, switchToActive()).WillOnce(Return(true));
    EXPECT_CALL(m_cdmServiceMock, switchToActive()).WillOnce(Return(true));
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::ACTIVE)).WillOnce(Return(false));
}

void SessionServerManagerTests::willSetStateActive()
{
    EXPECT_CALL(m_playbackServiceMock, switchToActive()).WillOnce(Return(true));
    EXPECT_CALL(m_cdmServiceMock, switchToActive()).WillOnce(Return(true));
    EXPECT_CALL(m_controlServiceMock, setApplicationState(ApplicationState::RUNNING));
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::ACTIVE)).WillOnce(Return(true));
}

void SessionServerManagerTests::willFailToSetStateInactive()
{
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::INACTIVE))
        .WillOnce(Return(false));
}

void SessionServerManagerTests::willFailToSetStateInactiveAndGoBackToActive()
{
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_playbackServiceMock, switchToActive()).WillOnce(Return(false));
    EXPECT_CALL(m_cdmServiceMock, switchToActive()).WillOnce(Return(false));
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::INACTIVE))
        .WillOnce(Return(false));
}

void SessionServerManagerTests::willSetStateInactive()
{
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_controlServiceMock, setApplicationState(ApplicationState::INACTIVE));
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::INACTIVE))
        .WillOnce(Return(true));
}

void SessionServerManagerTests::willFailToSetStateNotRunning()
{
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_controlServiceMock, setApplicationState(ApplicationState::UNKNOWN));
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::NOT_RUNNING))
        .WillOnce(Return(false));
}

void SessionServerManagerTests::willSetStateNotRunning()
{
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_controlServiceMock, setApplicationState(ApplicationState::UNKNOWN));
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::NOT_RUNNING))
        .WillOnce(Return(true));
}

void SessionServerManagerTests::willPing()
{
    EXPECT_CALL(m_heartbeatProcedureFactoryMock, createHeartbeatProcedure(_, kPingId))
        .WillOnce(Return(m_heartbeatProcedureMock));
    EXPECT_CALL(m_cdmServiceMock, ping(_));
    EXPECT_CALL(m_playbackServiceMock, ping(_));
    EXPECT_CALL(m_controlServiceMock, ping(_)).WillOnce(Return(true));
}

void SessionServerManagerTests::willFailToPing()
{
    EXPECT_CALL(m_heartbeatProcedureFactoryMock, createHeartbeatProcedure(_, kPingId))
        .WillOnce(Return(m_heartbeatProcedureMock));
    EXPECT_CALL(m_cdmServiceMock, ping(_));
    EXPECT_CALL(m_playbackServiceMock, ping(_));
    EXPECT_CALL(m_controlServiceMock, ping(_)).WillOnce(Return(false));
}

void SessionServerManagerTests::willSetLogLevels()
{
    EXPECT_CALL(m_sessionManagementServerMock, setLogLevels(kLogLvl, kLogLvl, kLogLvl, kLogLvl));
}

void SessionServerManagerTests::setStateShouldFail(const SessionServerState &state)
{
    EXPECT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->setState(state));
}

void SessionServerManagerTests::setStateShouldSucceed(const SessionServerState &state)
{
    EXPECT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->setState(state));
}

void SessionServerManagerTests::triggerStartService()
{
    EXPECT_TRUE(m_sut);
    m_serviceThread = std::thread([&]() { m_sut->startService(); });
}

void SessionServerManagerTests::triggerSetLogLevels()
{
    EXPECT_TRUE(m_sut);
    m_sut->setLogLevels(kLogLvl, kLogLvl, kLogLvl, kLogLvl, kLogLvl, kLogLvl);
}

void SessionServerManagerTests::pingShouldSucceed()
{
    EXPECT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->ping(kPingId, m_ackSenderMock));
}

void SessionServerManagerTests::pingShouldFail()
{
    EXPECT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->ping(kPingId, m_ackSenderMock));
}
