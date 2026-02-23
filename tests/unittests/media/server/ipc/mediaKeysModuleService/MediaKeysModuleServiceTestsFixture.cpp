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
#include "MediaKeysClient.h"
#include "MediaKeysModuleService.h"
#include "MediaKeysProtoUtils.h"
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SaveArg;
using testing::SetArgReferee;

namespace
{
const std::string keySystem{"expectedKeySystem"};
constexpr int kHardcodedMediaKeysHandle{2};
constexpr firebolt::rialto::KeySessionType kKeySessionType{firebolt::rialto::KeySessionType::TEMPORARY};
constexpr int kKeySessionId{3};
constexpr firebolt::rialto::MediaKeyErrorStatus kErrorStatus{firebolt::rialto::MediaKeyErrorStatus::FAIL};
constexpr firebolt::rialto::InitDataType kInitDataType{firebolt::rialto::InitDataType::CENC};
const std::vector<std::uint8_t> kInitData{6, 7, 2};
const std::vector<std::uint8_t> kResponseData{9, 7, 8};
const std::vector<uint8_t> kKeyId{1, 2, 3};
const std::vector<uint8_t> kDrmHeader{6, 3, 8};
const std::vector<unsigned char> kLicenseRequestMessage{3, 2, 1};
const std::vector<unsigned char> kLicenseRenewalMessage{0, 4, 8};
const std::string kUrl{"http://"};
constexpr firebolt::rialto::LimitedDurationLicense kLdlState{firebolt::rialto::LimitedDurationLicense::NOT_SPECIFIED};
} // namespace

MATCHER_P4(LicenseRequestEventMatcher, kKeySessionId, mediaKeysHandle, requestMessage, kUrl, "")
{
    std::shared_ptr<firebolt::rialto::LicenseRequestEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::LicenseRequestEvent>(arg);
    std::vector<unsigned char> messageVector =
        std::vector<unsigned char>{event->license_request_message().begin(), event->license_request_message().end()};
    return ((kKeySessionId == event->key_session_id()) && (mediaKeysHandle == event->media_keys_handle()) &&
            (requestMessage == messageVector) && (kUrl == event->url()));
}

MATCHER_P3(LicenseRenewalEventMatcher, kKeySessionId, mediaKeysHandle, renewalMessage, "")
{
    std::shared_ptr<firebolt::rialto::LicenseRenewalEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::LicenseRenewalEvent>(arg);
    std::vector<unsigned char> messageVector =
        std::vector<unsigned char>{event->license_renewal_message().begin(), event->license_renewal_message().end()};
    return ((kKeySessionId == event->key_session_id()) && (mediaKeysHandle == event->media_keys_handle()) &&
            (renewalMessage == messageVector));
}

MATCHER_P3(KeyStatusesChangedEventMatcher, kKeySessionId, mediaKeysHandle, keyStatuses, "")
{
    std::shared_ptr<firebolt::rialto::KeyStatusesChangedEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::KeyStatusesChangedEvent>(arg);

    firebolt::rialto::KeyStatusVector actualKeyStatuses;
    for (auto it = event->key_statuses().begin(); it != event->key_statuses().end(); it++)
    {
        std::vector<unsigned char> keyVector = std::vector<unsigned char>{it->key_id().begin(), it->key_id().end()};
        firebolt::rialto::KeyStatus keyStatus = convertKeyStatus(it->key_status());
        actualKeyStatuses.push_back(std::make_pair(keyVector, keyStatus));
    }

    return ((kKeySessionId == event->key_session_id()) && (mediaKeysHandle == event->media_keys_handle()) &&
            (keyStatuses == actualKeyStatuses));
}

MediaKeysModuleServiceTests::MediaKeysModuleServiceTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_serverMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ServerMock>>()},
      m_closureMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClosureMock>>()},
      m_controllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ControllerMock>>()},
      m_invalidControllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::RpcControllerMock>>()}
{
    m_service = std::make_shared<firebolt::rialto::server::ipc::MediaKeysModuleService>(m_cdmServiceMock);
    m_client = std::make_shared<firebolt::rialto::server::ipc::MediaKeysClient>(kHardcodedMediaKeysHandle, m_clientMock);
}

