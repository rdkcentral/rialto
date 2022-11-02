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

#ifndef MEDIA_KEYS_MODULE_SERVICE_TESTS_FIXTURE_H_
#define MEDIA_KEYS_MODULE_SERVICE_TESTS_FIXTURE_H_

#include "CdmServiceMock.h"
#include "ClosureMock.h"
#include "IMediaKeysClient.h"
#include "IMediaKeysModuleService.h"
#include "IpcClientMock.h"
#include "IpcControllerMock.h"
#include "IpcServerMock.h"
#include "RpcControllerMock.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class MediaKeysModuleServiceTests : public testing::Test
{
public:
    MediaKeysModuleServiceTests();
    ~MediaKeysModuleServiceTests() override;

    void clientWillConnect();
    void clientWillDisconnect();
    void cdmServiceWillCreateMediaKeys();
    void cdmServiceWillFailToCreateMediaKeys();
    void cdmServiceWillDestroyMediaKeys();
    void cdmServiceWillFailToDestroyMediaKeys();
    void cdmServiceWillCreateKeySession();
    void cdmServiceWillFailToCreateKeySession();
    void cdmServiceWillGenerateRequest();
    void cdmServiceWillFailToGenerateRequest();
    void cdmServiceWillLoadSession();
    void cdmServiceWillFailToLoadSession();
    void cdmServiceWillUpdateSession();
    void cdmServiceWillFailToUpdateSession();
    void cdmServiceWillCloseKeySession();
    void cdmServiceWillFailToCloseKeySession();
    void cdmServiceWillRemoveKeySession();
    void cdmServiceWillFailToRemoveKeySession();
    void cdmServiceWillGetCdmKeySessionId();
    void cdmServiceWillFailToGetCdmKeySessionId();

    void mediaClientWillSendLicenseRequestEvent();
    void mediaClientWillSendLicenseRenewalEvent();
    void mediaClientWillSendKeyStatusesChangedEvent();

    void sendClientConnected();
    void sendClientDisconnected();
    int sendCreateMediaKeysRequestAndReceiveResponse();
    void sendCreateMediaKeysRequestWithInvalidIpcAndReceiveFailedResponse();
    void sendCreateMediaKeysRequestAndExpectFailure();
    void sendDestroyMediaKeysRequestAndReceiveResponse();
    void sendDestroyMediaKeysRequestWithInvalidIpcAndReceiveFailedResponse();
    void sendCreateKeySessionRequestAndReceiveResponse();
    void sendCreateKeySessionRequestAndReceiveErrorResponse();
    void sendCreateKeySessionRequestWithInvalidIpcAndReceiveFailedResponse();
    void sendGenerateRequestRequestAndReceiveResponse();
    void sendGenerateRequestRequestAndReceiveErrorResponse();
    void sendLoadSessionRequestAndReceiveResponse();
    void sendLoadSessionRequestAndReceiveErrorResponse();
    void sendUpdateSessionRequestAndReceiveResponse();
    void sendUpdateSessionRequestAndReceiveErrorResponse();
    void sendCloseKeySessionRequestAndReceiveResponse();
    void sendCloseKeySessionRequestAndReceiveErrorResponse();
    void sendRemoveKeySessionRequestAndReceiveResponse();
    void sendRemoveKeySessionRequestAndReceiveErrorResponse();
    void sendGetCdmKeySessionIdRequestAndReceiveResponse();
    void sendGetCdmKeySessionIdRequestAndReceiveErrorResponse();

    void sendLicenseRequestEvent();
    void sendLicenseRenewalEvent();
    void sendKeyStatusesChangedEvent();

    void expectInvalidControllerRequestFailure();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::mock::ClientMock>> m_clientMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::mock::ServerMock>> m_serverMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::mock::ClosureMock>> m_closureMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::mock::ControllerMock>> m_controllerMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::mock::RpcControllerMock>> m_invalidControllerMock;
    StrictMock<firebolt::rialto::server::service::CdmServiceMock> m_cdmServiceMock;
    std::shared_ptr<firebolt::rialto::server::ipc::IMediaKeysModuleService> m_service;
    std::shared_ptr<firebolt::rialto::IMediaKeysClient> m_client;
    firebolt::rialto::KeyStatusVector m_keyStatuses;

    void expectRequestSuccess();
    void expectRequestFailure();

    void createKeyStatusVector();
};

#endif // MEDIA_KEYS_MODULE_SERVICE_TESTS_FIXTURE_H_
