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

#include <string>
#include <utility>

#include "LoggingLevels.h"
#include "MatchersServerManager.h"
#include "SessionServerAppManager.h"
#include "SessionServerAppManagerTestsFixture.h"

using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;
using testing::StrictMock;

namespace rialto::servermanager::service
{
bool operator==(const LoggingLevels &lhs, const LoggingLevels &rhs)
{
    return lhs.defaultLoggingLevel == rhs.defaultLoggingLevel && lhs.clientLoggingLevel == rhs.clientLoggingLevel &&
           lhs.sessionServerLoggingLevel == rhs.sessionServerLoggingLevel &&
           lhs.ipcLoggingLevel == rhs.ipcLoggingLevel && lhs.serverManagerLoggingLevel == rhs.serverManagerLoggingLevel &&
           lhs.commonLoggingLevel == rhs.commonLoggingLevel;
}
} // namespace rialto::servermanager::service

namespace
{
const std::string kAppName{"YouTube"};
const std::string kEmptyAppName{""};
constexpr int kServerId{3};
constexpr unsigned kNumOfPreloadedServers{1};
const int kAppMgmtSocket{0};
const std::string kSessionServerSocketName{getenv("RIALTO_SOCKET_PATH")};
constexpr int kMaxSessions{2};
constexpr int kMaxWebAudioPlayers{3};
constexpr rialto::servermanager::service::LoggingLevels
    kExampleLoggingLevels{rialto::servermanager::service::LoggingLevel::FATAL,
                          rialto::servermanager::service::LoggingLevel::ERROR,
                          rialto::servermanager::service::LoggingLevel::WARNING,
                          rialto::servermanager::service::LoggingLevel::MILESTONE,
                          rialto::servermanager::service::LoggingLevel::INFO};
const firebolt::rialto::common::AppConfig kAppConfig{kSessionServerSocketName};
constexpr int kPingId{125};
const std::string kClientDisplayName{"westeros-rialto"};
constexpr unsigned int kSocketPermissions{0777};
const std::string kSocketOwner;
const std::string kSocketGroup;

} // namespace

MATCHER_P2(MaxResourceMatcher, maxPlaybacks, maxWebAudioPlayers, "")
{
    return ((maxPlaybacks == arg.maxPlaybacks) && (maxWebAudioPlayers == arg.maxWebAudioPlayers));
}

SessionServerAppManagerTests::SessionServerAppManagerTests()
    : m_controller{std::make_unique<StrictMock<rialto::servermanager::ipc::ControllerMock>>()},
      m_stateObserver{std::make_shared<StrictMock<rialto::servermanager::service::StateObserverMock>>()},
      m_sessionServerApp{std::make_unique<StrictMock<rialto::servermanager::common::SessionServerAppMock>>()},
      m_sessionServerAppFactory{
          std::make_unique<StrictMock<rialto::servermanager::common::SessionServerAppFactoryMock>>()},
      m_healthcheckServiceFactory{
          std::make_unique<StrictMock<rialto::servermanager::common::HealthcheckServiceFactoryMock>>()},
      m_healthcheckService{std::make_unique<StrictMock<rialto::servermanager::common::HealthcheckServiceMock>>()},
      m_controllerMock{dynamic_cast<StrictMock<rialto::servermanager::ipc::ControllerMock> &>(*m_controller)},
      m_sessionServerAppMock{
          dynamic_cast<StrictMock<rialto::servermanager::common::SessionServerAppMock> &>(*m_sessionServerApp)},
      m_sessionServerAppFactoryMock{dynamic_cast<StrictMock<rialto::servermanager::common::SessionServerAppFactoryMock> &>(
          *m_sessionServerAppFactory)},
      m_healthcheckServiceFactoryMock{
          dynamic_cast<StrictMock<rialto::servermanager::common::HealthcheckServiceFactoryMock> &>(
              *m_healthcheckServiceFactory)},
      m_healthcheckServiceMock{
          dynamic_cast<StrictMock<rialto::servermanager::common::HealthcheckServiceMock> &>(*m_healthcheckService)}
{
    auto eventThreadFactoryMock = std::make_shared<StrictMock<firebolt::rialto::common::EventThreadFactoryMock>>();
    auto eventThreadMock = std::make_unique<StrictMock<firebolt::rialto::common::EventThreadMock>>();
    EXPECT_CALL(*eventThreadMock, addImpl(_)).WillRepeatedly(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*eventThreadMock, flush());
    EXPECT_CALL(*eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(eventThreadMock))));
    EXPECT_CALL(m_healthcheckServiceFactoryMock, createHealthcheckService(_))
        .WillOnce(Return(ByMove(std::move(m_healthcheckService))));

    m_sut =
        std::make_unique<rialto::servermanager::common::SessionServerAppManager>(m_controller, m_stateObserver,
                                                                                 std::move(m_sessionServerAppFactory),
                                                                                 std::move(m_healthcheckServiceFactory),
                                                                                 eventThreadFactoryMock);
}

