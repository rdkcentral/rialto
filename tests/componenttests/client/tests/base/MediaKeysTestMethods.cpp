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
const std::vector<uint8_t> kLicenseResponse{0x4D, 0x79, 0x4C, 0x69, 0x63, 0x65, 0x6E, 0x73,
                                            0x65, 0x44, 0x61, 0x74, 0x61, 0x3A, 0x20, 0x31};
const std::string kUrl{"www.licenseServer.com"};
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
    EXPECT_EQ(m_mediaKeys->createKeySession(kSessionTypeTemp, m_mediaKeysClientMock, kIsNotLdl, keySessionId), kStatusFailed);
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

void MediaKeysTestMethods::initaliseMediaKeySession()
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

void MediaKeysTestMethods::terminateMediaKeySession()
{
    // Close session
    MediaKeysTestMethods::shouldCloseKeySession();
    MediaKeysTestMethods::closeKeySession();

    // Destroy media keys
    MediaKeysTestMethods::shouldDestroyMediaKeys();
    MediaKeysTestMethods::destroyMediaKeys();
}

} // namespace firebolt::rialto::client::ct
