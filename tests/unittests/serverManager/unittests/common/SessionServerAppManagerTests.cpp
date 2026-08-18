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

#include "SessionServerAppManagerTestsFixture.h"
#include "ControllerMock.h"
#include "EventThreadFactoryMock.h"
#include "EventThreadMock.h"
#include "HealthcheckServiceFactoryMock.h"
#include "HealthcheckServiceMock.h"
#include "NamedSocketMock.h"
#include "SessionServerAppFactoryMock.h"
#include "SessionServerAppMock.h"
#include "StateObserverMock.h"
#include "MediaCapabilitiesMock.h"
#include "MatchersServerManager.h"
#include "SessionServerAppManager.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;
using testing::ByMove;
using testing::Invoke;
using testing::Return;
using testing::Eq;
using testing::_;
using rialto::servermanager::service::MediaCapabilitiesMock;

TEST_F(SessionServerAppManagerTests, GetConnectionInfoShouldReturnEmptyStringForNotRunningSessionServer)
{
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, InitiateApplicationShouldReturnFalseWhenNotRunningSessionServerIsSwitchedToNotRunning)
{
    ASSERT_FALSE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::NOT_RUNNING));
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, InitiateApplicationShouldReturnFalseWhenSessionServerFailedToLaunch)
{
    sessionServerLaunchWillFail(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_FALSE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, InitiateApplicationShouldReturnFalseWhenSessionServerFailedToConnect)
{
    sessionServerConnectWillFail(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_FALSE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, InitiateApplicationShouldReturnTrueWhenSessionServerIsLaunched)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, InitiateApplicationShouldReturnFalseWhenCalledForRunningApplication)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    ASSERT_FALSE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, GetConnectionInfoShouldReturnProperSocket)
{
    const std::string kAppSocket{getenv("RIALTO_SOCKET_PATH")};
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillReturnAppSocketName(kAppSocket);
    ASSERT_EQ(kAppSocket, triggerGetAppConnectionInfo());
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SetSessionServerStateShouldReturnFalseWhenAppIsNotLaunched)
{
    ASSERT_FALSE(triggerSetSessionServerState(firebolt::rialto::common::SessionServerState::ACTIVE));
}

TEST_F(SessionServerAppManagerTests, SetSessionServerStateShouldReturnFalseWhenUnableToSendMessage)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerChangeStateWillFail(firebolt::rialto::common::SessionServerState::ACTIVE);
    sessionServerWontBePreloaded();
    sessionServerWillIndicateStateChange(firebolt::rialto::common::SessionServerState::ERROR);
    ASSERT_FALSE(triggerSetSessionServerState(firebolt::rialto::common::SessionServerState::ACTIVE));
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SetSessionServerStateToNotRunningShouldReturnFalseAndKillAppWhenUnableToSendMessage)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerChangeStateWillFail(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    sessionServerWillKillRunningApplication();
    sessionServerWillIndicateStateChange(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    clientWillBeRemoved();
    ASSERT_FALSE(triggerSetSessionServerState(firebolt::rialto::common::SessionServerState::NOT_RUNNING));
}

TEST_F(SessionServerAppManagerTests, SetSessionServerStateShouldReturnTrueWhenStateIsChanged)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillChangeState(firebolt::rialto::common::SessionServerState::ACTIVE);
    ASSERT_TRUE(triggerSetSessionServerState(firebolt::rialto::common::SessionServerState::ACTIVE));
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, StateObserverShouldBeInformedAboutStateChangeToInactive)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillChangeStateToUninitialized();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
    sessionServerWillChangeStateToInactive();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::INACTIVE);
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldRemoveApplicationWhenSetConfigurationFails)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillFailToSetConfiguration();
    sessionServerWillIndicateStateChange(firebolt::rialto::common::SessionServerState::ERROR);
    clientWillBeRemoved();
    sessionServerWillIndicateStateChange(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    sessionServerWillKillRunningApplication();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldRemoveApplicationWhenNotRunningIndicationIsReceived)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    clientWillBeRemoved();
    sessionServerWillIndicateStateChange(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldForwardErrorIndicationOfRunningApp)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillIndicateStateChange(firebolt::rialto::common::SessionServerState::ERROR);
    sessionServerWontBePreloaded();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::ERROR);
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldRespawnPreloadedServerWhenErrorIndicationIsReceived)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillIndicateStateChange(firebolt::rialto::common::SessionServerState::ERROR);
    clientWillBeRemoved();
    sessionServerWillKillRunningApplication();
    newSessionServerWillBeLaunched();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::ERROR);
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldSetNewLogLevel)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    sessionServerWillSetLogLevels();
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    ASSERT_TRUE(triggerSetLogLevel());
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldFailToSetNewLogLevel)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    sessionServerWillFailToSetLogLevels();
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    ASSERT_FALSE(triggerSetLogLevel());
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, PreloadedServerShouldFailToLaunch)
{
    preloadedSessionServerLaunchWillFail();
    triggerPreloadSessionServers();
}