void SessionServerAppManagerTests::sessionServerLaunchWillFail(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerAppFactoryMock, create(kAppName, state, kAppConfig, _))
        .WillOnce(Return(ByMove(std::move(m_sessionServerApp))));
    EXPECT_CALL(m_sessionServerAppMock, launch()).WillOnce(Return(false));
}

void SessionServerAppManagerTests::preloadedSessionServerLaunchWillFail()
{
    EXPECT_CALL(m_sessionServerAppFactoryMock, create(_)).WillOnce(Return(ByMove(std::move(m_sessionServerApp))));
    EXPECT_CALL(m_sessionServerAppMock, launch()).WillOnce(Return(false));
}

void SessionServerAppManagerTests::sessionServerConnectWillFail(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerAppFactoryMock, create(kAppName, state, kAppConfig, _))
        .WillOnce(Return(ByMove(std::move(m_sessionServerApp))));
    EXPECT_CALL(m_sessionServerAppMock, launch()).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerAppMock, getAppName()).WillRepeatedly(ReturnRef(kAppName));
    EXPECT_CALL(m_sessionServerAppMock, getAppManagementSocketName()).WillOnce(Return(kAppMgmtSocket));
    EXPECT_CALL(m_sessionServerAppMock, getServerId()).WillRepeatedly(Return(kServerId));
    EXPECT_CALL(m_controllerMock, createClient(kServerId, kAppMgmtSocket)).WillOnce(Return(false));
    EXPECT_CALL(m_sessionServerAppMock, kill());
    EXPECT_CALL(m_healthcheckServiceMock, onServerRemoved(kServerId));
}

void SessionServerAppManagerTests::preloadedSessionServerConnectWillFail()
{
    EXPECT_CALL(m_sessionServerAppFactoryMock, create(_)).WillOnce(Return(ByMove(std::move(m_sessionServerApp))));
    EXPECT_CALL(m_sessionServerAppMock, launch()).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerAppMock, getServerId()).WillRepeatedly(Return(kServerId));
    EXPECT_CALL(m_sessionServerAppMock, getAppManagementSocketName()).WillOnce(Return(kAppMgmtSocket));
    EXPECT_CALL(m_controllerMock, createClient(kServerId, kAppMgmtSocket)).WillOnce(Return(false));
    EXPECT_CALL(m_sessionServerAppMock, kill());
    EXPECT_CALL(m_healthcheckServiceMock, onServerRemoved(kServerId));
}

void SessionServerAppManagerTests::sessionServerChangeStateWillFail(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerAppMock, getServerId()).WillRepeatedly(Return(kServerId));
    EXPECT_CALL(m_sessionServerAppMock, setExpectedState(state));
    EXPECT_CALL(m_controllerMock, performSetState(kServerId, state)).WillOnce(Return(false));
}

void SessionServerAppManagerTests::sessionServerWillLaunch(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerAppFactoryMock, create(kAppName, state, kAppConfig, _))
        .WillOnce(Return(ByMove(std::move(m_sessionServerApp))));
    EXPECT_CALL(m_sessionServerAppMock, launch()).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerAppMock, getAppName()).WillRepeatedly(ReturnRef(kAppName));
    EXPECT_CALL(m_sessionServerAppMock, getAppManagementSocketName()).WillOnce(Return(kAppMgmtSocket));
    EXPECT_CALL(m_sessionServerAppMock, getServerId()).WillRepeatedly(Return(kServerId));
    EXPECT_CALL(m_controllerMock, createClient(kServerId, kAppMgmtSocket)).WillOnce(Return(true));
}

