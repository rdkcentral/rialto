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

#include "IpcTestsFixture.h"

TEST_F(IpcTests, PerformSetStateShouldReturnFalseWhenNoAppIsConnected)
{
    ASSERT_FALSE(triggerPerformSetState(firebolt::rialto::common::SessionServerState::INACTIVE));
}

TEST_F(IpcTests, PerformPingShouldReturnFalseWhenNoAppIsConnected)
{
    ASSERT_FALSE(triggerPerformPing());
}

TEST_F(IpcTests, ShouldFailToCreateClientWhenServerIsNotRunning)
{
    ASSERT_FALSE(triggerCreateClientConnectToFakeSocket());
}

TEST_F(IpcTests, ShouldConnectToRialtoSessionServer)
{
    configureServerToSendOkResponses();
    ASSERT_TRUE(triggerCreateClient());
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldFailToConnectToTheSameAppTwice)
{
    configureServerToSendOkResponses();
    ASSERT_TRUE(triggerCreateClient());
    ASSERT_FALSE(triggerCreateClient());
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldRemoveClient)
{
    configureServerToSendOkResponses();
    ASSERT_TRUE(triggerCreateClient());
    triggerRemoveClient();
    // PerformSetState should fail, when client is removed
    ASSERT_FALSE(triggerPerformSetState(firebolt::rialto::common::SessionServerState::INACTIVE));
}

TEST_F(IpcTests, ShouldSuccessfullySetState)
{
    configureServerToSendOkResponses();
    ASSERT_TRUE(triggerCreateClient());
    ASSERT_TRUE(triggerPerformSetState(firebolt::rialto::common::SessionServerState::INACTIVE));
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldFailToSetStateWhenFailResponseIsReceived)
{
    configureServerToSendFailResponses();
    ASSERT_TRUE(triggerCreateClient());
    ASSERT_FALSE(triggerPerformSetState(firebolt::rialto::common::SessionServerState::INACTIVE));
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldSuccessfullyPing)
{
    configureServerToSendOkResponses();
    ASSERT_TRUE(triggerCreateClient());
    ASSERT_TRUE(triggerPerformPing());
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldFailToPingWhenFailResponseIsReceived)
{
    configureServerToSendFailResponses();
    ASSERT_TRUE(triggerCreateClient());
    ASSERT_FALSE(triggerPerformPing());
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldForwardStateChangedIndicationToSessionServerAppManager)
{
    configureServerToSendOkResponses();
    ASSERT_TRUE(triggerCreateClient());
    sessionServerAppManagerWillBeNotifiedAboutSessionServerStateChange(
        firebolt::rialto::common::SessionServerState::INACTIVE);
    simulateStateChangedEventInactive();
    waitForExpectationsMet();
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldForwardAckEventToSessionServerAppManager)
{
    configureServerToSendOkResponses();
    ASSERT_TRUE(triggerCreateClient());
    sessionServerAppManagerWillBeNotifiedAboutCompletedHealthcheck();
    simulateAckEvent();
    waitForExpectationsMet();
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldForwardNotRunningStateChangeToSessionServerAppManagerWhenUnexpectedlyDisconnected)
{
    configureServerToSendOkResponses();
    ASSERT_TRUE(triggerCreateClient());
    sessionServerAppManagerWillBeRequestedToRestartServer();
    simulateClientDisconnection();
    waitForExpectationsMet();
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldSuccessfullySetLogLevels)
{
    configureServerToSendOkResponses();
    ASSERT_TRUE(triggerCreateClient());
    ASSERT_TRUE(triggerSetLogLevels());
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldFailToSetLogLevels)
{
    configureServerToSendFailResponses();
    ASSERT_TRUE(triggerCreateClient());
    ASSERT_FALSE(triggerSetLogLevels());
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldSuccessfullySetConfiguration)
{
    configureServerToSendOkResponses();
    ASSERT_TRUE(triggerCreateClient());
    ASSERT_TRUE(triggerPerformSetConfiguration());
    triggerRemoveClient();
}

TEST_F(IpcTests, ShouldFailToSetConfiguration)
{
    configureServerToSendFailResponses();
    ASSERT_TRUE(triggerCreateClient());
    ASSERT_FALSE(triggerPerformSetConfiguration());
    triggerRemoveClient();
}
