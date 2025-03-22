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

#include "MediaKeysModuleServiceTestsFixture.h"
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

TEST_F(MediaKeysModuleServiceTests, shouldConnectClient)
{
    clientWillConnect();
    sendClientConnected();
}

TEST_F(MediaKeysModuleServiceTests, shouldCreateMediaKeys)
{
    cdmServiceWillCreateMediaKeys();
    sendCreateMediaKeysRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToCreateMediaKeysDueToInvalidIpc)
{
    expectInvalidControllerRequestFailure();
    sendCreateMediaKeysRequestWithInvalidIpcAndReceiveFailedResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToCreateMediaKeys)
{
    cdmServiceWillFailToCreateMediaKeys();
    sendCreateMediaKeysRequestAndExpectFailure();
}

TEST_F(MediaKeysModuleServiceTests, shouldDestroyMediaKeysWhenDisconnectClient)
{
    clientWillConnect();
    sendClientConnected();
    cdmServiceWillCreateMediaKeys();
    int sessionId = sendCreateMediaKeysRequestAndReceiveResponse();
    clientWillDisconnect(sessionId);
    sendClientDisconnected();
}

TEST_F(MediaKeysModuleServiceTests, shouldDestroyMediaKeys)
{
    cdmServiceWillDestroyMediaKeys();
    sendDestroyMediaKeysRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToDestroyMediaKeysDueToInvalidIpc)
{
    expectInvalidControllerRequestFailure();
    sendDestroyMediaKeysRequestWithInvalidIpcAndReceiveFailedResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToDestroyMediaKeys)
{
    cdmServiceWillFailToDestroyMediaKeys();
    sendDestroyMediaKeysRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldCreateKeySession)
{
    cdmServiceWillCreateKeySession();
    sendCreateKeySessionRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToCreateKeySessionDueToInvalidIpc)
{
    expectInvalidControllerRequestFailure();
    sendCreateKeySessionRequestWithInvalidIpcAndReceiveFailedResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToCreateKeySession)
{
    cdmServiceWillFailToCreateKeySession();
    sendCreateKeySessionRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldGenerateRequest)
{
    cdmServiceWillGenerateRequest();
    sendGenerateRequestRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToGenerateRequest)
{
    cdmServiceWillFailToGenerateRequest();
    sendGenerateRequestRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldLoadSession)
{
    cdmServiceWillLoadSession();
    sendLoadSessionRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToLoadSession)
{
    cdmServiceWillFailToLoadSession();
    sendLoadSessionRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldUpdateSession)
{
    cdmServiceWillUpdateSession();
    sendUpdateSessionRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToUpdateSession)
{
    cdmServiceWillFailToUpdateSession();
    sendUpdateSessionRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldCloseKeySession)
{
    cdmServiceWillCloseKeySession();
    sendCloseKeySessionRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToCloseKeySession)
{
    cdmServiceWillFailToCloseKeySession();
    sendCloseKeySessionRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldRemoveKeySession)
{
    cdmServiceWillRemoveKeySession();
    sendRemoveKeySessionRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToRemoveKeySession)
{
    cdmServiceWillFailToRemoveKeySession();
    sendRemoveKeySessionRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldGetCdmKeySessionId)
{
    cdmServiceWillGetCdmKeySessionId();
    sendGetCdmKeySessionIdRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToGetCdmKeySessionId)
{
    cdmServiceWillFailToGetCdmKeySessionId();
    sendGetCdmKeySessionIdRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldGetExistingKey)
{
    cdmServiceWillGetExistingKey();
    sendContainsKeyRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToGetExistingKey)
{
    cdmServiceWillFailToGetExistingKey();
    sendContainsKeyRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldSetDrmHeader)
{
    cdmServiceWillSetDrmHeader();
    sendSetDrmHeaderRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToSetDrmHeader)
{
    cdmServiceWillFailToSetDrmHeader();
    sendSetDrmHeaderRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldDeleteDrmStore)
{
    cdmServiceWillDeleteDrmStore();
    sendDeleteDrmStoreRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToDeleteDrmStore)
{
    cdmServiceWillFailToDeleteDrmStore();
    sendDeleteDrmStoreRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldDeleteKeyStore)
{
    cdmServiceWillDeleteKeyStore();
    sendDeleteKeyStoreRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToDeleteKeyStore)
{
    cdmServiceWillFailToDeleteKeyStore();
    sendDeleteKeyStoreRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldGetDrmStoreHash)
{
    cdmServiceWillGetDrmStoreHash();
    sendGetDrmStoreHashRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToGetDrmStoreHash)
{
    cdmServiceWillFailToGetDrmStoreHash();
    sendGetDrmStoreHashRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldGetKeyStoreHash)
{
    cdmServiceWillGetKeyStoreHash();
    sendGetKeyStoreHashRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToGetKeyStoreHash)
{
    cdmServiceWillFailToGetKeyStoreHash();
    sendGetKeyStoreHashRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldGetLdlSessionsLimit)
{
    cdmServiceWillGetLdlSessionsLimit();
    sendGetLdlSessionsLimitRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToGetLdlSessionsLimit)
{
    cdmServiceWillFailToGetLdlSessionsLimit();
    sendGetLdlSessionsLimitRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldGetLastDrmError)
{
    cdmServiceWillGetLastDrmError();
    sendGetLastDrmErrorRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToGetLastDrmError)
{
    cdmServiceWillFailToGetLastDrmError();
    sendGetLastDrmErrorRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldGetDrmTime)
{
    cdmServiceWillGetDrmTime();
    sendGetDrmTimeRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToGetDrmTime)
{
    cdmServiceWillFailToGetDrmTime();
    sendGetDrmTimeRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldReleaseKeySession)
{
    cdmServiceWillReleaseKeySession();
    sendReleaseKeySessionRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToReleaseKeySession)
{
    cdmServiceWillFailToReleaseKeySession();
    sendReleaseKeySessionRequestAndReceiveErrorResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldSendLicenseRequest)
{
    mediaClientWillSendLicenseRequestEvent();
    sendLicenseRequestEvent();
}

TEST_F(MediaKeysModuleServiceTests, shouldSendLicenseRenewal)
{
    mediaClientWillSendLicenseRenewalEvent();
    sendLicenseRenewalEvent();
}

TEST_F(MediaKeysModuleServiceTests, shouldSendKeyStatusChanged)
{
    mediaClientWillSendKeyStatusesChangedEvent();
    sendKeyStatusesChangedEvent();
}

TEST_F(MediaKeysModuleServiceTests, FactoryCreatesObject)
{
    testFactoryCreatesObject();
}

TEST_F(MediaKeysModuleServiceTests, shouldGetMetricSystemData)
{
    cdmServiceWillGetMetricSystemData();
    sendGetMetricSystemDataRequestAndReceiveResponse();
}

TEST_F(MediaKeysModuleServiceTests, shouldFailToGetMetricSystemData)
{
    cdmServiceWillFailToGetMetricSystemData();
    sendGetMetricSystemDataRequestAndReceiveErrorResponse();
}