TEST_F(SessionServerAppManagerTests, PreloadedServerShouldFailToConnect)
{
    preloadedSessionServerConnectWillFail();
    triggerPreloadSessionServers();
}

TEST_F(SessionServerAppManagerTests, PreloadedServerShouldLaunch)
{
    preloadedSessionServerWillLaunch();
    triggerPreloadSessionServers();
    preloadedSessionServerWillChangeStateToUninitialized();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldFailToConfigurePreloadedAppDueToAppError)
{
    preloadedSessionServerWillLaunch();
    triggerPreloadSessionServers();
    preloadedSessionServerWillChangeStateToUninitialized();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
    preloadedSessionServerWillFailToConfigure(firebolt::rialto::common::SessionServerState::INACTIVE);
    preloadedSessionServerWillCloseWithError();
    triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE);
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldFailToConfigurePreloadedAppDueToServerError)
{
    preloadedSessionServerWillLaunch();
    triggerPreloadSessionServers();
    preloadedSessionServerWillChangeStateToUninitialized();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
    preloadedSessionServerWillBeConfigured(firebolt::rialto::common::SessionServerState::INACTIVE);
    preloadedSessionServerWillFailToSetConfiguration();
    preloadedSessionServerWillCloseWithError();
    triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE);
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldConfigure)
{
    preloadedSessionServerWillLaunch();
    triggerPreloadSessionServers();
    preloadedSessionServerWillChangeStateToUninitialized();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
    preloadedSessionServerWillBeConfigured(firebolt::rialto::common::SessionServerState::INACTIVE);
    preloadedSessionServerWillSetConfiguration();
    triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE);
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldConfigureWithSocketFd)
{
    preloadedSessionServerWillLaunch();
    triggerPreloadSessionServers();
    preloadedSessionServerWillChangeStateToUninitialized();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
    preloadedSessionServerWillBeConfigured(firebolt::rialto::common::SessionServerState::INACTIVE);
    preloadedSessionServerWillSetConfigurationWithFd();
    triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE);
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldRemoveApplicationWhenSetConfigurationWithFdFails)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillFailToSetConfigurationWithFd();
    sessionServerWillIndicateStateChange(firebolt::rialto::common::SessionServerState::ERROR);
    clientWillBeRemoved();
    sessionServerWillIndicateStateChange(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    sessionServerWillKillRunningApplication();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldHandleAck)
{
    constexpr bool kSuccess{true};
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    healthcheckServiceWillHandleAck(kSuccess);
    triggerOnAck(kSuccess);
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldHandleFailedAck)
{
    constexpr bool kSuccess{false};
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    healthcheckServiceWillHandleAck(kSuccess);
    triggerOnAck(kSuccess);
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldSendPingEvents)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    pingWillBeSentToRunningApps();
    triggerSendPingEvents();
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldFailToSendPingEvents)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    pingSendToRunningAppsWillFail();
    triggerSendPingEvents();
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SessionServerShouldRestart)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillChangeStateToUninitialized();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
    sessionServerWillChangeStateToInactive();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::INACTIVE);
    sessionServerWillBeRestarted(firebolt::rialto::common::SessionServerState::INACTIVE);
    triggerRestartServer();
}

TEST_F(SessionServerAppManagerTests, SessionServerShouldSkipRestart)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillChangeStateToUninitialized();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
    sessionServerWillRestartWillBeSkipped();
    triggerRestartServer();
    sessionServerWillKillRunningApplication();
}

TEST_F(SessionServerAppManagerTests, SessionServerShouldReportStartupTimeout)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillHandleServerStartupTimeout();
    triggerOnServerStartupTimeout();
}

TEST_F(SessionServerAppManagerTests, SessionServerShouldSkipReportingStartupTimeoutWhenServerDoesNotExist)
{
    triggerOnServerStartupTimeout();
}

TEST_F(SessionServerAppManagerTests, MediaCapabilitiesOptionalsShouldBePopulatedAndForwardedWhenCapabilitiesAreOk)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillKillRunningApplication();
}

// Separate test fixture for testing media capabilities error cases
class SessionServerAppManagerMediaCapabilitiesErrorTests : public testing::Test
{
public:
    SessionServerAppManagerMediaCapabilitiesErrorTests();
    virtual ~SessionServerAppManagerMediaCapabilitiesErrorTests() = default;