void SessionServerAppManagerTests::preloadedSessionServerWillLaunch()
{
    EXPECT_CALL(m_sessionServerAppFactoryMock, create(_)).WillOnce(Return(ByMove(std::move(m_sessionServerApp))));
    EXPECT_CALL(m_sessionServerAppMock, launch()).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerAppMock, getServerId()).WillRepeatedly(Return(kServerId));
    EXPECT_CALL(m_sessionServerAppMock, getAppManagementSocketName()).WillOnce(Return(kAppMgmtSocket));
    EXPECT_CALL(m_controllerMock, createClient(kServerId, kAppMgmtSocket)).WillOnce(Return(true));
}

void SessionServerAppManagerTests::preloadedSessionServerWillFailToConfigure(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerAppMock, configure(kAppName, state, kAppConfig)).WillOnce(Return(false));
}

void SessionServerAppManagerTests::preloadedSessionServerWillBeConfigured(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerAppMock, configure(kAppName, state, kAppConfig)).WillOnce(Return(true));
}

void SessionServerAppManagerTests::preloadedSessionServerWillCloseWithError()
{
    m_secondSessionServerApp = std::make_unique<StrictMock<rialto::servermanager::common::SessionServerAppMock>>();
    auto &secondSessionServerAppMock{
        dynamic_cast<StrictMock<rialto::servermanager::common::SessionServerAppMock> &>(*m_secondSessionServerApp)};
    EXPECT_CALL(m_sessionServerAppMock, isPreloaded()).WillOnce(Return(true)).WillOnce(Return(false));
    EXPECT_CALL(m_sessionServerAppMock, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerAppMock, getAppName()).WillRepeatedly(ReturnRef(kEmptyAppName));
    EXPECT_CALL(m_controllerMock, removeClient(kServerId));
    EXPECT_CALL(m_sessionServerAppMock, kill());
    EXPECT_CALL(m_healthcheckServiceMock, onServerRemoved(kServerId));
    EXPECT_CALL(m_sessionServerAppFactoryMock, create(_)).WillOnce(Return(ByMove(std::move(m_secondSessionServerApp))));
    EXPECT_CALL(secondSessionServerAppMock, launch()).WillOnce(Return(false));
}

void SessionServerAppManagerTests::sessionServerWillChangeState(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerAppMock, setExpectedState(state));
    EXPECT_CALL(m_controllerMock, performSetState(kServerId, state)).WillOnce(Return(true));
}

void SessionServerAppManagerTests::sessionServerWillChangeStateToUninitialized()
{
    EXPECT_CALL(m_sessionServerAppMock, cancelStartupTimer());
    EXPECT_CALL(m_sessionServerAppMock, getInitialState())
        .WillOnce(Return(firebolt::rialto::common::SessionServerState::INACTIVE));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketName()).WillOnce(Return(kSessionServerSocketName));
    EXPECT_CALL(m_sessionServerAppMock, getClientDisplayName()).WillOnce(Return(kClientDisplayName));
    EXPECT_CALL(m_sessionServerAppMock, getAppName()).WillRepeatedly(ReturnRef(kAppName));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketPermissions()).WillOnce(Return(kSocketPermissions));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketOwner()).WillOnce(Return(kSocketOwner));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketGroup()).WillOnce(Return(kSocketGroup));
    EXPECT_CALL(m_sessionServerAppMock, getMaxPlaybackSessions()).WillOnce(Return(kMaxSessions));
    EXPECT_CALL(m_sessionServerAppMock, getMaxWebAudioPlayers()).WillOnce(Return(kMaxWebAudioPlayers));
    EXPECT_CALL(m_sessionServerAppMock, isPreloaded()).WillOnce(Return(false));
    EXPECT_CALL(m_controllerMock,
                performSetConfiguration(kServerId, firebolt::rialto::common::SessionServerState::INACTIVE,
                                        kSessionServerSocketName, kClientDisplayName,
                                        MaxResourceMatcher(kMaxSessions, kMaxWebAudioPlayers), kSocketPermissions,
                                        kSocketOwner, kSocketGroup, kAppName))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_stateObserver, stateChanged(kAppName, firebolt::rialto::common::SessionServerState::UNINITIALIZED));
}

