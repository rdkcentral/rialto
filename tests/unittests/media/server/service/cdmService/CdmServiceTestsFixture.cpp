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
const std::vector<std::string> kKeySystems{"expectedKeySystem1", "expectedKeySystem2", "expectedKeySystem3"};
const std::string kVersion{"123"};
constexpr int kMediaKeysHandle{2};
constexpr firebolt::rialto::KeySessionType kKeySessionType{firebolt::rialto::KeySessionType::TEMPORARY};
constexpr bool kIsLDL{false};
constexpr int kKeySessionId{3};
constexpr firebolt::rialto::InitDataType kInitDataType{firebolt::rialto::InitDataType::CENC};
const std::vector<std::uint8_t> kInitData{6, 7, 2};
const std::vector<std::uint8_t> kResponseData{9, 7, 8};
const std::vector<uint8_t> keyId{1, 4, 7};
const std::vector<uint8_t> kDrmHeader{4, 9, 3};
const uint32_t kSubSampleCount{2};
constexpr uint32_t kInitWithLast15{1};
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
      m_heartbeatProcedureMock{std::make_shared<StrictMock<firebolt::rialto::server::HeartbeatProcedureMock>>()},
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
    EXPECT_CALL(m_mediaKeysCapabilitiesMock, getSupportedKeySystems()).WillOnce(Return(kKeySystems));
}

void CdmServiceTests::supportsKeySystemWillReturnTrue()
{
    EXPECT_CALL(m_mediaKeysCapabilitiesMock, supportsKeySystem(kKeySystems[0])).WillOnce(Return(true));
}

void CdmServiceTests::getSupportedKeySystemVersionWillSucceed()
{
    EXPECT_CALL(m_mediaKeysCapabilitiesMock, getSupportedKeySystemVersion(kKeySystems[0], _))
        .WillOnce(DoAll(SetArgReferee<1>(kVersion), Return(true)));
}

void CdmServiceTests::getSupportedKeySystemVersionWillFail()
{
    EXPECT_CALL(m_mediaKeysCapabilitiesMock, getSupportedKeySystemVersion(kKeySystems[0], _))
        .WillOnce(DoAll(SetArgReferee<1>(kVersion), Return(false)));
}

void CdmServiceTests::triggerSwitchToActiveSuccess()
{
    EXPECT_TRUE(m_sut.switchToActive());
}

void CdmServiceTests::triggerSwitchToInactive()
{
    m_sut.switchToInactive();
}

void CdmServiceTests::triggerPing()
{
    m_sut.ping(m_heartbeatProcedureMock);
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
    EXPECT_CALL(m_mediaKeysMock, createKeySession(kKeySessionType, _, kIsLDL, _))
        .WillOnce(DoAll(SetArgReferee<3>(kKeySessionId), Return(status)));
}

void CdmServiceTests::mediaKeysWillGenerateRequestWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, generateRequest(kKeySessionId, kInitDataType, kInitData)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillLoadSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, loadSession(kKeySessionId)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillUpdateSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, updateSession(kKeySessionId, kResponseData)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillCloseKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, closeKeySession(kKeySessionId)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillRemoveKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, removeKeySession(kKeySessionId)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillGetCdmKeySessionIdWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, getCdmKeySessionId(kKeySessionId, _)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillCheckIfKeyIsPresent(bool result)
{
    EXPECT_CALL(m_mediaKeysMock, containsKey(kKeySessionId, keyId)).WillOnce(Return(result));
}

void CdmServiceTests::mediaKeysWillSetDrmHeaderWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, setDrmHeader(kKeySessionId, kDrmHeader)).WillOnce(Return(status));
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
    EXPECT_CALL(m_mediaKeysMock, getLastDrmError(kKeySessionId, _)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillGetDrmTimeWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, getDrmTime(_)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillDecryptDeprecatedWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(kKeySessionId)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaKeysMock, decrypt(kKeySessionId, _, _, kSubSampleCount, _, _, kInitWithLast15, _))
        .WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillDecryptWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(kKeySessionId)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaKeysMock, decrypt(kKeySessionId, _, _)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillSelectKeyIdWithStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(kKeySessionId)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaKeysMock, selectKeyId(kKeySessionId, keyId)).WillOnce(Return(status));
}

