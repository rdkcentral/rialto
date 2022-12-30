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

#include "CdmServiceTestsFixture.h"
#include <string>
#include <utility>

using testing::_;
using testing::ByMove;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgReferee;
using testing::Throw;

namespace
{
const std::vector<std::string> keySystems{"expectedKeySystem1", "expectedKeySystem2", "expectedKeySystem3"};
const std::string version{"123"};
constexpr int mediaKeysHandle{2};
constexpr firebolt::rialto::KeySessionType keySessionType{firebolt::rialto::KeySessionType::TEMPORARY};
constexpr bool isLDL{false};
constexpr int keySessionId{3};
constexpr firebolt::rialto::InitDataType initDataType{firebolt::rialto::InitDataType::CENC};
const std::vector<std::uint8_t> initData{6, 7, 2};
const std::vector<std::uint8_t> responseData{9, 7, 8};
const std::vector<uint8_t> keyId{1, 4, 7};
const std::vector<uint8_t> drmHeader{4, 9, 3};
const std::vector<unsigned char> licenseRequestMessage{3, 2, 1};
const std::vector<unsigned char> licenseRenewalMessage{0, 4, 8};
const std::string url{"http://"};
const uint32_t subSampleCount{2};
constexpr uint32_t initWithLast15{1};
} // namespace

CdmServiceTests::CdmServiceTests()
    : m_mediaKeysFactoryMock{std::make_shared<StrictMock<firebolt::rialto::server::MediaKeysServerInternalFactoryMock>>()},
      m_mediaKeys{std::make_unique<StrictMock<firebolt::rialto::server::MediaKeysServerInternalMock>>()},
      m_mediaKeysMock{dynamic_cast<StrictMock<firebolt::rialto::server::MediaKeysServerInternalMock> &>(*m_mediaKeys)},
      m_mediaKeysCapabilitiesFactoryMock{
          std::make_shared<StrictMock<firebolt::rialto::MediaKeysCapabilitiesFactoryMock>>()},
      m_mediaKeysCapabilities{std::make_shared<StrictMock<firebolt::rialto::MediaKeysCapabilitiesMock>>()},
      m_mediaKeysCapabilitiesMock{
          dynamic_cast<StrictMock<firebolt::rialto::MediaKeysCapabilitiesMock> &>(*m_mediaKeysCapabilities)},
      m_mediaKeysClientMock{std::make_shared<StrictMock<firebolt::rialto::MediaKeysClientMock>>()},
      m_sut{m_mediaKeysFactoryMock, m_mediaKeysCapabilitiesFactoryMock}
{
}

void CdmServiceTests::mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities()
{
    EXPECT_CALL(*m_mediaKeysCapabilitiesFactoryMock, getMediaKeysCapabilities()).WillOnce(Return(m_mediaKeysCapabilities));
}

void CdmServiceTests::mediaKeysCapabilitiesFactoryWillReturnNullptr()
{
    EXPECT_CALL(*m_mediaKeysCapabilitiesFactoryMock, getMediaKeysCapabilities()).WillOnce(Return(nullptr));
}

void CdmServiceTests::getSupportedKeySystemsWillReturnKeySystems()
{
    EXPECT_CALL(m_mediaKeysCapabilitiesMock, getSupportedKeySystems()).WillOnce(Return(keySystems));
}

void CdmServiceTests::supportsKeySystemWillReturnTrue()
{
    EXPECT_CALL(m_mediaKeysCapabilitiesMock, supportsKeySystem(keySystems[0])).WillOnce(Return(true));
}

void CdmServiceTests::getSupportedKeySystemVersionWillSucceed()
{
    EXPECT_CALL(m_mediaKeysCapabilitiesMock, getSupportedKeySystemVersion(keySystems[0], _))
        .WillOnce(DoAll(SetArgReferee<1>(version), Return(true)));
}

void CdmServiceTests::getSupportedKeySystemVersionWillFail()
{
    EXPECT_CALL(m_mediaKeysCapabilitiesMock, getSupportedKeySystemVersion(keySystems[0], _))
        .WillOnce(DoAll(SetArgReferee<1>(version), Return(false)));
}

void CdmServiceTests::triggerSwitchToActiveSuccess()
{
    EXPECT_TRUE(m_sut.switchToActive());
}

void CdmServiceTests::triggerSwitchToInactive()
{
    m_sut.switchToInactive();
}

void CdmServiceTests::mediaKeysFactoryWillCreateMediaKeys()
{
    EXPECT_CALL(*m_mediaKeysFactoryMock, createMediaKeysServerInternal(_)).WillOnce(Return(ByMove(std::move(m_mediaKeys))));
}

