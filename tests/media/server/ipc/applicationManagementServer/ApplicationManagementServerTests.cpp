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

#include "ApplicationManagementServerTestsFixture.h"

TEST_F(ApplicationManagementServerTests, shouldInitialize)
{
    clientWillBeInitialized();
    initializeApplicationManager();
    clientWillNotBeConnected();
}

TEST_F(ApplicationManagementServerTests, shouldNotInitializeIfClientInitalizationFails)
{
    clientWillFailToInitialized();
    initializeApplicationManagerAndExpectFailure();
}

TEST_F(ApplicationManagementServerTests, shouldChangeState)
{
    clientWillBeInitialized();
    initializeApplicationManager();
    clientWillReceiveStateChangedEvent(firebolt::rialto::server::SessionServerState::ACTIVE);
    sendStateChangedEvent(firebolt::rialto::server::SessionServerState::ACTIVE);
    clientWillNotBeConnected();
}

TEST_F(ApplicationManagementServerTests, shouldNotChangeStateIfClientNotConnected)
{
    clientWillBeInitialized();
    initializeApplicationManager();
    clientWillNotBeConnected();
    sendStateChangedEventAndExpectFailure(firebolt::rialto::server::SessionServerState::ACTIVE);
    clientWillNotBeConnected();
}

TEST_F(ApplicationManagementServerTests, shouldStart)
{
    clientWillBeInitialized();
    initializeApplicationManager();
    serverThreadWillStart();
    startApplicationManager();
    clientWillNotBeConnected();
}

TEST_F(ApplicationManagementServerTests, shouldStop)
{
    clientWillBeInitialized();
    initializeApplicationManager();
    clientWillBeDisconnected();
    stopApplicationManager();
    clientWillNotBeConnected();
}