MediaKeysModuleServiceTests::~MediaKeysModuleServiceTests() {}

void MediaKeysModuleServiceTests::clientWillConnect()
{
    EXPECT_CALL(*m_clientMock, exportService(_));
}

void MediaKeysModuleServiceTests::clientWillDisconnect(int sessionId)
{
    EXPECT_CALL(m_cdmServiceMock, destroyMediaKeys(sessionId));
}

void MediaKeysModuleServiceTests::cdmServiceWillCreateMediaKeys()
{
    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_cdmServiceMock, createMediaKeys(_, keySystem)).WillOnce(Return(true));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToCreateMediaKeys()
{
    expectRequestFailure();
    EXPECT_CALL(m_cdmServiceMock, createMediaKeys(_, keySystem)).WillOnce(Return(false));
}

void MediaKeysModuleServiceTests::cdmServiceWillDestroyMediaKeys()
{
    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_cdmServiceMock, destroyMediaKeys(kHardcodedMediaKeysHandle)).WillOnce(Return(true));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToDestroyMediaKeys()
{
    expectRequestFailure();
    EXPECT_CALL(m_cdmServiceMock, destroyMediaKeys(kHardcodedMediaKeysHandle)).WillOnce(Return(false));
}

void MediaKeysModuleServiceTests::cdmServiceWillCreateKeySession()
{
    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_cdmServiceMock, createKeySession(kHardcodedMediaKeysHandle, kKeySessionType, _, _))
        .WillOnce(DoAll(SetArgReferee<3>(kKeySessionId), Return(firebolt::rialto::MediaKeyErrorStatus::OK)));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToCreateKeySession()
{
    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_cdmServiceMock, createKeySession(kHardcodedMediaKeysHandle, kKeySessionType, _, _))
        .WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillGenerateRequest()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock,
                generateRequest(kHardcodedMediaKeysHandle, kKeySessionId, kInitDataType, kInitData, kLdlState))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToGenerateRequest()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock,
                generateRequest(kHardcodedMediaKeysHandle, kKeySessionId, kInitDataType, kInitData, kLdlState))
        .WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillLoadSession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, loadSession(kHardcodedMediaKeysHandle, kKeySessionId))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToLoadSession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, loadSession(kHardcodedMediaKeysHandle, kKeySessionId)).WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillUpdateSession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, updateSession(kHardcodedMediaKeysHandle, kKeySessionId, kResponseData))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToUpdateSession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, updateSession(kHardcodedMediaKeysHandle, kKeySessionId, kResponseData))
        .WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillCloseKeySession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, closeKeySession(kHardcodedMediaKeysHandle, kKeySessionId))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToCloseKeySession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, closeKeySession(kHardcodedMediaKeysHandle, kKeySessionId)).WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillRemoveKeySession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, removeKeySession(kHardcodedMediaKeysHandle, kKeySessionId))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToRemoveKeySession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, removeKeySession(kHardcodedMediaKeysHandle, kKeySessionId)).WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillGetCdmKeySessionId()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getCdmKeySessionId(kHardcodedMediaKeysHandle, kKeySessionId, _))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToGetCdmKeySessionId()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getCdmKeySessionId(kHardcodedMediaKeysHandle, kKeySessionId, _))
        .WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillGetExistingKey()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, containsKey(kHardcodedMediaKeysHandle, kKeySessionId, kKeyId)).WillOnce(Return(true));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToGetExistingKey()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, containsKey(kHardcodedMediaKeysHandle, kKeySessionId, kKeyId)).WillOnce(Return(false));
}

