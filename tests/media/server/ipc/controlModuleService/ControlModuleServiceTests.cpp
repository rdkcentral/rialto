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

#include "ControlModuleServiceTestsFixture.h"
#include <unistd.h>

TEST_F(ControlModuleServiceTests, shouldEstablishConnection)
{
    clientWillConnect();
    sendClientConnected();
}

TEST_F(ControlModuleServiceTests, shouldDisconnect)
{
    clientWillConnect();
    sendClientConnected();
    sendClientDisconnected();
}

TEST_F(ControlModuleServiceTests, shouldFailToRegisterClient)
{
    clientWillConnect();
    sendClientConnected();
    willFailDueToInvalidController();
    sendRegisterClientRequestWithInvalidControllerAndReceiveFailure();
}

TEST_F(ControlModuleServiceTests, shouldRegisterClientAndUnregisterItWhenDisconnected)
{
    clientWillConnect();
    sendClientConnected();
    controlServiceWillRegisterClient();
    sendRegisterClientRequestAndReceiveResponse();
    controlServiceWillRemoveControl();
    sendClientDisconnected();
}

TEST_F(ControlModuleServiceTests, shouldRegisterClientAndUnregisterItWhenDestructed)
{
    clientWillConnect();
    sendClientConnected();
    controlServiceWillRegisterClient();
    sendRegisterClientRequestAndReceiveResponse();
    controlServiceWillRemoveControl();
}

TEST_F(ControlModuleServiceTests, shouldRegisterClientWithCompatibleSchemaVersion)
{
    clientWillConnect();
    sendClientConnected();
    controlServiceWillRegisterClient();
    sendRegisterClientRequestAndReceiveResponse(firebolt::rialto::common::getCurrentSchemaVersion());
    controlServiceWillRemoveControl();
}

TEST_F(ControlModuleServiceTests, shouldFailToRegisterClientWithNotCompatibleSchemaVersion)
{
    firebolt::rialto::common::SchemaVersion schema{firebolt::rialto::common::getCurrentSchemaVersion().major() + 1, 0, 0};
    clientWillConnect();
    sendClientConnected();
    controlServiceWillFailToRegisterClient();
    sendRegisterClientRequestAndReceiveResponse(schema);
}

TEST_F(ControlModuleServiceTests, shouldGetSharedMemory)
{
    playbackServiceWillGetSharedMemory();
    sendGetSharedMemoryRequestAndReceiveResponse();
}

TEST_F(ControlModuleServiceTests, shouldFailToGetSharedMemory)
{
    playbackServiceWillFailToGetSharedMemory();
    sendGetSharedMemoryRequestAndExpectFailure();
}

TEST_F(ControlModuleServiceTests, shouldFailToAckWhenControlServiceReturnsFailure)
{
    playbackServiceWillFailToAck();
    sendAckRequestAndExpectFailure();
}

TEST_F(ControlModuleServiceTests, shouldAck)
{
    playbackServiceWillAck();
    sendAckRequestAndReceiveResponse();
}

TEST_F(ControlModuleServiceTests, FactoryCreatesObject)
{
    testFactoryCreatesObject();
}
