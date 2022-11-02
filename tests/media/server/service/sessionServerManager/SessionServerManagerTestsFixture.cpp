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

using firebolt::rialto::server::SessionServerState;
using firebolt::rialto::server::ipc::ApplicationManagementServerMock;
using firebolt::rialto::server::ipc::SessionManagementServerMock;
using firebolt::rialto::server::service::SessionServerManager;
using testing::_;
using testing::ByMove;
using testing::Return;
using testing::Throw;

namespace
{
constexpr int appManagementSocket{3};
constexpr int maxPlaybacks{2};
constexpr RIALTO_DEBUG_LEVEL logLvl{RIALTO_DEBUG_LEVEL_DEFAULT};
const std::string sessionManagementSocket{"/tmp/rialtosessionservermanagertests-0"};
} // namespace

SessionServerManagerTests::SessionServerManagerTests()
    : m_applicationManagementServer{std::make_unique<StrictMock<ApplicationManagementServerMock>>()},
      m_applicationManagementServerMock{
          dynamic_cast<StrictMock<ApplicationManagementServerMock> &>(*m_applicationManagementServer)},
      m_sessionManagementServer{std::make_unique<StrictMock<SessionManagementServerMock>>()},
      m_sessionManagementServerMock{dynamic_cast<StrictMock<SessionManagementServerMock> &>(*m_sessionManagementServer)}
{
    EXPECT_CALL(m_ipcFactoryMock, createApplicationManagementServer(_))
        .WillOnce(Return(ByMove(std::move(m_applicationManagementServer))));
    EXPECT_CALL(m_ipcFactoryMock, createSessionManagementServer(_, _))
        .WillOnce(Return(ByMove(std::move(m_sessionManagementServer))));
    m_sut = std::make_unique<SessionServerManager>(m_ipcFactoryMock, m_playbackServiceMock, m_cdmServiceMock);
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
    EXPECT_CALL(m_applicationManagementServerMock, initialize(appManagementSocket)).WillOnce(Return(false));
    EXPECT_TRUE(m_sut);
    std::string arg1{"AppName"};
    std::string arg2{std::to_string(appManagementSocket)};
    char *args[]{&arg1[0], &arg2[0]};
    EXPECT_FALSE(m_sut->initialize(2, args));
}

void SessionServerManagerTests::willNotInitializeWhenApplicationManagementServerThrows()
{
    EXPECT_CALL(m_applicationManagementServerMock, initialize(appManagementSocket))
        .WillOnce(Throw(std::logic_error("some error")));
    EXPECT_TRUE(m_sut);
    std::string arg1{"AppName"};
    std::string arg2{std::to_string(appManagementSocket)};
    char *args[]{&arg1[0], &arg2[0]};
    EXPECT_FALSE(m_sut->initialize(2, args));
}

void SessionServerManagerTests::willNotInitializeWhenApplicationManagementServerFailsToSendEvent()
{
    EXPECT_CALL(m_applicationManagementServerMock, initialize(appManagementSocket)).WillOnce(Return(true));
    EXPECT_CALL(m_applicationManagementServerMock, start());
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::UNINITIALIZED))
        .WillOnce(Return(false));
    EXPECT_TRUE(m_sut);
    std::string arg1{"AppName"};
    std::string arg2{std::to_string(appManagementSocket)};
    char *args[]{&arg1[0], &arg2[0]};
    EXPECT_FALSE(m_sut->initialize(2, args));
}

void SessionServerManagerTests::willInitialize()
{
    EXPECT_CALL(m_applicationManagementServerMock, initialize(appManagementSocket)).WillOnce(Return(true));
    EXPECT_CALL(m_applicationManagementServerMock, start());
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::UNINITIALIZED))
        .WillOnce(Return(true));
    EXPECT_TRUE(m_sut);
    std::string arg1{"AppName"};
    std::string arg2{std::to_string(appManagementSocket)};
    char *args[]{&arg1[0], &arg2[0]};
    EXPECT_TRUE(m_sut->initialize(2, args));
}

void SessionServerManagerTests::willFailToSetConfigurationWhenSessionManagementServerFailsToInit()
{
    EXPECT_CALL(m_sessionManagementServerMock, initialize(sessionManagementSocket)).WillOnce(Return(false));
    EXPECT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->setConfiguration(sessionManagementSocket, SessionServerState::INACTIVE, maxPlaybacks));
}

void SessionServerManagerTests::willFailToSetConfigurationWhenSessionManagementServerFailsToSetInitialState()
{
    EXPECT_CALL(m_sessionManagementServerMock, initialize(sessionManagementSocket)).WillOnce(Return(true));
    EXPECT_CALL(m_sessionManagementServerMock, start());
    EXPECT_CALL(m_playbackServiceMock, setMaxPlaybacks(maxPlaybacks));
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::INACTIVE))
        .WillOnce(Return(false));
    EXPECT_CALL(m_playbackServiceMock, switchToActive()).WillOnce(Return(false));
    EXPECT_CALL(m_cdmServiceMock, switchToActive()).WillOnce(Return(false));
    EXPECT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->setConfiguration(sessionManagementSocket, SessionServerState::INACTIVE, maxPlaybacks));
}

void SessionServerManagerTests::willSetConfiguration()
{
    EXPECT_CALL(m_sessionManagementServerMock, initialize(sessionManagementSocket)).WillOnce(Return(true));
    EXPECT_CALL(m_sessionManagementServerMock, start());
    EXPECT_CALL(m_playbackServiceMock, setMaxPlaybacks(maxPlaybacks));
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::INACTIVE))
        .WillOnce(Return(true));
    EXPECT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->setConfiguration(sessionManagementSocket, SessionServerState::INACTIVE, maxPlaybacks));
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
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::ACTIVE)).WillOnce(Return(true));
}

void SessionServerManagerTests::willFailToSetStateInactive()
{
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_playbackServiceMock, switchToActive()).WillOnce(Return(true));
    EXPECT_CALL(m_cdmServiceMock, switchToActive()).WillOnce(Return(true));
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::INACTIVE))
        .WillOnce(Return(false));
}

void SessionServerManagerTests::willSetStateInactive()
{
    EXPECT_CALL(m_playbackServiceMock, switchToInactive());
    EXPECT_CALL(m_cdmServiceMock, switchToInactive());
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::INACTIVE))
        .WillOnce(Return(true));
}

void SessionServerManagerTests::willFailToSetStateNotRunning()
{
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::NOT_RUNNING))
        .WillOnce(Return(false));
}

void SessionServerManagerTests::willSetStateNotRunning()
{
    EXPECT_CALL(m_applicationManagementServerMock, sendStateChangedEvent(SessionServerState::NOT_RUNNING))
        .WillOnce(Return(true));
}

void SessionServerManagerTests::willSetLogLevels()
{
    EXPECT_CALL(m_sessionManagementServerMock, setLogLevels(logLvl, logLvl, logLvl, logLvl));
}

void SessionServerManagerTests::setStateShouldFail(const firebolt::rialto::server::SessionServerState &state)
{
    EXPECT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->setState(state));
}

void SessionServerManagerTests::setStateShouldSucceed(const firebolt::rialto::server::SessionServerState &state)
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
    m_sut->setLogLevels(logLvl, logLvl, logLvl, logLvl, logLvl, logLvl);
}
