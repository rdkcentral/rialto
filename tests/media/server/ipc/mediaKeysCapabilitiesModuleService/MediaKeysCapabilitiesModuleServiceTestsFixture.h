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

#ifndef MEDIA_KEYS_CAPABILITIES_MODULE_SERVICE_TESTS_FIXTURE_H_
#define MEDIA_KEYS_CAPABILITIES_MODULE_SERVICE_TESTS_FIXTURE_H_

#include "CdmServiceMock.h"
#include "ClosureMock.h"
#include "IMediaKeysCapabilitiesModuleService.h"
#include "IpcClientMock.h"
#include "IpcControllerMock.h"
#include "IpcServerMock.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class MediaKeysCapabilitiesModuleServiceTests : public testing::Test
{
public:
    MediaKeysCapabilitiesModuleServiceTests();
    ~MediaKeysCapabilitiesModuleServiceTests() override;

    void testFactoryCreatesObject();

    void clientWillConnect();
    void cdmServiceWillGetSupportedKeySystems();
    void cdmServiceWillSupportsKeySystem();
    void cdmServiceWillGetSupportedKeySystemVersion();
    void cdmServiceWillFailToGetSupportedKeySystemVersion();

    void sendClientConnected();
    void sendGetSupportedKeySystemsRequestAndReceiveResponse();
    void sendSupportsKeySystemRequestAndReceiveResponse();
    void sendGetSupportedKeySystemVersionRequestAndReceiveResponse();
    void sendGetSupportedKeySystemVersionRequestAndExpectFailure();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_clientMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ServerMock>> m_serverMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClosureMock>> m_closureMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ControllerMock>> m_controllerMock;
    StrictMock<firebolt::rialto::server::service::CdmServiceMock> m_cdmServiceMock;
    std::shared_ptr<firebolt::rialto::server::ipc::IMediaKeysCapabilitiesModuleService> m_service;

    void expectRequestSuccess();
    void expectRequestFailure();
};

#endif // MEDIA_KEYS_CAPABILITIES_MODULE_SERVICE_TESTS_FIXTURE_H_
