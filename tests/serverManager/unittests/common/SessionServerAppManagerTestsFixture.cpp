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
#include "LoggingLevels.h"
#include "Matchers.h"
#include "SessionServerAppManager.h"
#include <string>
#include <utility>

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
const std::string APP_NAME{"YouTube"};
constexpr int kAppId{3};
const int APP_MGMT_SOCKET{0};
const std::string SESSION_SERVER_SOCKET_NAME{getenv("RIALTO_SOCKET_PATH")};
constexpr int MAX_SESSIONS{2};
constexpr int MAX_WEB_AUDIO_PLAYERS{3};
constexpr int NUM_OF_PRELOADED_SERVERS{0};
constexpr rialto::servermanager::service::LoggingLevels
    EXAMPLE_LOGGING_LEVELS{rialto::servermanager::service::LoggingLevel::FATAL,
                           rialto::servermanager::service::LoggingLevel::ERROR,
                           rialto::servermanager::service::LoggingLevel::WARNING,
                           rialto::servermanager::service::LoggingLevel::MILESTONE,
                           rialto::servermanager::service::LoggingLevel::INFO};
const firebolt::rialto::common::AppConfig APP_CONFIG{SESSION_SERVER_SOCKET_NAME};
} // namespace

MATCHER_P2(MaxResourceMatcher, maxPlaybacks, maxWebAudioPlayers, "")
{
    return ((maxPlaybacks == arg.maxPlaybacks) && (maxWebAudioPlayers == arg.maxWebAudioPlayers));
}

SessionServerAppManagerTests::SessionServerAppManagerTests()
    : m_controller{std::make_unique<StrictMock<rialto::servermanager::ipc::ControllerMock>>()},
      m_stateObserver{std::make_shared<StrictMock<rialto::servermanager::service::StateObserverMock>>()},
      m_sessionServerApp{std::make_unique<StrictMock<rialto::servermanager::common::SessionServerAppMock>>()},
      m_controllerMock{dynamic_cast<StrictMock<rialto::servermanager::ipc::ControllerMock> &>(*m_controller)},
      m_sessionServerAppMock{
          dynamic_cast<StrictMock<rialto::servermanager::common::SessionServerAppMock> &>(*m_sessionServerApp)}
{
    auto appFactory = std::make_unique<StrictMock<rialto::servermanager::common::SessionServerAppFactoryMock>>();
    m_sessionServerAppFactoryMock = appFactory.get();

    auto eventThreadFactoryMock = std::make_shared<StrictMock<firebolt::rialto::common::EventThreadFactoryMock>>();
    auto eventThreadMock = std::make_unique<StrictMock<firebolt::rialto::common::EventThreadMock>>();
    m_eventThreadMock = eventThreadMock.get();
    EXPECT_CALL(*eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(eventThreadMock))));

    m_sut = std::make_unique<rialto::servermanager::common::SessionServerAppManager>(m_controller, m_stateObserver,
                                                                                     std::move(appFactory),
                                                                                     eventThreadFactoryMock,
                                                                                     NUM_OF_PRELOADED_SERVERS);
}

void SessionServerAppManagerTests::sessionServerLaunchWillFail(const firebolt::rialto::common::SessionServerState &state)
{
    ASSERT_TRUE(m_sessionServerAppFactoryMock);
    EXPECT_CALL(*m_sessionServerAppFactoryMock, create(APP_NAME, state, APP_CONFIG, _))
        .WillOnce(Return(ByMove(std::move(m_sessionServerApp))));
    EXPECT_CALL(m_sessionServerAppMock, launch()).WillOnce(Return(false));
}

void SessionServerAppManagerTests::sessionServerConnectWillFail(const firebolt::rialto::common::SessionServerState &state)
{
    ASSERT_TRUE(m_sessionServerAppFactoryMock);
    EXPECT_CALL(*m_sessionServerAppFactoryMock, create(APP_NAME, state, APP_CONFIG, _))
        .WillOnce(Return(ByMove(std::move(m_sessionServerApp))));
    EXPECT_CALL(m_sessionServerAppMock, launch()).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerAppMock, getAppName()).WillRepeatedly(ReturnRef(APP_NAME));
    EXPECT_CALL(m_sessionServerAppMock, getAppManagementSocketName()).WillOnce(Return(APP_MGMT_SOCKET));
    EXPECT_CALL(m_sessionServerAppMock, getAppId()).WillRepeatedly(Return(kAppId));
    EXPECT_CALL(m_controllerMock, createClient(kAppId, APP_MGMT_SOCKET)).WillOnce(Return(false));
    EXPECT_CALL(m_sessionServerAppMock, kill());
}

void SessionServerAppManagerTests::sessionServerChangeStateWillFail(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_sessionServerAppMock, getAppId()).WillRepeatedly(Return(kAppId));
    EXPECT_CALL(m_controllerMock, performSetState(kAppId, state)).WillOnce(Return(false));
}

void SessionServerAppManagerTests::sessionServerWillLaunch(const firebolt::rialto::common::SessionServerState &state)
{
    ASSERT_TRUE(m_sessionServerAppFactoryMock);
    EXPECT_CALL(*m_sessionServerAppFactoryMock, create(APP_NAME, state, APP_CONFIG, _))
        .WillOnce(Return(ByMove(std::move(m_sessionServerApp))));
    EXPECT_CALL(m_sessionServerAppMock, launch()).WillOnce(Return(true));
    EXPECT_CALL(m_sessionServerAppMock, getAppName()).WillRepeatedly(ReturnRef(APP_NAME));
    EXPECT_CALL(m_sessionServerAppMock, getAppManagementSocketName()).WillOnce(Return(APP_MGMT_SOCKET));
    EXPECT_CALL(m_sessionServerAppMock, getAppId()).WillRepeatedly(Return(kAppId));
    EXPECT_CALL(m_controllerMock, createClient(kAppId, APP_MGMT_SOCKET)).WillOnce(Return(true));
}