void SessionServerAppManagerTests::preloadedSessionServerWillChangeStateToUninitialized()
{
    EXPECT_CALL(m_sessionServerAppMock, getAppName()).WillOnce(ReturnRef(kEmptyAppName));
    EXPECT_CALL(m_sessionServerAppMock, cancelStartupTimer());
    EXPECT_CALL(m_sessionServerAppMock, isPreloaded()).WillOnce(Return(true));
}

void SessionServerAppManagerTests::sessionServerWillChangeStateToInactive()
{
    EXPECT_CALL(*m_stateObserver, stateChanged(kAppName, firebolt::rialto::common::SessionServerState::INACTIVE));
}

void SessionServerAppManagerTests::preloadedSessionServerWillSetConfiguration()
{
    m_secondSessionServerApp = std::make_unique<StrictMock<rialto::servermanager::common::SessionServerAppMock>>();
    auto &secondSessionServerAppMock{
        dynamic_cast<StrictMock<rialto::servermanager::common::SessionServerAppMock> &>(*m_secondSessionServerApp)};
    EXPECT_CALL(m_sessionServerAppMock, isPreloaded()).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerAppMock, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerAppMock, getAppName()).WillRepeatedly(ReturnRef(kEmptyAppName));
    EXPECT_CALL(m_sessionServerAppMock, getInitialState())
        .WillOnce(Return(firebolt::rialto::common::SessionServerState::INACTIVE));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketName()).WillOnce(Return(kSessionServerSocketName));
    EXPECT_CALL(m_sessionServerAppMock, getClientDisplayName()).WillOnce(Return(kClientDisplayName));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketPermissions()).WillOnce(Return(kSocketPermissions));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketOwner()).WillOnce(Return(kSocketOwner));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketGroup()).WillOnce(Return(kSocketGroup));
    EXPECT_CALL(m_sessionServerAppMock, getMaxPlaybackSessions()).WillOnce(Return(kMaxSessions));
    EXPECT_CALL(m_sessionServerAppMock, getMaxWebAudioPlayers()).WillOnce(Return(kMaxWebAudioPlayers));
    EXPECT_CALL(m_controllerMock,
                performSetConfiguration(kServerId, firebolt::rialto::common::SessionServerState::INACTIVE,
                                        kSessionServerSocketName, kClientDisplayName,
                                        MaxResourceMatcher(kMaxSessions, kMaxWebAudioPlayers), kSocketPermissions,
                                        kSocketOwner, kSocketGroup, kEmptyAppName))
        .WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerAppFactoryMock, create(_)).WillOnce(Return(ByMove(std::move(m_secondSessionServerApp))));
    EXPECT_CALL(secondSessionServerAppMock, launch()).WillOnce(Return(false));
}

void SessionServerAppManagerTests::sessionServerWillFailToSetConfiguration()
{
    EXPECT_CALL(m_sessionServerAppMock, cancelStartupTimer());
    EXPECT_CALL(m_sessionServerAppMock, getInitialState())
        .WillOnce(Return(firebolt::rialto::common::SessionServerState::INACTIVE));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketName()).WillOnce(Return(kSessionServerSocketName));
    EXPECT_CALL(m_sessionServerAppMock, getClientDisplayName()).WillOnce(Return(kClientDisplayName));
    EXPECT_CALL(m_sessionServerAppMock, getAppName()).WillOnce(ReturnRef(kAppName));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketPermissions()).WillOnce(Return(kSocketPermissions));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketOwner()).WillOnce(Return(kSocketOwner));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketGroup()).WillOnce(Return(kSocketGroup));
    EXPECT_CALL(m_sessionServerAppMock, getMaxPlaybackSessions()).WillOnce(Return(kMaxSessions));
    EXPECT_CALL(m_sessionServerAppMock, getMaxWebAudioPlayers()).WillOnce(Return(kMaxWebAudioPlayers));
    EXPECT_CALL(m_sessionServerAppMock, isPreloaded()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_controllerMock,
                performSetConfiguration(kServerId, firebolt::rialto::common::SessionServerState::INACTIVE,
                                        kSessionServerSocketName, kClientDisplayName,
                                        MaxResourceMatcher(kMaxSessions, kMaxWebAudioPlayers), kSocketPermissions,
                                        kSocketOwner, kSocketGroup, kAppName))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_stateObserver, stateChanged(kAppName, firebolt::rialto::common::SessionServerState::UNINITIALIZED));
}

