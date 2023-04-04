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
    const std::string APP_SOCKET{getenv("RIALTO_SOCKET_PATH")};
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    sessionServerWillReturnAppSocketName(APP_SOCKET);
    ASSERT_EQ(APP_SOCKET, triggerGetAppConnectionInfo());
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
    ASSERT_FALSE(triggerSetSessionServerState(firebolt::rialto::common::SessionServerState::ACTIVE));
    sessionServerWillKillRunningApplication();
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
    clientWillBeRemovedAfterStateChangedIndication(firebolt::rialto::common::SessionServerState::ERROR);
    sessionServerWillKillRunningApplication();
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldRemoveApplicationWhenNotRunningIndicationIsReceived)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    clientWillBeRemovedAfterStateChangedIndication(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    triggerOnSessionServerStateChanged(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldRemoveApplicationWhenErrorIndicationIsReceived)
{
    sessionServerWillLaunch(firebolt::rialto::common::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(firebolt::rialto::common::SessionServerState::INACTIVE));
    clientWillBeRemovedAfterStateChangedIndication(firebolt::rialto::common::SessionServerState::ERROR);
    sessionServerWillKillRunningApplication();
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