    void sessionServerWillLaunchWithoutCapabilities(const firebolt::rialto::common::SessionServerState &state);

protected:
    std::unique_ptr<rialto::servermanager::ipc::IController> m_controller;
    std::shared_ptr<StrictMock<rialto::servermanager::service::StateObserverMock>> m_stateObserver;
    std::shared_ptr<StrictMock<rialto::servermanager::common::SessionServerAppMock>> m_sessionServerAppMock;
    std::unique_ptr<rialto::servermanager::common::ISessionServerAppFactory> m_sessionServerAppFactory;
    std::unique_ptr<rialto::servermanager::common::IHealthcheckServiceFactory> m_healthcheckServiceFactory;
    std::unique_ptr<rialto::servermanager::common::IHealthcheckService> m_healthcheckService;
    StrictMock<rialto::servermanager::ipc::ControllerMock> &m_controllerMock;
    StrictMock<rialto::servermanager::common::SessionServerAppFactoryMock> &m_sessionServerAppFactoryMock;
    StrictMock<rialto::servermanager::common::HealthcheckServiceFactoryMock> &m_healthcheckServiceFactoryMock;
    StrictMock<rialto::servermanager::common::HealthcheckServiceMock> &m_healthcheckServiceMock;
    StrictMock<firebolt::rialto::ipc::NamedSocketFactoryMock> m_namedSocketFactoryMock;
    std::unique_ptr<firebolt::rialto::ipc::INamedSocket> m_namedSocket{
        std::make_unique<testing::StrictMock<firebolt::rialto::ipc::NamedSocketMock>>()};
    testing::StrictMock<firebolt::rialto::ipc::NamedSocketMock> &m_namedSocketMock{
        dynamic_cast<testing::StrictMock<firebolt::rialto::ipc::NamedSocketMock> &>(*m_namedSocket)};
    std::unique_ptr<rialto::servermanager::common::ISessionServerAppManager> m_sut;
    std::shared_ptr<testing::StrictMock<rialto::servermanager::service::MediaCapabilitiesMock>> m_mediaCapabilities;
    testing::StrictMock<rialto::servermanager::service::MediaCapabilitiesMock> &m_mediaCapabilitiesMock;
};

namespace
{
const std::string kAppName{"YouTube"};
const std::string kEmptyAppName{""};
constexpr int kServerId{3};
const std::string kSessionServerSocketName{getenv("RIALTO_SOCKET_PATH") ? getenv("RIALTO_SOCKET_PATH") : ""};
constexpr int kMaxSessions{2};
constexpr int kMaxWebAudioPlayers{3};
const firebolt::rialto::common::AppConfig kAppConfig{kSessionServerSocketName};
const std::string kClientDisplayName{"westeros-rialto"};
constexpr unsigned int kSocketPermissions{0777};
const std::string kSocketOwner;
const std::string kSocketGroup;

MATCHER_P2(MaxResourceMatcherError, maxPlaybacks, maxWebAudioPlayers, "")
{
    return ((maxPlaybacks == arg.maxPlaybacks) && (maxWebAudioPlayers == arg.maxWebAudioPlayers));
}

MATCHER_P(SmartPtrMatcherError, expectedPtr, "")
{
    return expectedPtr == arg.get();
}
} // namespace

