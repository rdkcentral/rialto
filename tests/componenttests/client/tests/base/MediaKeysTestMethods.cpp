/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "MediaKeysTestMethods.h"
#include "CommonConstants.h"
#include "MediaKeysProtoRequestMatchers.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
const std::string kKeySystemWidevine{"com.widevine.alpha"};
const std::string kKeySystemPlayready{"com.microsoft.playready"};
constexpr int32_t kMediaKeysHandle{1};
constexpr firebolt::rialto::KeySessionType kSessionTypeTemp{firebolt::rialto::KeySessionType::TEMPORARY};
constexpr bool kIsNotLdl{false};
constexpr firebolt::rialto::MediaKeyErrorStatus kStatusOk{firebolt::rialto::MediaKeyErrorStatus::OK};
constexpr firebolt::rialto::MediaKeyErrorStatus kStatusFailed{firebolt::rialto::MediaKeyErrorStatus::FAIL};
constexpr firebolt::rialto::InitDataType kInitDataTypeCenc{firebolt::rialto::InitDataType::CENC};
const std::vector<unsigned char> kLicenseRequestMessage{'r', 'e', 'q', 'u', 'e', 's', 't'};
const std::vector<unsigned char> kLicensRenewalMessage{'r', 'e', 'n', 'e', 'w', 'a', 'l'};
const std::vector<uint8_t> kLicenseResponse{0x4D, 0x79, 0x4C, 0x69, 0x63, 0x65, 0x6E, 0x73,
                                            0x65, 0x44, 0x61, 0x74, 0x61, 0x3A, 0x20, 0x31};
const std::vector<uint8_t> kLicenseRenewalResponse{0x1A, 0xB3, 0x7F, 0x8E, 0xD4, 0x60, 0x2F, 0x91,
                                                   0x66, 0x9E, 0x3E, 0xA7, 0xD2, 0x81, 0x4F, 0xC2};
const std::vector<uint8_t> kInvalidKeyId{0x45, 0x9F, 0x27, 0xF8, 0x14, 0xC2, 0x7B, 0xE9,
                                         0x5A, 0x36, 0x3E, 0xA7, 0x9E, 0x73, 0xC8, 0xC2};
const std::vector<unsigned char> kKeyStoreHash{'k', 'e', 'y', 'h', 'a', 's', 'h'};
const std::vector<unsigned char> kDrmStoreHash{'d', 'r', 'm', 'h', 'a', 's', 'h'};
const std::vector<uint8_t> kDrmHeader{0x91, 0x2E, 0x5D, 0xF3, 0x77, 0xA4, 0x4B, 0xD6,
                                      0x7A, 0x3D, 0xB8, 0x56, 0x1F, 0xE2, 0x89, 0xC4};
const std::vector<uint8_t> kDrmHeader2{0xAE, 0x64, 0x1B, 0xF8, 0x35, 0x97, 0x50, 0x2C,
                                       0x8F, 0x42, 0x71, 0xE5, 0x2B, 0x9C, 0x5E, 0x13};
const std::string kUrl{"www.licenseServer.com"};
const std::string kCdmKeySessionId{"CDM ID"};
constexpr uint32_t kErrorCode{98538};
constexpr uint32_t kLdlSessionLimit{5};
constexpr uint64_t kDrmTime{1704200814000000000};
} // namespace