void CdmServiceTests::mediaKeysFactoryWillReturnNullptr()
{
    EXPECT_CALL(*m_mediaKeysFactoryMock, createMediaKeysServerInternal(_))
        .WillOnce(Return(ByMove(std::unique_ptr<firebolt::rialto::server::IMediaKeysServerInternal>())));
}

void CdmServiceTests::mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, createKeySession(keySessionType, _, isLDL, _))
        .WillOnce(DoAll(SetArgReferee<3>(keySessionId), Return(status)));
}

void CdmServiceTests::mediaKeysWillGenerateRequestWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, generateRequest(keySessionId, initDataType, initData)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillLoadSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, loadSession(keySessionId)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillUpdateSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, updateSession(keySessionId, responseData)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillCloseKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, closeKeySession(keySessionId)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillRemoveKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, removeKeySession(keySessionId)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillGetCdmKeySessionIdWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, getCdmKeySessionId(keySessionId, _)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillCheckIfKeyIsPresent(bool result)
{
    EXPECT_CALL(m_mediaKeysMock, containsKey(keySessionId, keyId)).WillOnce(Return(result));
}

void CdmServiceTests::mediaKeysWillSetDrmHeaderWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, setDrmHeader(keySessionId, drmHeader)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillDeleteDrmStoreWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, deleteDrmStore()).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillDeleteKeyStoreWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, deleteKeyStore()).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillGetDrmStoreHashWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, getDrmStoreHash(_)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillGetKeyStoreHashWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, getKeyStoreHash(_)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillGetLdlSessionsLimitWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, getLdlSessionsLimit(_)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillGetLastDrmErrorWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, getLastDrmError(keySessionId, _)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillGetDrmTimeWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, getDrmTime(_)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillDecryptWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(keySessionId)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaKeysMock, decrypt(keySessionId, _, _, subSampleCount, _, _, initWithLast15)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillSelectKeyIdWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(keySessionId)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaKeysMock, selectKeyId(keySessionId, keyId)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillNotFindMediaKeySession()
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(keySessionId)).WillOnce(Return(false));
}

void CdmServiceTests::mediaKeysWillCheckIfKeySystemIsNetflix(bool result)
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(keySessionId)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaKeysMock, isNetflixKeySystem(keySessionId)).WillOnce(Return(result));
}

void CdmServiceTests::createMediaKeysShouldSucceed()
{
    EXPECT_TRUE(m_sut.createMediaKeys(mediaKeysHandle, keySystems[0]));
}

void CdmServiceTests::createMediaKeysShouldFail()
{
    EXPECT_FALSE(m_sut.createMediaKeys(mediaKeysHandle, keySystems[0]));
}

void CdmServiceTests::destroyMediaKeysShouldSucceed()
{
    EXPECT_TRUE(m_sut.destroyMediaKeys(mediaKeysHandle));
}

void CdmServiceTests::destroyMediaKeysShouldFail()
{
    EXPECT_FALSE(m_sut.destroyMediaKeys(mediaKeysHandle));
}

void CdmServiceTests::createKeySessionShouldSucceed()
{
    int32_t returnKeySessionId = -1;
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK,
              m_sut.createKeySession(mediaKeysHandle, keySessionType, m_mediaKeysClientMock, isLDL, returnKeySessionId));
    EXPECT_GE(returnKeySessionId, -1);
}

void CdmServiceTests::createKeySessionShouldFailWithReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    int32_t returnKeySessionId = -1;
    EXPECT_EQ(status,
              m_sut.createKeySession(mediaKeysHandle, keySessionType, m_mediaKeysClientMock, isLDL, returnKeySessionId));
}

void CdmServiceTests::generateRequestShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.generateRequest(mediaKeysHandle, keySessionId, initDataType, initData));
}

void CdmServiceTests::loadSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.loadSession(mediaKeysHandle, keySessionId));
}

void CdmServiceTests::updateSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.updateSession(mediaKeysHandle, keySessionId, responseData));
}

void CdmServiceTests::closeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.closeKeySession(mediaKeysHandle, keySessionId));
}

void CdmServiceTests::removeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.removeKeySession(mediaKeysHandle, keySessionId));
}

void CdmServiceTests::getCdmKeySessionIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    std::string cdmKeySessionId;
    EXPECT_EQ(status, m_sut.getCdmKeySessionId(mediaKeysHandle, keySessionId, cdmKeySessionId));
}