void MediaKeysModuleServiceTests::cdmServiceWillSetDrmHeader()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, setDrmHeader(kHardcodedMediaKeysHandle, kKeySessionId, kDrmHeader))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToSetDrmHeader()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, setDrmHeader(kHardcodedMediaKeysHandle, kKeySessionId, kDrmHeader))
        .WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillDeleteDrmStore()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, deleteDrmStore(kHardcodedMediaKeysHandle))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToDeleteDrmStore()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, deleteDrmStore(kHardcodedMediaKeysHandle)).WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillDeleteKeyStore()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, deleteKeyStore(kHardcodedMediaKeysHandle))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToDeleteKeyStore()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, deleteKeyStore(kHardcodedMediaKeysHandle)).WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillGetDrmStoreHash()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getDrmStoreHash(kHardcodedMediaKeysHandle, _))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToGetDrmStoreHash()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getDrmStoreHash(kHardcodedMediaKeysHandle, _)).WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillGetKeyStoreHash()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getKeyStoreHash(kHardcodedMediaKeysHandle, _))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToGetKeyStoreHash()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getKeyStoreHash(kHardcodedMediaKeysHandle, _)).WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillGetLdlSessionsLimit()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getLdlSessionsLimit(kHardcodedMediaKeysHandle, _))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToGetLdlSessionsLimit()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getLdlSessionsLimit(kHardcodedMediaKeysHandle, _)).WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillGetLastDrmError()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getLastDrmError(kHardcodedMediaKeysHandle, kKeySessionId, _))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToGetLastDrmError()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getLastDrmError(kHardcodedMediaKeysHandle, kKeySessionId, _))
        .WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillGetDrmTime()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getDrmTime(kHardcodedMediaKeysHandle, _))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToGetDrmTime()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getDrmTime(kHardcodedMediaKeysHandle, _)).WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillReleaseKeySession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, releaseKeySession(kHardcodedMediaKeysHandle, kKeySessionId))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToReleaseKeySession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, releaseKeySession(kHardcodedMediaKeysHandle, kKeySessionId)).WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::cdmServiceWillGetMetricSystemData()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getMetricSystemData(kHardcodedMediaKeysHandle, _))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
}

void MediaKeysModuleServiceTests::cdmServiceWillFailToGetMetricSystemData()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getMetricSystemData(kHardcodedMediaKeysHandle, _)).WillOnce(Return(kErrorStatus));
}

void MediaKeysModuleServiceTests::mediaClientWillSendLicenseRequestEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(LicenseRequestEventMatcher(kKeySessionId, kHardcodedMediaKeysHandle,
                                                                    kLicenseRequestMessage, kUrl)));
}

void MediaKeysModuleServiceTests::mediaClientWillSendLicenseRenewalEvent()
{
    EXPECT_CALL(*m_clientMock,
                sendEvent(LicenseRenewalEventMatcher(kKeySessionId, kHardcodedMediaKeysHandle, kLicenseRenewalMessage)));
}

void MediaKeysModuleServiceTests::mediaClientWillSendKeyStatusesChangedEvent()
{
    createKeyStatusVector();
    EXPECT_CALL(*m_clientMock,
                sendEvent(KeyStatusesChangedEventMatcher(kKeySessionId, kHardcodedMediaKeysHandle, m_keyStatuses)));
}

void MediaKeysModuleServiceTests::createKeyStatusVector()
{
    std::vector<firebolt::rialto::KeyStatus> possibleKeyStatuses = {firebolt::rialto::KeyStatus::USABLE,
                                                                    firebolt::rialto::KeyStatus::EXPIRED,
                                                                    firebolt::rialto::KeyStatus::OUTPUT_RESTRICTED};
    for (uint32_t i = 0; i < 3; i++)
    {
        std::vector<unsigned char> keyVector = std::vector<unsigned char>{(unsigned char)i};
        m_keyStatuses.push_back(std::make_pair(keyVector, possibleKeyStatuses[i % possibleKeyStatuses.size()]));
    }
}

void MediaKeysModuleServiceTests::sendClientConnected()
{
    m_service->clientConnected(m_clientMock);
}

void MediaKeysModuleServiceTests::sendClientDisconnected()
{
    m_service->clientDisconnected(m_clientMock);
}