void SessionServerAppManagerTests::preloadedSessionServerWillFailToSetConfiguration()
{
    EXPECT_CALL(m_sessionServerAppMock, getInitialState())
        .WillOnce(Return(firebolt::rialto::common::SessionServerState::INACTIVE));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketName()).WillOnce(Return(kSessionServerSocketName));
    EXPECT_CALL(m_sessionServerAppMock, getClientDisplayName()).WillOnce(Return(kClientDisplayName));
    EXPECT_CALL(m_sessionServerAppMock, getAppName()).WillOnce(ReturnRef(kAppName));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketPermissions()).WillOnce(Return(kSocketPermissions));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketOwner()).WillOnce(Return(kSocketOwner));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketGroup()).WillOnce(Return(kSocketGroup));
    EXPECT_CALL(m_sessionServerAppMock, getMaxPlaybackSessions()).WillOnce(Return(kMaxSessions));
    EXPECT_CALL(m_sessionServerAppMock, getMaxWebAudioPlayers()).WillOnce(Return(kMaxWebAudioPlayers));
    EXPECT_CALL(m_controllerMock,
                performSetConfiguration(kServerId, firebolt::rialto::common::SessionServerState::INACTIVE,
                                        kSessionServerSocketName, kClientDisplayName,
                                        MaxResourceMatcher(kMaxSessions, kMaxWebAudioPlayers), kSocketPermissions,
                                        kSocketOwner, kSocketGroup, kAppName))
        .WillOnce(Return(false));
}

void SessionServerAppManagerTests::sessionServerWillSetLogLevels()
{
    EXPECT_CALL(m_controllerMock, setLogLevels(kExampleLoggingLevels)).WillOnce(Return(true));
}

void SessionServerAppManagerTests::sessionServerWillFailToSetLogLevels()
{
    EXPECT_CALL(m_controllerMock, setLogLevels(kExampleLoggingLevels)).WillOnce(Return(false));
}

void SessionServerAppManagerTests::clientWillBeRemoved()
{
    EXPECT_CALL(m_healthcheckServiceMock, onServerRemoved(kServerId));
    EXPECT_CALL(m_controllerMock, removeClient(kServerId));
}

void SessionServerAppManagerTests::sessionServerWillIndicateStateChange(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(*m_stateObserver, stateChanged(kAppName, state));
}

void SessionServerAppManagerTests::sessionServerWillKillRunningApplication()
{
    EXPECT_CALL(m_sessionServerAppMock, kill());
}

void SessionServerAppManagerTests::sessionServerWontBePreloaded()
{
    EXPECT_CALL(m_sessionServerAppMock, isPreloaded()).WillOnce(Return(false));
}

void SessionServerAppManagerTests::healthcheckServiceWillHandleAck(bool success)
{
    EXPECT_CALL(m_healthcheckServiceMock, onAckReceived(kServerId, kPingId, success));
}

void SessionServerAppManagerTests::pingWillBeSentToRunningApps()
{
    EXPECT_CALL(m_controllerMock, performPing(kServerId, kPingId)).WillOnce(Return(true));
    EXPECT_CALL(m_healthcheckServiceMock, onPingSent(kServerId, kPingId));
}

void SessionServerAppManagerTests::pingSendToRunningAppsWillFail()
{
    EXPECT_CALL(m_controllerMock, performPing(kServerId, kPingId)).WillOnce(Return(false));
    EXPECT_CALL(m_healthcheckServiceMock, onPingFailed(kServerId, kPingId));
}

