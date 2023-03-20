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

MATCHER(nullptrMatcher, "")
{
    return arg == nullptr;
}

MATCHER(notNullptrMatcher, "")
{
    return arg != nullptr;
}

ACTION_P(memcpyChallenge, vec)
{
    memcpy(arg1, &vec[0], vec.size());
}

class RialtoServerMediaKeySessionGenerateRequestTest : public MediaKeySessionTestBase
{
protected:
    const InitDataType m_kInitDataType = InitDataType::CENC;
    const std::vector<uint8_t> m_kInitData{1, 2, 3};
    const std::vector<unsigned char> m_kChallenge{'d', 'e', 'f'};

    ~RialtoServerMediaKeySessionGenerateRequestTest() { destroyKeySession(); }
};

/**
 * Test that GenerateRequest can generate request successfully for an none netflix keysystem.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, SuccessNoneNetflix)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData));

    // Close ocdm before destroying
    expectCloseKeySession(kWidevineKeySystem);
}

/**
 * Test that GenerateRequest can generate request successfully for a netflix keysystem.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, SuccessNetflix)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, nullptrMatcher(), _))
        .WillOnce(DoAll(SetArgPointee<2>(m_kChallenge.size()), Return(MediaKeyErrorStatus::OK)));
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, notNullptrMatcher(), _))
        .WillOnce(DoAll(memcpyChallenge(m_kChallenge), Return(MediaKeyErrorStatus::OK)));
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRequest(m_kKeySessionId, m_kChallenge, _));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData));

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
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData));

    // Generate request again should just return OK
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData));

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
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));
    EXPECT_EQ(MediaKeyErrorStatus::NOT_SUPPORTED, m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData));
}

/**
 * Test that GenerateRequest fails if ocdm onError is called during the operation.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, OnErrorFailure)
{
    createKeySession(kWidevineKeySystem);
    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size()))
        .WillOnce(Invoke([this](KeySessionType sessionType, InitDataType initDataType, const uint8_t initData[], uint32_t initDataSize)
            {
                m_mediaKeySession->onError("Failure");
                return MediaKeyErrorStatus::OK;
            }));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData));

    // OcdmSession will be closed on destruction
    expectCloseKeySession(kWidevineKeySystem);
}