namespace firebolt::rialto::client::ct
{
MediaKeysTestMethods::MediaKeysTestMethods()
    : m_mediaKeysClientMock{std::make_shared<StrictMock<MediaKeysClientMock>>()},
      m_mediaKeysModuleMock{std::make_shared<StrictMock<MediaKeysModuleMock>>()}
{
}

MediaKeysTestMethods::~MediaKeysTestMethods() {}

void MediaKeysTestMethods::shouldCreateMediaKeysWidevine()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, createMediaKeys(_, createMediaKeysRequestMatcher(kKeySystemWidevine), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->createMediaKeysResponse(kMediaKeysHandle)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::createMediaKeysWidevine()
{
    m_mediaKeysFactory = firebolt::rialto::IMediaKeysFactory::createFactory();
    m_mediaKeys = m_mediaKeysFactory->createMediaKeys(kKeySystemWidevine);
    EXPECT_NE(m_mediaKeys, nullptr);
}

void MediaKeysTestMethods::shouldCreateMediaKeysPlayready()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, createMediaKeys(_, createMediaKeysRequestMatcher(kKeySystemPlayready), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->createMediaKeysResponse(kMediaKeysHandle)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::createMediaKeysPlayready()
{
    m_mediaKeysFactory = firebolt::rialto::IMediaKeysFactory::createFactory();
    m_mediaKeys = m_mediaKeysFactory->createMediaKeys(kKeySystemPlayready);
    EXPECT_NE(m_mediaKeys, nullptr);
}

void MediaKeysTestMethods::shouldCreateKeySession()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                createKeySession(_,
                                 createKeySessionRequestMatcher(kMediaKeysHandle,
                                                                convertKeySessionType(kSessionTypeTemp), kIsNotLdl),
                                 _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->createKeySessionResponse(kStatusOk, kKeySessionId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::shouldCreateKeySessionFailure()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                createKeySession(_,
                                 createKeySessionRequestMatcher(kMediaKeysHandle,
                                                                convertKeySessionType(kSessionTypeTemp), kIsNotLdl),
                                 _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->createKeySessionResponse(kStatusFailed, kKeySessionId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::createKeySession()
{
    int32_t keySessionId;
    EXPECT_EQ(m_mediaKeys->createKeySession(kSessionTypeTemp, m_mediaKeysClientMock, kIsNotLdl, keySessionId), kStatusOk);
    EXPECT_EQ(keySessionId, kKeySessionId);
}

void MediaKeysTestMethods::createKeySessionFailure()
{
    int32_t keySessionId;
    EXPECT_EQ(m_mediaKeys->createKeySession(kSessionTypeTemp, m_mediaKeysClientMock, kIsNotLdl, keySessionId),
              kStatusFailed);
}

void MediaKeysTestMethods::shouldGenerateRequest()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                generateRequest(_,
                                generateRequestRequestMatcher(kMediaKeysHandle, kKeySessionId,
                                                              convertInitDataType(kInitDataTypeCenc), kInitData),
                                _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->generateRequestResponse(kStatusOk)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::shouldGenerateRequestFailure()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                generateRequest(_,
                                generateRequestRequestMatcher(kMediaKeysHandle, kKeySessionId,
                                                              convertInitDataType(kInitDataTypeCenc), kInitData),
                                _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->generateRequestResponse(kStatusFailed)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::shouldGenerateRequestAndSendNotifyLicenseRequest()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                generateRequest(_,
                                generateRequestRequestMatcher(kMediaKeysHandle, kKeySessionId,
                                                              convertInitDataType(kInitDataTypeCenc), kInitData),
                                _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->generateRequestResponse(kStatusOk)),
                        WithArgs<0, 3>(Invoke(
                            [this](::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
                            {
                                getServerStub()->notifyLicenseRequestEvent(kMediaKeysHandle, kKeySessionId,
                                                                           kLicenseRequestMessage, kUrl);
                                m_mediaKeysModuleMock->defaultReturn(controller, done);
                            }))));
}

void MediaKeysTestMethods::generateRequest()
{
    EXPECT_EQ(m_mediaKeys->generateRequest(kKeySessionId, kInitDataTypeCenc, kInitData), kStatusOk);
}

void MediaKeysTestMethods::generateRequestFailure()
{
    EXPECT_EQ(m_mediaKeys->generateRequest(kKeySessionId, kInitDataTypeCenc, kInitData), kStatusFailed);
}

void MediaKeysTestMethods::shouldNotifyLicenseRequest()
{
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRequest(kKeySessionId, kLicenseRequestMessage, kUrl))
        .WillOnce(Invoke(this, &MediaKeysTestMethods::notifyEvent));
}

void MediaKeysTestMethods::sendNotifyLicenseRequest()
{
    getServerStub()->notifyLicenseRequestEvent(kMediaKeysHandle, kKeySessionId, kLicenseRequestMessage, kUrl);
    waitEvent();
}

void MediaKeysTestMethods::shouldNotifyKeyStatusesChanged()
{
    EXPECT_CALL(*m_mediaKeysClientMock, onKeyStatusesChanged(kKeySessionId, kKeyStatuses))
        .WillOnce(Invoke(this, &MediaKeysTestMethods::notifyEvent));
}

void MediaKeysTestMethods::sendNotifyKeyStatusesChanged()
{
    getServerStub()->notifyKeyStatusesChangedEvent(kMediaKeysHandle, kKeySessionId, kKeyStatuses);
    waitEvent();
}

void MediaKeysTestMethods::shouldUpdateSession()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                updateSession(_, updateSessionRequestMatcher(kMediaKeysHandle, kKeySessionId, kLicenseResponse), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->updateSessionResponse(kStatusOk)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::shouldUpdateSessionFailure()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                updateSession(_, updateSessionRequestMatcher(kMediaKeysHandle, kKeySessionId, kLicenseResponse), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->updateSessionResponse(kStatusFailed)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::updateSession()
{
    EXPECT_EQ(m_mediaKeys->updateSession(kKeySessionId, kLicenseResponse), kStatusOk);
}

void MediaKeysTestMethods::updateSessionFailure()
{
    EXPECT_EQ(m_mediaKeys->updateSession(kKeySessionId, kLicenseResponse), kStatusFailed);
}

void MediaKeysTestMethods::shouldUpdateSessionRenewal()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                updateSession(_, updateSessionRequestMatcher(kMediaKeysHandle, kKeySessionId, kLicenseRenewalResponse),
                              _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->updateSessionResponse(kStatusOk)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::updateSessionRenewal()
{
    EXPECT_EQ(m_mediaKeys->updateSession(kKeySessionId, kLicenseRenewalResponse), kStatusOk);
}

void MediaKeysTestMethods::shouldCloseKeySession()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                closeKeySession(_, closeKeySessionRequestMatcher(kMediaKeysHandle, kKeySessionId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->closeKeySessionResponse(kStatusOk)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::shouldCloseKeySessionFailure()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                closeKeySession(_, closeKeySessionRequestMatcher(kMediaKeysHandle, kKeySessionId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->closeKeySessionResponse(kStatusFailed)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::closeKeySession()
{
    EXPECT_EQ(m_mediaKeys->closeKeySession(kKeySessionId), kStatusOk);
}

void MediaKeysTestMethods::closeKeySessionFailure()
{
    EXPECT_EQ(m_mediaKeys->closeKeySession(kKeySessionId), kStatusFailed);
}

void MediaKeysTestMethods::shouldDestroyMediaKeys()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, destroyMediaKeys(_, destroyMediaKeysRequestMatcher(kMediaKeysHandle), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn)));
}

void MediaKeysTestMethods::destroyMediaKeys()
{
    m_mediaKeys.reset();
}

void MediaKeysTestMethods::initaliseWidevineMediaKeySession()
{
    // Create a new widevine media keys object
    MediaKeysTestMethods::shouldCreateMediaKeysWidevine();
    MediaKeysTestMethods::createMediaKeysWidevine();

    // Create new key session
    MediaKeysTestMethods::shouldCreateKeySession();
    MediaKeysTestMethods::createKeySession();

    // Generate license request
    MediaKeysTestMethods::shouldGenerateRequest();
    MediaKeysTestMethods::generateRequest();
    MediaKeysTestMethods::shouldNotifyLicenseRequest();
    MediaKeysTestMethods::sendNotifyLicenseRequest();

    // Update session
    MediaKeysTestMethods::shouldUpdateSession();
    MediaKeysTestMethods::updateSession();
    MediaKeysTestMethods::shouldNotifyKeyStatusesChanged();
    MediaKeysTestMethods::sendNotifyKeyStatusesChanged();
}

void MediaKeysTestMethods::initalisePlayreadyMediaKeySession()
{
    // Create a new playready media keys object
    MediaKeysTestMethods::shouldCreateMediaKeysPlayready();
    MediaKeysTestMethods::createMediaKeysPlayready();

    // Create new key session
    MediaKeysTestMethods::shouldCreateKeySession();
    MediaKeysTestMethods::createKeySession();

    // Generate license request
    MediaKeysTestMethods::shouldGenerateRequest();
    MediaKeysTestMethods::generateRequest();
    MediaKeysTestMethods::shouldNotifyLicenseRequest();
    MediaKeysTestMethods::sendNotifyLicenseRequest();

    // Update session
    MediaKeysTestMethods::shouldUpdateSession();
    MediaKeysTestMethods::updateSession();
    MediaKeysTestMethods::shouldNotifyKeyStatusesChanged();
    MediaKeysTestMethods::sendNotifyKeyStatusesChanged();
}

void MediaKeysTestMethods::terminateMediaKeySession()
{
    // Close session
    MediaKeysTestMethods::shouldCloseKeySession();
    MediaKeysTestMethods::closeKeySession();

    // Destroy media keys
    MediaKeysTestMethods::shouldDestroyMediaKeys();
    MediaKeysTestMethods::destroyMediaKeys();
}

void MediaKeysTestMethods::shouldNotifyLicenseRenewal()
{
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRenewal(kKeySessionId, kLicensRenewalMessage))
        .WillOnce(Invoke(this, &MediaKeysTestMethods::notifyEvent));
}

void MediaKeysTestMethods::sendNotifyLicenseRenewal()
{
    getServerStub()->notifyLicenseRenewal(kMediaKeysHandle, kKeySessionId, kLicensRenewalMessage);
    waitEvent();
}

void MediaKeysTestMethods::shouldLoadSession()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, loadSession(_, loadSessionRequestMatcher(kMediaKeysHandle, kKeySessionId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->loadSessionResponse(kStatusOk)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::loadSession()
{
    EXPECT_EQ(m_mediaKeys->loadSession(kKeySessionId), kStatusOk);
}

void MediaKeysTestMethods::shouldContainsKey()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                containsKey(_, containsKeyRequestMatcher(kMediaKeysHandle, kKeySessionId, kKeyStatuses[0].first), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->containsKeyResponse(true)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::containsKey()
{
    EXPECT_EQ(m_mediaKeys->containsKey(kKeySessionId, kKeyStatuses[0].first), true);
}

void MediaKeysTestMethods::shouldNotContainKey()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                containsKey(_, containsKeyRequestMatcher(kMediaKeysHandle, kKeySessionId, kInvalidKeyId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->containsKeyResponse(false)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::doesNotContainKey()
{
    EXPECT_EQ(m_mediaKeys->containsKey(kKeySessionId, kInvalidKeyId), false);
}

void MediaKeysTestMethods::selectKeyId(const uint32_t keyIndex)
{
    EXPECT_EQ(m_mediaKeys->selectKeyId(kKeySessionId, kKeyStatuses[keyIndex].first), kStatusOk);
}

void MediaKeysTestMethods::shouldRemoveKeySession()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                removeKeySession(_, removeKeySessionRequestMatcher(kMediaKeysHandle, kKeySessionId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->removeKeySessionResponse(kStatusOk)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::removeKeySession()
{
    EXPECT_EQ(m_mediaKeys->removeKeySession(kKeySessionId), kStatusOk);
}

void MediaKeysTestMethods::shouldGetKeyStoreHash()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, getKeyStoreHash(_, getKeyStoreHashRequestMatcher(kMediaKeysHandle), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->getKeyStoreHashResponse(kStatusOk, kKeyStoreHash)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::getKeyStoreHash()
{
    std::vector<unsigned char> keyStoreHash;
    EXPECT_EQ(m_mediaKeys->getKeyStoreHash(keyStoreHash), kStatusOk);
    EXPECT_EQ(kKeyStoreHash, keyStoreHash);
}

void MediaKeysTestMethods::shouldGetDrmStoreHash()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, getDrmStoreHash(_, getDrmStoreHashRequestMatcher(kMediaKeysHandle), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->getDrmStoreHashResponse(kStatusOk, kDrmStoreHash)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::getDrmStoreHash()
{
    std::vector<unsigned char> drmStoreHash;
    EXPECT_EQ(m_mediaKeys->getDrmStoreHash(drmStoreHash), kStatusOk);
    EXPECT_EQ(kDrmStoreHash, drmStoreHash);
}

void MediaKeysTestMethods::shouldFailToGetKeyStoreHash()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, getKeyStoreHash(_, getKeyStoreHashRequestMatcher(kMediaKeysHandle), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->getKeyStoreHashResponse(kStatusFailed, {})),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::getKeyStoreHashFailure()
{
    std::vector<unsigned char> keyStoreHash;
    EXPECT_EQ(m_mediaKeys->getKeyStoreHash(keyStoreHash), kStatusFailed);
}

void MediaKeysTestMethods::shouldFailToGetDrmStoreHash()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, getDrmStoreHash(_, getDrmStoreHashRequestMatcher(kMediaKeysHandle), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->getDrmStoreHashResponse(kStatusFailed, {})),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::getDrmStoreHashFailure()
{
    std::vector<unsigned char> drmStoreHash;
    EXPECT_EQ(m_mediaKeys->getDrmStoreHash(drmStoreHash), kStatusFailed);
}

void MediaKeysTestMethods::shouldDeleteKeyStore()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, deleteKeyStore(_, deleteKeyStoreRequestMatcher(kMediaKeysHandle), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->deleteKeyStoreResponse(kStatusOk)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::deleteKeyStore()
{
    EXPECT_EQ(m_mediaKeys->deleteKeyStore(), kStatusOk);
}

void MediaKeysTestMethods::shouldDeleteDrmStore()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, deleteDrmStore(_, deleteDrmStoreRequestMatcher(kMediaKeysHandle), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->deleteDrmStoreResponse(kStatusOk)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::deleteDrmStore()
{
    EXPECT_EQ(m_mediaKeys->deleteDrmStore(), kStatusOk);
}

void MediaKeysTestMethods::shouldSetDrmHeader()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                setDrmHeader(_, setDrmHeaderRequestMatcher(kMediaKeysHandle, kKeySessionId, kDrmHeader), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->setDrmHeaderResponse(kStatusOk)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::setDrmHeader()
{
    EXPECT_EQ(m_mediaKeys->setDrmHeader(kKeySessionId, kDrmHeader), kStatusOk);
}

void MediaKeysTestMethods::shouldSetDrmHeaderSecond()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                setDrmHeader(_, setDrmHeaderRequestMatcher(kMediaKeysHandle, kKeySessionId, kDrmHeader2), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->setDrmHeaderResponse(kStatusOk)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::setDrmHeaderSecond()
{
    EXPECT_EQ(m_mediaKeys->setDrmHeader(kKeySessionId, kDrmHeader2), kStatusOk);
}

void MediaKeysTestMethods::shouldGetCdmKeySessionId()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                getCdmKeySessionId(_, getCdmKeySessionIdRequestMatcher(kMediaKeysHandle, kKeySessionId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->getCdmKeySessionIdResponse(kStatusOk, kCdmKeySessionId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::getCdmKeySessionId()
{
    std::string cdmKeySessionId;
    EXPECT_EQ(m_mediaKeys->getCdmKeySessionId(kKeySessionId, cdmKeySessionId), kStatusOk);
    EXPECT_EQ(kCdmKeySessionId, cdmKeySessionId);
}

void MediaKeysTestMethods::shouldGetLastDrmError()
{
    EXPECT_CALL(*m_mediaKeysModuleMock,
                getLastDrmError(_, getLastDrmErrorRequestMatcher(kMediaKeysHandle, kKeySessionId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->getLastDrmErrorResponse(kStatusOk, kErrorCode)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::getLastDrmError()
{
    uint32_t errorCode = 0;
    EXPECT_EQ(m_mediaKeys->getLastDrmError(kKeySessionId, errorCode), kStatusOk);
    EXPECT_EQ(kErrorCode, errorCode);
}

void MediaKeysTestMethods::shouldgetLdlSessionsLimit()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, getLdlSessionsLimit(_, getLdlSessionsLimitRequestMatcher(kMediaKeysHandle), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->getLdlSessionsLimitResponse(kStatusOk, kLdlSessionLimit)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::getLdlSessionsLimit()
{
    uint32_t ldlSessionLimit = 0;
    EXPECT_EQ(m_mediaKeys->getLdlSessionsLimit(ldlSessionLimit), kStatusOk);
    EXPECT_EQ(kLdlSessionLimit, ldlSessionLimit);
}

void MediaKeysTestMethods::shouldGetDrmTime()
{
    EXPECT_CALL(*m_mediaKeysModuleMock, getDrmTime(_, getDrmTimeRequestMatcher(kMediaKeysHandle), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaKeysModuleMock->getDrmTimeResponse(kStatusOk, kDrmTime)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaKeysModuleMock), &MediaKeysModuleMock::defaultReturn))));
}

void MediaKeysTestMethods::getDrmTime()
{
    uint64_t drmTime = 0;
    EXPECT_EQ(m_mediaKeys->getDrmTime(drmTime), kStatusOk);
    EXPECT_EQ(kDrmTime, drmTime);
}

} // namespace firebolt::rialto::client::ct