void SessionServerAppManagerTests::newSessionServerWillBeLaunched()
{
    m_secondSessionServerApp = std::make_unique<StrictMock<rialto::servermanager::common::SessionServerAppMock>>();
    auto &secondSessionServerAppMock{
        dynamic_cast<StrictMock<rialto::servermanager::common::SessionServerAppMock> &>(*m_secondSessionServerApp)};
    EXPECT_CALL(m_sessionServerAppMock, isPreloaded()).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerAppFactoryMock, create(_)).WillOnce(Return(ByMove(std::move(m_secondSessionServerApp))));
    EXPECT_CALL(secondSessionServerAppMock, launch()).WillOnce(Return(false));
}

void SessionServerAppManagerTests::sessionServerWillReturnAppSocketName(const std::string &socketName)
{
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketName()).WillOnce(Return(socketName));
}

void SessionServerAppManagerTests::sessionServerWillBeRestarted(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerAppMock, getExpectedState()).WillOnce(Return(state));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketName()).WillOnce(Return(kSessionServerSocketName));
    EXPECT_CALL(m_sessionServerAppMock, getClientDisplayName()).WillOnce(Return(kClientDisplayName));
    sessionServerWillKillRunningApplication();
    sessionServerWillIndicateStateChange(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    clientWillBeRemoved();
    auto restartedSessionServer{std::make_unique<StrictMock<rialto::servermanager::common::SessionServerAppMock>>()};
    EXPECT_CALL(*restartedSessionServer, launch()).WillOnce(Return(true));
    EXPECT_CALL(*restartedSessionServer, getAppName()).WillRepeatedly(ReturnRef(kAppName));
    EXPECT_CALL(*restartedSessionServer, getAppManagementSocketName()).WillOnce(Return(kAppMgmtSocket));
    EXPECT_CALL(*restartedSessionServer, getServerId()).WillRepeatedly(Return(kServerId));
    EXPECT_CALL(*restartedSessionServer, kill());
    EXPECT_CALL(m_sessionServerAppFactoryMock, create(kAppName, state, kAppConfig, _))
        .WillOnce(Return(ByMove(std::move(restartedSessionServer))));
    EXPECT_CALL(m_controllerMock, createClient(kServerId, kAppMgmtSocket)).WillOnce(Return(true));
}

void SessionServerAppManagerTests::sessionServerWillRestartWillBeSkipped()
{
    EXPECT_CALL(m_sessionServerAppMock, getExpectedState())
        .WillOnce(Return(firebolt::rialto::common::SessionServerState::UNINITIALIZED));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketName()).WillOnce(Return(kSessionServerSocketName));
    EXPECT_CALL(m_sessionServerAppMock, getClientDisplayName()).WillOnce(Return(kClientDisplayName));
}

void SessionServerAppManagerTests::triggerPreloadSessionServers()
{
    EXPECT_TRUE(m_sut);
    return m_sut->preloadSessionServers(kNumOfPreloadedServers);
}

bool SessionServerAppManagerTests::triggerInitiateApplication(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_TRUE(m_sut);
    return m_sut->initiateApplication(kAppName, state, kAppConfig);
}

bool SessionServerAppManagerTests::triggerSetSessionServerState(const firebolt::rialto::common::SessionServerState &newState)
{
    EXPECT_TRUE(m_sut);
    return m_sut->setSessionServerState(kAppName, newState);
}

void SessionServerAppManagerTests::triggerOnSessionServerStateChanged(
    const firebolt::rialto::common::SessionServerState &newState)
{
    EXPECT_TRUE(m_sut);
    return m_sut->onSessionServerStateChanged(kServerId, newState);
}

void SessionServerAppManagerTests::triggerOnAck(bool success)
{
    EXPECT_TRUE(m_sut);
    return m_sut->onAck(kServerId, kPingId, success);
}

std::string SessionServerAppManagerTests::triggerGetAppConnectionInfo()
{
    EXPECT_TRUE(m_sut);
    return m_sut->getAppConnectionInfo(kAppName);
}

bool SessionServerAppManagerTests::triggerSetLogLevel()
{
    EXPECT_TRUE(m_sut);
    return m_sut->setLogLevels(kExampleLoggingLevels);
}

void SessionServerAppManagerTests::triggerSendPingEvents()
{
    EXPECT_TRUE(m_sut);
    m_sut->sendPingEvents(kPingId);
}

void SessionServerAppManagerTests::triggerRestartServer()
{
    m_sut->restartServer(kServerId);
}
