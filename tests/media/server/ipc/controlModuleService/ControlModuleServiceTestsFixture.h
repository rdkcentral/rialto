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

#ifndef CONTROL_MODULE_SERVICE_TESTS_FIXTURE_H_
#define CONTROL_MODULE_SERVICE_TESTS_FIXTURE_H_

#include "ClosureMock.h"
#include "ControlServiceMock.h"
#include "IControlModuleService.h"
#include "IpcClientMock.h"
#include "IpcControllerMock.h"
#include "IpcServerMock.h"
#include "PlaybackServiceMock.h"
#include "RpcControllerMock.h"
#include "SchemaVersion.h"
#include <gtest/gtest.h>
#include <memory>
#include <optional>

using testing::StrictMock;

class ControlModuleServiceTests : public testing::Test
{
public:
    ControlModuleServiceTests();
    ~ControlModuleServiceTests() override;

    void clientWillConnect();
    void controlServiceWillRemoveControl();
    void controlServiceWillRegisterClient();
    void controlServiceWillFailToRegisterClient();
    void willFailDueToInvalidController();
    void playbackServiceWillGetSharedMemory();
    void playbackServiceWillFailToGetSharedMemory();
    void playbackServiceWillAck();
    void playbackServiceWillFailToAck();

    void sendClientConnected();
    void sendClientDisconnected();
    void sendRegisterClientRequestAndReceiveResponse(
        const std::optional<firebolt::rialto::common::SchemaVersion> &schemaVersion = std::nullopt);
    void sendRegisterClientRequestWithInvalidControllerAndReceiveFailure();
    void sendGetSharedMemoryRequestAndReceiveResponse();
    void sendGetSharedMemoryRequestAndExpectFailure();
    void sendAckRequestAndReceiveResponse();
    void sendAckRequestAndExpectFailure();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_clientMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ServerMock>> m_serverMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClosureMock>> m_closureMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ControllerMock>> m_controllerMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::RpcControllerMock>> m_invalidControllerMock;
    StrictMock<firebolt::rialto::server::service::PlaybackServiceMock> m_playbackServiceMock;
    StrictMock<firebolt::rialto::server::service::ControlServiceMock> m_controlServiceMock;
    std::shared_ptr<firebolt::rialto::server::ipc::IControlModuleService> m_service;
};

#endif // CONTROL_MODULE_SERVICE_TESTS_FIXTURE_H_