void CdmServiceTests::decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    GstBuffer encryptedData{};
    GstBuffer subSample{};
    GstBuffer IV{};
    GstBuffer keyId{};
    EXPECT_EQ(status,
              m_sut.decrypt(keySessionId, &encryptedData, &subSample, subSampleCount, &IV, &keyId, initWithLast15));
}

void CdmServiceTests::selectKeyIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.selectKeyId(keySessionId, keyId));
}

void CdmServiceTests::containsKeyShouldReturn(bool result)
{
    EXPECT_EQ(result, m_sut.containsKey(mediaKeysHandle, keySessionId, keyId));
}

void CdmServiceTests::setDrmHeaderShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.setDrmHeader(mediaKeysHandle, keySessionId, drmHeader));
}

void CdmServiceTests::deleteDrmStoreShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.deleteDrmStore(mediaKeysHandle));
}

void CdmServiceTests::deleteKeyStoreShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.deleteKeyStore(mediaKeysHandle));
}

void CdmServiceTests::getDrmStoreHashShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    std::vector<unsigned char> drmStoreHash;
    EXPECT_EQ(status, m_sut.getDrmStoreHash(mediaKeysHandle, drmStoreHash));
}

void CdmServiceTests::getKeyStoreHashShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    std::vector<unsigned char> keyStoreHash;
    EXPECT_EQ(status, m_sut.getKeyStoreHash(mediaKeysHandle, keyStoreHash));
}

void CdmServiceTests::getLdlSessionsLimitShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    uint32_t ldlLimit;
    EXPECT_EQ(status, m_sut.getLdlSessionsLimit(mediaKeysHandle, ldlLimit));
}

void CdmServiceTests::getLastDrmErrorShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    uint32_t errorCode;
    EXPECT_EQ(status, m_sut.getLastDrmError(mediaKeysHandle, keySessionId, errorCode));
}

void CdmServiceTests::getDrmTimeShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    uint64_t drmTime;
    EXPECT_EQ(status, m_sut.getDrmTime(mediaKeysHandle, drmTime));
}

void CdmServiceTests::isNetflixKeySystemShouldReturn(bool result)
{
    EXPECT_EQ(result, m_sut.isNetflixKeySystem(keySessionId));
}

void CdmServiceTests::getSupportedKeySystemsShouldSucceed()
{
    EXPECT_EQ(m_sut.getSupportedKeySystems(), keySystems);
}

void CdmServiceTests::getSupportedKeySystemsReturnNon()
{
    EXPECT_EQ(m_sut.getSupportedKeySystems(), std::vector<std::string>{});
}

void CdmServiceTests::supportsKeySystemReturnTrue()
{
    EXPECT_TRUE(m_sut.supportsKeySystem(keySystems[0]));
}

void CdmServiceTests::supportsKeySystemReturnFalse()
{
    EXPECT_FALSE(m_sut.supportsKeySystem(keySystems[0]));
}

void CdmServiceTests::getSupportedKeySystemVersionShouldSucceed()
{
    std::string returnVersion;
    EXPECT_TRUE(m_sut.getSupportedKeySystemVersion(keySystems[0], returnVersion));
    EXPECT_EQ(returnVersion, version);
}

void CdmServiceTests::getSupportedKeySystemVersionShouldFail()
{
    std::string returnVersion;
    EXPECT_FALSE(m_sut.getSupportedKeySystemVersion(keySystems[0], returnVersion));
}

void CdmServiceTests::incrementSessionIdUsageCounter()
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(keySessionId)).WillOnce(Return(true));
    m_sut.incrementSessionIdUsageCounter(keySessionId);
}

void CdmServiceTests::incrementSessionIdUsageCounterSessionNotFound()
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(keySessionId)).WillOnce(Return(false));
    m_sut.incrementSessionIdUsageCounter(keySessionId);
}

void CdmServiceTests::decrementSessionIdUsageCounterAndCloseSession()
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(keySessionId)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaKeysMock, closeKeySession(keySessionId)).WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
    m_sut.decrementSessionIdUsageCounter(keySessionId);
}

void CdmServiceTests::decrementSessionIdUsageCounterAndNoCloseSession()
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(keySessionId)).WillOnce(Return(true));
    m_sut.decrementSessionIdUsageCounter(keySessionId);
}

void CdmServiceTests::decrementSessionIdUsageCounterSessionNotFound()
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(keySessionId)).WillOnce(Return(false));
    m_sut.decrementSessionIdUsageCounter(keySessionId);
}
