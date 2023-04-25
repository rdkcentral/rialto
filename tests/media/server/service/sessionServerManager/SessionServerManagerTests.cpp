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

TEST_F(SessionServerManagerTests, shouldNotInitializeWithWrongNumberOfArgs)
{
    willNotInitializeWithWrongNumberOfArgs();
}

TEST_F(SessionServerManagerTests, shouldNotInitializeWithWrongSocket)
{
    willNotInitializeWithWrongSocket();
}

TEST_F(SessionServerManagerTests, shouldNotInitializeWhenApplicationManagementServerFailsToInit)
{
    willNotInitializeWhenApplicationManagementServerFailsToInit();
}

TEST_F(SessionServerManagerTests, shouldNotInitializeWhenSomethingThrowsException)
{
    willNotInitializeWhenApplicationManagementServerThrows();
}

TEST_F(SessionServerManagerTests, shouldNotInitializeWhenApplicationManagementServerFailsToSendEvent)
{
    willNotInitializeWhenApplicationManagementServerFailsToSendEvent();
}

TEST_F(SessionServerManagerTests, shouldInitialize)
{
    willInitialize();
}

TEST_F(SessionServerManagerTests, shouldFailToSetConfigurationWhenSessionManagementServerFailsToInit)
{
    willFailToSetConfigurationWhenSessionManagementServerFailsToInit();
}

TEST_F(SessionServerManagerTests, shouldFailToSetConfigurationWhenSessionManagementServerFailsToSetInitialState)
{
    willFailToSetConfigurationWhenSessionManagementServerFailsToSetInitialState();
}

TEST_F(SessionServerManagerTests, shouldSetConfiguration)
{
    willSetConfiguration();
}

TEST_F(SessionServerManagerTests, shouldFailToSetUnsupportedState)
{
    willFailToSetUnsupportedState();
    setStateShouldFail(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
    setStateShouldFail(firebolt::rialto::common::SessionServerState::ERROR);
}

TEST_F(SessionServerManagerTests, shouldFailToSetActiveStateDueToPlaybackServiceError)
{
    willFailToSetStateActiveDueToPlaybackServiceError();
    setStateShouldFail(firebolt::rialto::common::SessionServerState::ACTIVE);
}

TEST_F(SessionServerManagerTests, shouldFailToSetActiveStateDueToCdmServiceError)
{
    willFailToSetStateActiveDueToCdmServiceError();
    setStateShouldFail(firebolt::rialto::common::SessionServerState::ACTIVE);
}

TEST_F(SessionServerManagerTests, shouldFailToSetActiveStateDueToSessionServerError)
{
    willFailToSetStateActiveDueToSessionServerError();
    setStateShouldFail(firebolt::rialto::common::SessionServerState::ACTIVE);
}

TEST_F(SessionServerManagerTests, shouldSetActiveState)
{
    willSetStateActive();
    setStateShouldSucceed(firebolt::rialto::common::SessionServerState::ACTIVE);
}

TEST_F(SessionServerManagerTests, shouldSkipSettingActiveStateTwice)
{
    willSetStateActive();
    setStateShouldSucceed(firebolt::rialto::common::SessionServerState::ACTIVE);
    setStateShouldSucceed(firebolt::rialto::common::SessionServerState::ACTIVE);
}

TEST_F(SessionServerManagerTests, shouldFailToSetInactiveState)
{
    willFailToSetStateInactive();
    setStateShouldFail(firebolt::rialto::common::SessionServerState::INACTIVE);
}

TEST_F(SessionServerManagerTests, shouldFailToSetInactiveStateAndGoBackToActive)
{
    willSetStateActive();
    setStateShouldSucceed(firebolt::rialto::common::SessionServerState::ACTIVE);
    willFailToSetStateInactiveAndGoBackToActive();
    setStateShouldFail(firebolt::rialto::common::SessionServerState::INACTIVE);
}

TEST_F(SessionServerManagerTests, shouldSetInactiveState)
{
    willSetStateInactive();
    setStateShouldSucceed(firebolt::rialto::common::SessionServerState::INACTIVE);
}

TEST_F(SessionServerManagerTests, shouldSkipSettingInactiveStateTwice)
{
    willSetStateInactive();
    setStateShouldSucceed(firebolt::rialto::common::SessionServerState::INACTIVE);
    setStateShouldSucceed(firebolt::rialto::common::SessionServerState::INACTIVE);
}

TEST_F(SessionServerManagerTests, shouldFailToSetNotRunningState)
{
    willFailToSetStateNotRunning();
    setStateShouldFail(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
}

TEST_F(SessionServerManagerTests, shouldSetNotRunningState)
{
    willSetStateNotRunning();
    setStateShouldSucceed(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
}

TEST_F(SessionServerManagerTests, shouldSkipSettingNotRunningStateTwice)
{
    willSetStateNotRunning();
    setStateShouldSucceed(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    setStateShouldSucceed(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
}

TEST_F(SessionServerManagerTests, shouldStopServiceWhenNotRunningIsSet)
{
    triggerStartService();
    willSetStateNotRunning();
    setStateShouldSucceed(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
}

TEST_F(SessionServerManagerTests, shouldSetLogLevels)
{
    willSetLogLevels();
    triggerSetLogLevels();
}