SessionServerAppManagerMediaCapabilitiesErrorTests::SessionServerAppManagerMediaCapabilitiesErrorTests()
    : m_controller{std::make_unique<StrictMock<rialto::servermanager::ipc::ControllerMock>>()},
      m_stateObserver{std::make_shared<StrictMock<rialto::servermanager::service::StateObserverMock>>()},
      m_sessionServerAppMock{std::make_shared<StrictMock<rialto::servermanager::common::SessionServerAppMock>>()},
      m_sessionServerAppFactory{
          std::make_unique<StrictMock<rialto::servermanager::common::SessionServerAppFactoryMock>>()},
      m_healthcheckServiceFactory{
          std::make_unique<StrictMock<rialto::servermanager::common::HealthcheckServiceFactoryMock>>()},
      m_healthcheckService{std::make_unique<StrictMock<rialto::servermanager::common::HealthcheckServiceMock>>()},
      m_controllerMock{dynamic_cast<StrictMock<rialto::servermanager::ipc::ControllerMock> &>(*m_controller)},
      m_sessionServerAppFactoryMock{dynamic_cast<StrictMock<rialto::servermanager::common::SessionServerAppFactoryMock> &>(
          *m_sessionServerAppFactory)},
      m_healthcheckServiceFactoryMock{
          dynamic_cast<StrictMock<rialto::servermanager::common::HealthcheckServiceFactoryMock> &>(
              *m_healthcheckServiceFactory)},
      m_healthcheckServiceMock{
          dynamic_cast<StrictMock<rialto::servermanager::common::HealthcheckServiceMock> &>(*m_healthcheckService)},
      m_mediaCapabilities{std::make_shared<StrictMock<MediaCapabilitiesMock>>()},
      m_mediaCapabilitiesMock{*m_mediaCapabilities}
{
    auto eventThreadFactoryMock = std::make_shared<StrictMock<firebolt::rialto::common::EventThreadFactoryMock>>();
    auto eventThreadMock = std::make_unique<StrictMock<firebolt::rialto::common::EventThreadMock>>();
    EXPECT_CALL(*eventThreadMock, addImpl(_)).WillRepeatedly(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*eventThreadMock, flush());
    EXPECT_CALL(*eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(eventThreadMock))));
    EXPECT_CALL(m_healthcheckServiceFactoryMock, createHealthcheckService(_))
        .WillOnce(Return(ByMove(std::move(m_healthcheckService))));
    EXPECT_CALL(m_mediaCapabilitiesMock, getAudioDecoderCapabilities(_))
        .WillOnce(Return(firebolt::rialto::DecoderCapabilitiesStatus::CONFIG_NOT_FOUND));
    EXPECT_CALL(m_mediaCapabilitiesMock, getVideoDecoderCapabilities(_))
        .WillOnce(Return(firebolt::rialto::DecoderCapabilitiesStatus::CONFIG_NOT_FOUND));
    m_sut =
        std::make_unique<rialto::servermanager::common::SessionServerAppManager>(m_controller, m_stateObserver,
                                                                                 std::move(m_sessionServerAppFactory),
                                                                                 std::move(m_healthcheckServiceFactory),
                                                                                 eventThreadFactoryMock,
                                                                                 m_namedSocketFactoryMock,m_mediaCapabilities);
}

void SessionServerAppManagerMediaCapabilitiesErrorTests::sessionServerWillLaunchWithoutCapabilities(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_namedSocketFactoryMock, createNamedSocket()).WillOnce(Return(ByMove(std::move(m_namedSocket))));
    EXPECT_CALL(m_sessionServerAppFactoryMock,
                create(kAppName, state, kAppConfig, _, SmartPtrMatcherError(&m_namedSocketMock)))
        .WillOnce(Return(m_sessionServerAppMock));
    EXPECT_CALL(*m_sessionServerAppMock, launch()).WillOnce(Return(true));
    EXPECT_CALL(*m_sessionServerAppMock, isConnected()).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_sessionServerAppMock, getSessionManagementSocketName()).WillRepeatedly(Return(kSessionServerSocketName));
    EXPECT_CALL(*m_sessionServerAppMock, getClientDisplayName()).WillRepeatedly(Return(kClientDisplayName));
    EXPECT_CALL(*m_sessionServerAppMock, getInitialState()).WillRepeatedly(Return(state));
    EXPECT_CALL(*m_sessionServerAppMock, getSessionManagementSocketPermissions()).WillRepeatedly(Return(kSocketPermissions));
    EXPECT_CALL(*m_sessionServerAppMock, getSessionManagementSocketOwner()).WillRepeatedly(Return(kSocketOwner));
    EXPECT_CALL(*m_sessionServerAppMock, getSessionManagementSocketGroup()).WillRepeatedly(Return(kSocketGroup));
    EXPECT_CALL(*m_sessionServerAppMock, getMaxPlaybackSessions()).WillRepeatedly(Return(kMaxSessions));
    EXPECT_CALL(*m_sessionServerAppMock, getMaxWebAudioPlayers()).WillRepeatedly(Return(kMaxWebAudioPlayers));
    EXPECT_CALL(*m_sessionServerAppMock, isPreloaded()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_controllerMock,
                performSetConfiguration(kServerId, state,
                                        kSessionServerSocketName, kClientDisplayName,
                                        MaxResourceMatcherError(kMaxSessions, kMaxWebAudioPlayers), kSocketPermissions,
                                        kSocketOwner, kSocketGroup, kAppName, Eq(std::nullopt), Eq(std::nullopt)))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_stateObserver, stateChanged(kAppName, firebolt::rialto::common::SessionServerState::UNINITIALIZED));
}

TEST_F(SessionServerAppManagerMediaCapabilitiesErrorTests, MediaCapabilitiesOptionalsShoulBeNulloptWhenCapabilitiesAreNotFound)
{
    sessionServerWillLaunchWithoutCapabilities(firebolt::rialto::common::SessionServerState::INACTIVE);
    auto app = m_sut->initiateApplication(kAppName, firebolt::rialto::common::SessionServerState::INACTIVE, kAppConfig);
    ASSERT_TRUE(app);
}