int MediaKeysModuleServiceTests::sendCreateMediaKeysRequestAndReceiveResponse()
{
    firebolt::rialto::CreateMediaKeysRequest request;
    firebolt::rialto::CreateMediaKeysResponse response;

    // Set an invalid sessionId in the response
    response.set_media_keys_handle(-1);

    request.set_key_system(keySystem);

    m_service->createMediaKeys(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_GE(response.media_keys_handle(), 0);

    return response.media_keys_handle();
}

void MediaKeysModuleServiceTests::sendCreateMediaKeysRequestWithInvalidIpcAndReceiveFailedResponse()
{
    firebolt::rialto::CreateMediaKeysRequest request;
    firebolt::rialto::CreateMediaKeysResponse response;

    request.set_key_system(keySystem);

    m_service->createMediaKeys(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaKeysModuleServiceTests::sendCreateMediaKeysRequestAndExpectFailure()
{
    firebolt::rialto::CreateMediaKeysRequest request;
    firebolt::rialto::CreateMediaKeysResponse response;

    request.set_key_system(keySystem);

    m_service->createMediaKeys(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaKeysModuleServiceTests::sendDestroyMediaKeysRequestAndReceiveResponse()
{
    firebolt::rialto::DestroyMediaKeysRequest request;
    firebolt::rialto::DestroyMediaKeysResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->destroyMediaKeys(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaKeysModuleServiceTests::sendDestroyMediaKeysRequestWithInvalidIpcAndReceiveFailedResponse()
{
    firebolt::rialto::DestroyMediaKeysRequest request;
    firebolt::rialto::DestroyMediaKeysResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->destroyMediaKeys(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaKeysModuleServiceTests::sendCreateKeySessionRequestAndReceiveResponse()
{
    firebolt::rialto::CreateKeySessionRequest request;
    firebolt::rialto::CreateKeySessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_session_type(convertKeySessionType(kKeySessionType));

    m_service->createKeySession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_GE(response.key_session_id(), -1);
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendCreateKeySessionRequestAndReceiveErrorResponse()
{
    firebolt::rialto::CreateKeySessionRequest request;
    firebolt::rialto::CreateKeySessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_session_type(convertKeySessionType(kKeySessionType));

    m_service->createKeySession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_GE(response.key_session_id(), -1);
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendCreateKeySessionRequestWithInvalidIpcAndReceiveFailedResponse()
{
    firebolt::rialto::CreateKeySessionRequest request;
    firebolt::rialto::CreateKeySessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_session_type(convertKeySessionType(kKeySessionType));

    m_service->createKeySession(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaKeysModuleServiceTests::sendGenerateRequestRequestAndReceiveResponse()
{
    firebolt::rialto::GenerateRequestRequest request;
    firebolt::rialto::GenerateRequestResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);
    request.set_init_data_type(convertInitDataType(kInitDataType));
    request.set_ldl_state(convertLimitedDurationLicense(kLdlState));

    for (auto it = kInitData.begin(); it != kInitData.end(); it++)
    {
        request.add_init_data(*it);
    }

    m_service->generateRequest(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGenerateRequestRequestAndReceiveErrorResponse()
{
    firebolt::rialto::GenerateRequestRequest request;
    firebolt::rialto::GenerateRequestResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);
    request.set_init_data_type(convertInitDataType(kInitDataType));
    request.set_ldl_state(convertLimitedDurationLicense(kLdlState));

    for (auto it = kInitData.begin(); it != kInitData.end(); it++)
    {
        request.add_init_data(*it);
    }

    m_service->generateRequest(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendLoadSessionRequestAndReceiveResponse()
{
    firebolt::rialto::LoadSessionRequest request;
    firebolt::rialto::LoadSessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->loadSession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendLoadSessionRequestAndReceiveErrorResponse()
{
    firebolt::rialto::LoadSessionRequest request;
    firebolt::rialto::LoadSessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->loadSession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendUpdateSessionRequestAndReceiveResponse()
{
    firebolt::rialto::UpdateSessionRequest request;
    firebolt::rialto::UpdateSessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    for (auto it = kResponseData.begin(); it != kResponseData.end(); it++)
    {
        request.add_response_data(*it);
    }

    m_service->updateSession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendUpdateSessionRequestAndReceiveErrorResponse()
{
    firebolt::rialto::UpdateSessionRequest request;
    firebolt::rialto::UpdateSessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    for (auto it = kResponseData.begin(); it != kResponseData.end(); it++)
    {
        request.add_response_data(*it);
    }

    m_service->updateSession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendCloseKeySessionRequestAndReceiveResponse()
{
    firebolt::rialto::CloseKeySessionRequest request;
    firebolt::rialto::CloseKeySessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->closeKeySession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendCloseKeySessionRequestAndReceiveErrorResponse()
{
    firebolt::rialto::CloseKeySessionRequest request;
    firebolt::rialto::CloseKeySessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->closeKeySession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendRemoveKeySessionRequestAndReceiveResponse()
{
    firebolt::rialto::RemoveKeySessionRequest request;
    firebolt::rialto::RemoveKeySessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->removeKeySession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendRemoveKeySessionRequestAndReceiveErrorResponse()
{
    firebolt::rialto::RemoveKeySessionRequest request;
    firebolt::rialto::RemoveKeySessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->removeKeySession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetCdmKeySessionIdRequestAndReceiveResponse()
{
    firebolt::rialto::GetCdmKeySessionIdRequest request;
    firebolt::rialto::GetCdmKeySessionIdResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->getCdmKeySessionId(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetCdmKeySessionIdRequestAndReceiveErrorResponse()
{
    firebolt::rialto::GetCdmKeySessionIdRequest request;
    firebolt::rialto::GetCdmKeySessionIdResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->getCdmKeySessionId(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendContainsKeyRequestAndReceiveResponse()
{
    firebolt::rialto::ContainsKeyRequest request;
    firebolt::rialto::ContainsKeyResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);
    for (const auto &item : kKeyId)
    {
        request.add_key_id(item);
    }

    m_service->containsKey(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_TRUE(response.contains_key());
}

void MediaKeysModuleServiceTests::sendContainsKeyRequestAndReceiveErrorResponse()
{
    firebolt::rialto::ContainsKeyRequest request;
    firebolt::rialto::ContainsKeyResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);
    for (const auto &item : kKeyId)
    {
        request.add_key_id(item);
    }

    m_service->containsKey(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_FALSE(response.contains_key());
}

void MediaKeysModuleServiceTests::sendSetDrmHeaderRequestAndReceiveResponse()
{
    firebolt::rialto::SetDrmHeaderRequest request;
    firebolt::rialto::SetDrmHeaderResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);
    for (const auto &item : kDrmHeader)
    {
        request.add_request_data(item);
    }

    m_service->setDrmHeader(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendSetDrmHeaderRequestAndReceiveErrorResponse()
{
    firebolt::rialto::SetDrmHeaderRequest request;
    firebolt::rialto::SetDrmHeaderResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);
    for (const auto &item : kDrmHeader)
    {
        request.add_request_data(item);
    }

    m_service->setDrmHeader(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendDeleteDrmStoreRequestAndReceiveResponse()
{
    firebolt::rialto::DeleteDrmStoreRequest request;
    firebolt::rialto::DeleteDrmStoreResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->deleteDrmStore(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendDeleteDrmStoreRequestAndReceiveErrorResponse()
{
    firebolt::rialto::DeleteDrmStoreRequest request;
    firebolt::rialto::DeleteDrmStoreResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->deleteDrmStore(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendDeleteKeyStoreRequestAndReceiveResponse()
{
    firebolt::rialto::DeleteKeyStoreRequest request;
    firebolt::rialto::DeleteKeyStoreResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->deleteKeyStore(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendDeleteKeyStoreRequestAndReceiveErrorResponse()
{
    firebolt::rialto::DeleteKeyStoreRequest request;
    firebolt::rialto::DeleteKeyStoreResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->deleteKeyStore(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetDrmStoreHashRequestAndReceiveResponse()
{
    firebolt::rialto::GetDrmStoreHashRequest request;
    firebolt::rialto::GetDrmStoreHashResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->getDrmStoreHash(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetDrmStoreHashRequestAndReceiveErrorResponse()
{
    firebolt::rialto::GetDrmStoreHashRequest request;
    firebolt::rialto::GetDrmStoreHashResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->getDrmStoreHash(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetKeyStoreHashRequestAndReceiveResponse()
{
    firebolt::rialto::GetKeyStoreHashRequest request;
    firebolt::rialto::GetKeyStoreHashResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->getKeyStoreHash(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetKeyStoreHashRequestAndReceiveErrorResponse()
{
    firebolt::rialto::GetKeyStoreHashRequest request;
    firebolt::rialto::GetKeyStoreHashResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->getKeyStoreHash(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetLdlSessionsLimitRequestAndReceiveResponse()
{
    firebolt::rialto::GetLdlSessionsLimitRequest request;
    firebolt::rialto::GetLdlSessionsLimitResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->getLdlSessionsLimit(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetLdlSessionsLimitRequestAndReceiveErrorResponse()
{
    firebolt::rialto::GetLdlSessionsLimitRequest request;
    firebolt::rialto::GetLdlSessionsLimitResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->getLdlSessionsLimit(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetLastDrmErrorRequestAndReceiveResponse()
{
    firebolt::rialto::GetLastDrmErrorRequest request;
    firebolt::rialto::GetLastDrmErrorResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->getLastDrmError(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetLastDrmErrorRequestAndReceiveErrorResponse()
{
    firebolt::rialto::GetLastDrmErrorRequest request;
    firebolt::rialto::GetLastDrmErrorResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->getLastDrmError(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetDrmTimeRequestAndReceiveResponse()
{
    firebolt::rialto::GetDrmTimeRequest request;
    firebolt::rialto::GetDrmTimeResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->getDrmTime(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetDrmTimeRequestAndReceiveErrorResponse()
{
    firebolt::rialto::GetDrmTimeRequest request;
    firebolt::rialto::GetDrmTimeResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->getDrmTime(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendReleaseKeySessionRequestAndReceiveResponse()
{
    firebolt::rialto::ReleaseKeySessionRequest request;
    firebolt::rialto::ReleaseKeySessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->releaseKeySession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendReleaseKeySessionRequestAndReceiveErrorResponse()
{
    firebolt::rialto::ReleaseKeySessionRequest request;
    firebolt::rialto::ReleaseKeySessionResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);
    request.set_key_session_id(kKeySessionId);

    m_service->releaseKeySession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendLicenseRequestEvent()
{
    m_client->onLicenseRequest(kKeySessionId, kLicenseRequestMessage, kUrl);
}

void MediaKeysModuleServiceTests::sendLicenseRenewalEvent()
{
    m_client->onLicenseRenewal(kKeySessionId, kLicenseRenewalMessage);
}

void MediaKeysModuleServiceTests::sendKeyStatusesChangedEvent()
{
    m_client->onKeyStatusesChanged(kKeySessionId, m_keyStatuses);
}

void MediaKeysModuleServiceTests::expectRequestSuccess()
{
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaKeysModuleServiceTests::expectRequestFailure()
{
    EXPECT_CALL(*m_controllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaKeysModuleServiceTests::expectInvalidControllerRequestFailure()
{
    EXPECT_CALL(*m_invalidControllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaKeysModuleServiceTests::testFactoryCreatesObject()
{
    std::shared_ptr<firebolt::rialto::server::ipc::IMediaKeysModuleServiceFactory> factory =
        firebolt::rialto::server::ipc::IMediaKeysModuleServiceFactory::createFactory();
    EXPECT_NE(factory, nullptr);
    EXPECT_NE(factory->create(m_cdmServiceMock), nullptr);
}

void MediaKeysModuleServiceTests::sendGetMetricSystemDataRequestAndReceiveResponse()
{
    firebolt::rialto::GetMetricSystemDataRequest request;
    firebolt::rialto::GetMetricSystemDataResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->getMetricSystemData(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK, convertMediaKeyErrorStatus(response.error_status()));
}

void MediaKeysModuleServiceTests::sendGetMetricSystemDataRequestAndReceiveErrorResponse()
{
    firebolt::rialto::GetMetricSystemDataRequest request;
    firebolt::rialto::GetMetricSystemDataResponse response;

    request.set_media_keys_handle(kHardcodedMediaKeysHandle);

    m_service->getMetricSystemData(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(kErrorStatus, convertMediaKeyErrorStatus(response.error_status()));
}