void CdmServiceTests::mediaKeysWillNotFindMediaKeySession()
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(kKeySessionId)).WillOnce(Return(false));
}

void CdmServiceTests::mediaKeysWillCheckIfKeySystemIsPlayready(bool result)
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(kKeySessionId)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaKeysMock, isPlayreadyKeySystem(kKeySessionId)).WillOnce(Return(result));
}

void CdmServiceTests::mediaKeysWillPing()
{
    EXPECT_CALL(*m_heartbeatProcedureMock, createHandler());
    EXPECT_CALL(m_mediaKeysMock, ping(_));
}

void CdmServiceTests::createMediaKeysShouldSucceed()
{
    EXPECT_TRUE(m_sut.createMediaKeys(kMediaKeysHandle, kKeySystems[0]));
}

void CdmServiceTests::createMediaKeysShouldFail()
{
    EXPECT_FALSE(m_sut.createMediaKeys(kMediaKeysHandle, kKeySystems[0]));
}

void CdmServiceTests::destroyMediaKeysShouldSucceed()
{
    EXPECT_TRUE(m_sut.destroyMediaKeys(kMediaKeysHandle));
}

void CdmServiceTests::destroyMediaKeysShouldFail()
{
    EXPECT_FALSE(m_sut.destroyMediaKeys(kMediaKeysHandle));
}

void CdmServiceTests::createKeySessionShouldSucceed()
{
    int32_t returnKeySessionId = -1;
    EXPECT_EQ(firebolt::rialto::MediaKeyErrorStatus::OK,
              m_sut.createKeySession(kMediaKeysHandle, kKeySessionType, m_mediaKeysClientMock, kIsLDL,
                                     returnKeySessionId));
    EXPECT_GE(returnKeySessionId, -1);
}

void CdmServiceTests::createKeySessionShouldFailWithReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    int32_t returnKeySessionId = -1;
    EXPECT_EQ(status, m_sut.createKeySession(kMediaKeysHandle, kKeySessionType, m_mediaKeysClientMock, kIsLDL,
                                             returnKeySessionId));
}

void CdmServiceTests::generateRequestShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.generateRequest(kMediaKeysHandle, kKeySessionId, kInitDataType, kInitData));
}

void CdmServiceTests::loadSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.loadSession(kMediaKeysHandle, kKeySessionId));
}

void CdmServiceTests::updateSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.updateSession(kMediaKeysHandle, kKeySessionId, kResponseData));
}

void CdmServiceTests::closeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.closeKeySession(kMediaKeysHandle, kKeySessionId));
}

void CdmServiceTests::removeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.removeKeySession(kMediaKeysHandle, kKeySessionId));
}

void CdmServiceTests::getCdmKeySessionIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    std::string cdmKeySessionId;
    EXPECT_EQ(status, m_sut.getCdmKeySessionId(kMediaKeysHandle, kKeySessionId, cdmKeySessionId));
}

void CdmServiceTests::decryptDeprecatedShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    GstBuffer encryptedData{};
    GstBuffer subSample{};
    GstBuffer IV{};
    GstBuffer keyId{};
    GstCaps caps{};
    EXPECT_EQ(status, m_sut.decrypt(kKeySessionId, &encryptedData, &subSample, kSubSampleCount, &IV, &keyId,
                                    kInitWithLast15, &caps));
}

void CdmServiceTests::decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    GstBuffer encryptedData{};
    GstCaps caps{};
    EXPECT_EQ(status, m_sut.decrypt(kKeySessionId, &encryptedData, &caps));
}

void CdmServiceTests::selectKeyIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.selectKeyId(kKeySessionId, keyId));
}

void CdmServiceTests::containsKeyShouldReturn(bool result)
{
    EXPECT_EQ(result, m_sut.containsKey(kMediaKeysHandle, kKeySessionId, keyId));
}

