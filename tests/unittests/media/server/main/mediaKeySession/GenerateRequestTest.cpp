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

#include "MediaKeySessionTestBase.h"

using ::testing::DoAll;
using ::testing::SetArgPointee;

class RialtoServerMediaKeySessionGenerateRequestTest : public MediaKeySessionTestBase
{
protected:
    ~RialtoServerMediaKeySessionGenerateRequestTest() { destroyKeySession(); }
};

/**
 * Test that GenerateRequest can generate request successfully for an none netflix keysystem.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, SuccessNoneNetflix)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size(), _, _))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK,
              m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData, std::vector<uint8_t>{}, m_kLdlState));

    // Close ocdm before destroying
    expectCloseKeySession(kWidevineKeySystem);
}

/**
 * Test that queued drm header is consumed after construction, non-empty cdmData is forwarded to constructSession,
 * and the extended interface path is used.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, SuccessWithQueuedDrmHeaderAndNonEmptyCdmDataUsesExtendedInterface)
{
    createKeySession(kWidevineKeySystem);

    const std::vector<uint8_t> kDrmHeader{3, 2, 1};
    const std::vector<uint8_t> kCdmData{7, 8, 9};

    // Session is not constructed yet, so header is queued and extended interface is enabled.
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->setDrmHeader(kDrmHeader));

    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size(), _, _))
        .WillOnce(Invoke(
            [&kCdmData](KeySessionType sessionType, InitDataType initDataType, const uint8_t initData[],
                        uint32_t initDataSize, const uint8_t cdmData[], uint32_t cdmDataSize)
            {
                EXPECT_NE(cdmData, nullptr);
                EXPECT_EQ(cdmDataSize, kCdmData.size());
                EXPECT_TRUE(std::equal(cdmData, cdmData + cdmDataSize, kCdmData.begin()));

                return MediaKeyErrorStatus::OK;
            }));

    EXPECT_CALL(*m_ocdmSessionMock, setDrmHeader(_, kDrmHeader.size()))
        .WillOnce(Invoke(
            [&kDrmHeader](const uint8_t drmHeader[], uint32_t drmHeaderSize)
            {
                EXPECT_NE(drmHeader, nullptr);
                EXPECT_EQ(drmHeaderSize, kDrmHeader.size());
                EXPECT_TRUE(std::equal(drmHeader, drmHeader + drmHeaderSize, kDrmHeader.begin()));

                return MediaKeyErrorStatus::OK;
            }));

    // Extended interface path triggers a manual challenge retrieval.
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, nullptrMatcher(), _))
        .WillOnce(DoAll(SetArgPointee<2>(m_kChallenge.size()), Return(MediaKeyErrorStatus::OK)));
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, notNullptrMatcher(), _))
        .WillOnce(DoAll(memcpyChallenge(m_kChallenge), Return(MediaKeyErrorStatus::OK)));
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRequest(m_kKeySessionId, m_kChallenge, _));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData, kCdmData,
                                                                          LimitedDurationLicense::NOT_SPECIFIED));

    // Extended interface path closes via challenge cancellation and decrypt context cleanup.
    EXPECT_CALL(*m_ocdmSessionMock, cancelChallengeData()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmSessionMock, cleanDecryptContext()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));
}

/**
 * Test that non-empty cdmData (without LDL) engages the extended interface path.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, SuccessWithNonEmptyCdmDataUsesExtendedInterfaceWithoutLdl)
{
    createKeySession(kWidevineKeySystem);

    const std::vector<uint8_t> kCdmData{7, 8, 9};
    const std::vector<uint8_t> kResponseData{4, 5, 6};

    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size(), _, _))
        .WillOnce(Invoke(
            [&kCdmData](KeySessionType sessionType, InitDataType initDataType, const uint8_t initData[],
                        uint32_t initDataSize, const uint8_t cdmData[], uint32_t cdmDataSize)
            {
                EXPECT_NE(cdmData, nullptr);
                EXPECT_EQ(cdmDataSize, kCdmData.size());
                EXPECT_TRUE(std::equal(cdmData, cdmData + cdmDataSize, kCdmData.begin()));

                return MediaKeyErrorStatus::OK;
            }));

    // Extended interface path triggers a manual challenge retrieval.
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, nullptrMatcher(), _))
        .WillOnce(DoAll(SetArgPointee<2>(m_kChallenge.size()), Return(MediaKeyErrorStatus::OK)));
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, notNullptrMatcher(), _))
        .WillOnce(DoAll(memcpyChallenge(m_kChallenge), Return(MediaKeyErrorStatus::OK)));
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRequest(m_kKeySessionId, m_kChallenge, _));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData, kCdmData,
                                                                          LimitedDurationLicense::NOT_SPECIFIED));

    EXPECT_CALL(*m_ocdmSessionMock, update(_, _)).Times(0);
    EXPECT_CALL(*m_ocdmSessionMock, storeLicenseData(kResponseData.data(), kResponseData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->updateSession(kResponseData));

    // Extended interface path closes via challenge cancellation and decrypt context cleanup.
    EXPECT_CALL(*m_ocdmSessionMock, cancelChallengeData()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmSessionMock, cleanDecryptContext()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));
}

/**
 * Test that persistent license restore flow (non-empty cdmData with no LDL) does not fetch challenge data.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, SuccessPersistentRestoreWithCdmDataDoesNotGenerateChallenge)
{
    m_keySessionType = KeySessionType::PERSISTENT_LICENCE;
    createKeySession(kWidevineKeySystem);

    const std::vector<uint8_t> kCdmData{7, 8, 9};
    const std::vector<uint8_t> kResponseData{4, 5, 6};

    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size(), _, _))
        .WillOnce(Invoke(
            [&kCdmData](KeySessionType sessionType, InitDataType initDataType, const uint8_t initData[],
                        uint32_t initDataSize, const uint8_t cdmData[], uint32_t cdmDataSize)
            {
                EXPECT_NE(cdmData, nullptr);
                EXPECT_EQ(cdmDataSize, kCdmData.size());
                EXPECT_TRUE(std::equal(cdmData, cdmData + cdmDataSize, kCdmData.begin()));

                return MediaKeyErrorStatus::OK;
            }));

    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(_, _, _)).Times(0);
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRequest(_, _, _)).Times(0);

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData, kCdmData,
                                                                          LimitedDurationLicense::NOT_SPECIFIED));

    EXPECT_CALL(*m_ocdmSessionMock, update(_, _)).Times(0);
    EXPECT_CALL(*m_ocdmSessionMock, storeLicenseData(kResponseData.data(), kResponseData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->updateSession(kResponseData));

    EXPECT_CALL(*m_ocdmSessionMock, cancelChallengeData()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmSessionMock, cleanDecryptContext()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));
}

/**
 * Test that GenerateRequest can generate request successfully for a netflix keysystem.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, SuccessNetflix)
{
    createKeySession(kNetflixKeySystem);

    generateRequestPlayready();

    // Close ocdm before destroying
    expectCloseKeySession(kNetflixKeySystem);
}

/**
 * Test that GenerateRequest can generate request successfully for a netflix keysystem.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, SuccessNetflixWithTwoGenerateChallengeCalls)
{
    createKeySession(kNetflixKeySystem);

    generateRequestPlayreadyWithTwoCalls();

    // Close ocdm before destroying
    expectCloseKeySession(kNetflixKeySystem);
}

/**
 * Test that GenerateRequest fails when returned challenge data size is zero
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, FailNetflixWhenChallengeDataSizeIsZero)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size(), _, _))
        .WillOnce(Return(MediaKeyErrorStatus::OK));
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, nullptrMatcher(), _))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK,
              m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData, std::vector<uint8_t>{},
                                                 firebolt::rialto::LimitedDurationLicense::DISABLED));

    // Close ocdm before destroying
    expectCloseKeySession(kNetflixKeySystem);
}

/**
 * Test that GenerateRequest fails when get challenge data fails
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, FailNetflixWhenGettingChallengeDataFails)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size(), _, _))
        .WillOnce(Return(MediaKeyErrorStatus::OK));
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, nullptrMatcher(), _))
        .WillOnce(DoAll(SetArgPointee<2>(m_kChallenge.size()), Return(MediaKeyErrorStatus::OK)));
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, notNullptrMatcher(), _))
        .WillOnce(DoAll(memcpyChallenge(m_kChallenge), Return(MediaKeyErrorStatus::FAIL)));

    EXPECT_EQ(MediaKeyErrorStatus::OK,
              m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData, std::vector<uint8_t>{},
                                                 firebolt::rialto::LimitedDurationLicense::DISABLED));

    // Close ocdm before destroying
    expectCloseKeySession(kNetflixKeySystem);
}

/**
 * Test that GenerateRequest manually fetches the challenge if the session has already been constructed.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, SessionAlreadyConstructed)
{
    // Generate inital request
    createKeySession(kWidevineKeySystem);
    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size(), _, _))
        .WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_EQ(MediaKeyErrorStatus::OK,
              m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData, std::vector<uint8_t>{}, m_kLdlState));

    // Generate request again should just return OK
    EXPECT_EQ(MediaKeyErrorStatus::OK,
              m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData, std::vector<uint8_t>{}, m_kLdlState));

    // OcdmSession will be closed on destruction
    expectCloseKeySession(kWidevineKeySystem);
}

/**
 * Test that GenerateRequest fails if the ocdm session api fails.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, OcdmSessionFailure)
{
    createKeySession(kWidevineKeySystem);
    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size(), _, _))
        .WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));
    EXPECT_EQ(MediaKeyErrorStatus::NOT_SUPPORTED,
              m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData, std::vector<uint8_t>{}, m_kLdlState));
}

/**
 * Test that GenerateRequest fails if ocdm onError is called during the operation.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, OnErrorFailure)
{
    createKeySession(kWidevineKeySystem);
    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size(), _, _))
        .WillOnce(Invoke(
            [this](KeySessionType sessionType, InitDataType initDataType, const uint8_t initData[],
                   uint32_t initDataSize, const uint8_t cdmData[], uint32_t cdmDataSize)
            {
                m_mediaKeySession->onError("Failure");
                return MediaKeyErrorStatus::OK;
            }));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL,
              m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData, std::vector<uint8_t>{}, m_kLdlState));

    // OcdmSession will be closed on destruction
    expectCloseKeySession(kWidevineKeySystem);
}
