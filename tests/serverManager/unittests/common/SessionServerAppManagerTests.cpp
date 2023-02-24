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
    ASSERT_FALSE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::NOT_RUNNING));
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, InitiateApplicationShouldReturnFalseWhenSessionServerFailedToLaunch)
{
    sessionServerLaunchWillFail(rialto::servermanager::service::SessionServerState::INACTIVE);
    ASSERT_FALSE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, InitiateApplicationShouldReturnFalseWhenSessionServerFailedToConnect)
{
    sessionServerConnectWillFail(rialto::servermanager::service::SessionServerState::INACTIVE);
    ASSERT_FALSE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, InitiateApplicationShouldReturnTrueWhenSessionServerIsLaunched)
{
    sessionServerWillLaunch(rialto::servermanager::service::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    sessionServerWillKillRunningApplicationAtTeardown();
}

TEST_F(SessionServerAppManagerTests, InitiateApplicationShouldReturnFalseWhenCalledForRunningApplication)
{
    sessionServerWillLaunch(rialto::servermanager::service::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    ASSERT_FALSE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    sessionServerWillKillRunningApplicationAtTeardown();
}

TEST_F(SessionServerAppManagerTests, GetConnectionInfoShouldReturnProperSocket)
{
    const std::string APP_SOCKET{getenv("RIALTO_SOCKET_PATH")};
    sessionServerWillLaunch(rialto::servermanager::service::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    sessionServerWillReturnAppSocketName(APP_SOCKET);
    ASSERT_EQ(APP_SOCKET, triggerGetAppConnectionInfo());
    sessionServerWillKillRunningApplicationAtTeardown();
}

TEST_F(SessionServerAppManagerTests, SetSessionServerStateShouldReturnFalseWhenAppIsNotLaunched)
{
    ASSERT_FALSE(triggerSetSessionServerState(rialto::servermanager::service::SessionServerState::ACTIVE));
}

TEST_F(SessionServerAppManagerTests, SetSessionServerStateShouldReturnFalseWhenUnableToSendMessage)
{
    sessionServerWillLaunch(rialto::servermanager::service::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    sessionServerChangeStateWillFail(rialto::servermanager::service::SessionServerState::ACTIVE);
    ASSERT_FALSE(triggerSetSessionServerState(rialto::servermanager::service::SessionServerState::ACTIVE));
    sessionServerWillKillRunningApplicationAtTeardown();
}

TEST_F(SessionServerAppManagerTests, SetSessionServerStateShouldReturnTrueWhenStateIsChanged)
{
    sessionServerWillLaunch(rialto::servermanager::service::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    sessionServerWillChangeState(rialto::servermanager::service::SessionServerState::ACTIVE);
    ASSERT_TRUE(triggerSetSessionServerState(rialto::servermanager::service::SessionServerState::ACTIVE));
    sessionServerWillKillRunningApplicationAtTeardown();
}

TEST_F(SessionServerAppManagerTests, StateObserverShouldBeInformedAboutStateChangeToInactive)
{
    sessionServerWillLaunch(rialto::servermanager::service::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    sessionServerWillChangeStateToUninitialized();
    triggerOnSessionServerStateChanged(rialto::servermanager::service::SessionServerState::UNINITIALIZED);
    sessionServerWillChangeStateToInactive();
    triggerOnSessionServerStateChanged(rialto::servermanager::service::SessionServerState::INACTIVE);
    sessionServerWillKillRunningApplicationAtTeardown();
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldRemoveApplicationWhenSetConfigurationFails)
{
    sessionServerWillLaunch(rialto::servermanager::service::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    sessionServerWillFailToSetConfiguration();
    clientWillBeRemovedAfterStateChangedIndication(rialto::servermanager::service::SessionServerState::ERROR);
    triggerOnSessionServerStateChanged(rialto::servermanager::service::SessionServerState::UNINITIALIZED);
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldRemoveApplicationWhenNotRunningIndicationIsReceived)
{
    sessionServerWillLaunch(rialto::servermanager::service::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    clientWillBeRemovedAfterStateChangedIndication(rialto::servermanager::service::SessionServerState::NOT_RUNNING);
    triggerOnSessionServerStateChanged(rialto::servermanager::service::SessionServerState::NOT_RUNNING);
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldRemoveApplicationWhenErrorIndicationIsReceived)
{
    sessionServerWillLaunch(rialto::servermanager::service::SessionServerState::INACTIVE);
    ASSERT_TRUE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    clientWillBeRemovedAfterStateChangedIndication(rialto::servermanager::service::SessionServerState::ERROR);
    triggerOnSessionServerStateChanged(rialto::servermanager::service::SessionServerState::ERROR);
    ASSERT_TRUE(triggerGetAppConnectionInfo().empty());
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldSetNewLogLevel)
{
    sessionServerWillLaunch(rialto::servermanager::service::SessionServerState::INACTIVE);
    sessionServerWillSetLogLevels();
    ASSERT_TRUE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    ASSERT_TRUE(triggerSetLogLevel());
    sessionServerWillKillRunningApplicationAtTeardown();
}

TEST_F(SessionServerAppManagerTests, SessionServerAppManagerShouldFailToSetNewLogLevel)
{
    sessionServerWillLaunch(rialto::servermanager::service::SessionServerState::INACTIVE);
    sessionServerWillFailToSetLogLevels();
    ASSERT_TRUE(triggerInitiateApplication(rialto::servermanager::service::SessionServerState::INACTIVE));
    ASSERT_FALSE(triggerSetLogLevel());
    sessionServerWillKillRunningApplicationAtTeardown();
}