void CdmServiceTests::setDrmHeaderShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.setDrmHeader(kMediaKeysHandle, kKeySessionId, kDrmHeader));
}

void CdmServiceTests::deleteDrmStoreShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.deleteDrmStore(kMediaKeysHandle));
}

void CdmServiceTests::deleteKeyStoreShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    EXPECT_EQ(status, m_sut.deleteKeyStore(kMediaKeysHandle));
}

void CdmServiceTests::getDrmStoreHashShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    std::vector<unsigned char> drmStoreHash;
    EXPECT_EQ(status, m_sut.getDrmStoreHash(kMediaKeysHandle, drmStoreHash));
}

void CdmServiceTests::getKeyStoreHashShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    std::vector<unsigned char> keyStoreHash;
    EXPECT_EQ(status, m_sut.getKeyStoreHash(kMediaKeysHandle, keyStoreHash));
}

void CdmServiceTests::getLdlSessionsLimitShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    uint32_t ldlLimit;
    EXPECT_EQ(status, m_sut.getLdlSessionsLimit(kMediaKeysHandle, ldlLimit));
}

void CdmServiceTests::getLastDrmErrorShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    uint32_t errorCode;
    EXPECT_EQ(status, m_sut.getLastDrmError(kMediaKeysHandle, kKeySessionId, errorCode));
}

void CdmServiceTests::getDrmTimeShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status)
{
    uint64_t drmTime;
    EXPECT_EQ(status, m_sut.getDrmTime(kMediaKeysHandle, drmTime));
}

void CdmServiceTests::isPlayreadyKeySystemShouldReturn(bool result)
{
    EXPECT_EQ(result, m_sut.isPlayreadyKeySystem(kKeySessionId));
}

void CdmServiceTests::getSupportedKeySystemsShouldSucceed()
{
    EXPECT_EQ(m_sut.getSupportedKeySystems(), kKeySystems);
}

void CdmServiceTests::getSupportedKeySystemsReturnNon()
{
    EXPECT_EQ(m_sut.getSupportedKeySystems(), std::vector<std::string>{});
}

void CdmServiceTests::supportsKeySystemReturnTrue()
{
    EXPECT_TRUE(m_sut.supportsKeySystem(kKeySystems[0]));
}

void CdmServiceTests::supportsKeySystemReturnFalse()
{
    EXPECT_FALSE(m_sut.supportsKeySystem(kKeySystems[0]));
}

void CdmServiceTests::getSupportedKeySystemVersionShouldSucceed()
{
    std::string returnVersion;
    EXPECT_TRUE(m_sut.getSupportedKeySystemVersion(kKeySystems[0], returnVersion));
    EXPECT_EQ(returnVersion, kVersion);
}

void CdmServiceTests::getSupportedKeySystemVersionShouldFail()
{
    std::string returnVersion;
    EXPECT_FALSE(m_sut.getSupportedKeySystemVersion(kKeySystems[0], returnVersion));
}

void CdmServiceTests::incrementSessionIdUsageCounter()
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(kKeySessionId)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaKeysMock, incrementSessionIdUsageCounter(kKeySessionId));
    m_sut.incrementSessionIdUsageCounter(kKeySessionId);
}

void CdmServiceTests::incrementSessionIdUsageCounterFails()
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(kKeySessionId)).WillOnce(Return(false));
    m_sut.incrementSessionIdUsageCounter(kKeySessionId);
}

void CdmServiceTests::decrementSessionIdUsageCounter()
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(kKeySessionId)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaKeysMock, decrementSessionIdUsageCounter(kKeySessionId));
    m_sut.decrementSessionIdUsageCounter(kKeySessionId);
}

void CdmServiceTests::decrementSessionIdUsageCounterFails()
{
    EXPECT_CALL(m_mediaKeysMock, hasSession(kKeySessionId)).WillOnce(Return(false));
    m_sut.decrementSessionIdUsageCounter(kKeySessionId);
}