void SessionServerAppManagerTests::sessionServerWillChangeState(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_controllerMock, performSetState(kAppId, state)).WillOnce(Return(true));
}

void SessionServerAppManagerTests::sessionServerWillChangeStateToUninitialized()
{
    EXPECT_CALL(m_sessionServerAppMock, cancelStartupTimer());
    EXPECT_CALL(m_sessionServerAppMock, getInitialState())
        .WillOnce(Return(firebolt::rialto::common::SessionServerState::INACTIVE));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketName()).WillOnce(Return(SESSION_SERVER_SOCKET_NAME));
    EXPECT_CALL(m_sessionServerAppMock, getMaxPlaybackSessions()).WillOnce(Return(MAX_SESSIONS));
    EXPECT_CALL(m_sessionServerAppMock, getMaxWebAudioPlayers()).WillOnce(Return(MAX_WEB_AUDIO_PLAYERS));
    EXPECT_CALL(m_sessionServerAppMock, isPreloaded()).WillOnce(Return(false));
    EXPECT_CALL(m_controllerMock, performSetConfiguration(kAppId, firebolt::rialto::common::SessionServerState::INACTIVE,
                                                          SESSION_SERVER_SOCKET_NAME,
                                                          MaxResourceMatcher(MAX_SESSIONS, MAX_WEB_AUDIO_PLAYERS)))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*m_stateObserver, stateChanged(APP_NAME, firebolt::rialto::common::SessionServerState::UNINITIALIZED));
}

void SessionServerAppManagerTests::sessionServerWillChangeStateToInactive()
{
    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*m_stateObserver, stateChanged(APP_NAME, firebolt::rialto::common::SessionServerState::INACTIVE));
}

void SessionServerAppManagerTests::sessionServerWillFailToSetConfiguration()
{
    EXPECT_CALL(m_sessionServerAppMock, cancelStartupTimer());
    EXPECT_CALL(m_sessionServerAppMock, getInitialState())
        .WillOnce(Return(firebolt::rialto::common::SessionServerState::INACTIVE));
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketName()).WillOnce(Return(SESSION_SERVER_SOCKET_NAME));
    EXPECT_CALL(m_sessionServerAppMock, getMaxPlaybackSessions()).WillOnce(Return(MAX_SESSIONS));
    EXPECT_CALL(m_sessionServerAppMock, getMaxWebAudioPlayers()).WillOnce(Return(MAX_WEB_AUDIO_PLAYERS));
    EXPECT_CALL(m_sessionServerAppMock, isPreloaded()).WillOnce(Return(false));
    EXPECT_CALL(m_controllerMock, performSetConfiguration(kAppId, firebolt::rialto::common::SessionServerState::INACTIVE,
                                                          SESSION_SERVER_SOCKET_NAME,
                                                          MaxResourceMatcher(MAX_SESSIONS, MAX_WEB_AUDIO_PLAYERS)))
        .WillOnce(Return(false));
}

void SessionServerAppManagerTests::sessionServerWillSetLogLevels()
{
    EXPECT_CALL(m_controllerMock, setLogLevels(EXAMPLE_LOGGING_LEVELS)).WillOnce(Return(true));
}

void SessionServerAppManagerTests::sessionServerWillFailToSetLogLevels()
{
    EXPECT_CALL(m_controllerMock, setLogLevels(EXAMPLE_LOGGING_LEVELS)).WillOnce(Return(false));
}

void SessionServerAppManagerTests::clientWillBeRemovedAfterStateChangedIndication(
    const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_CALL(m_controllerMock, removeClient(kAppId));
    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*m_stateObserver, stateChanged(APP_NAME, state));
}

void SessionServerAppManagerTests::sessionServerWillKillRunningApplicationAtTeardown()
{
    EXPECT_CALL(m_sessionServerAppMock, kill());
}

void SessionServerAppManagerTests::sessionServerWillReturnAppSocketName(const std::string &socketName)
{
    EXPECT_CALL(m_sessionServerAppMock, getSessionManagementSocketName()).WillOnce(Return(socketName));
}

bool SessionServerAppManagerTests::triggerInitiateApplication(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_TRUE(m_sut);
    return m_sut->initiateApplication(APP_NAME, state, APP_CONFIG);
}

bool SessionServerAppManagerTests::triggerSetSessionServerState(const firebolt::rialto::common::SessionServerState &newState)
{
    EXPECT_TRUE(m_sut);
    return m_sut->setSessionServerState(APP_NAME, newState);
}

void SessionServerAppManagerTests::triggerOnSessionServerStateChanged(
    const firebolt::rialto::common::SessionServerState &newState)
{
    EXPECT_TRUE(m_sut);
    return m_sut->onSessionServerStateChanged(kAppId, newState);
}

std::string SessionServerAppManagerTests::triggerGetAppConnectionInfo()
{
    EXPECT_TRUE(m_sut);
    return m_sut->getAppConnectionInfo(APP_NAME);
}

bool SessionServerAppManagerTests::triggerSetLogLevel()
{
    EXPECT_TRUE(m_sut);
    return m_sut->setLogLevels(EXAMPLE_LOGGING_LEVELS);
}
