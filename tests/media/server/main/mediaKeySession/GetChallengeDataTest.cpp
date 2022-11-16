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
using ::testing::Invoke;
using ::testing::InSequence;
using ::testing::SetArgPointee;

class RialtoServerMediaKeySessionGetChallengeDataTest : public MediaKeySessionTestBase
{
protected:
    const std::vector<uint8_t> m_kChallengeData{1, 2, 3};
};

/**
 * Test that MediaKeySession gets challenge data successfully
 */
TEST_F(RialtoServerMediaKeySessionGetChallengeDataTest, Success)
{
    InSequence inSequence{};

    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, nullptr, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_kChallengeData.size()), Return(MediaKeyErrorStatus::OK)));
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, _, _))
        .WillOnce(Invoke([&](bool isLDL, uint8_t *challenge, uint32_t *challengeSize) {
            EXPECT_TRUE(challengeSize);
            EXPECT_EQ(*challengeSize, m_kChallengeData.size());
            memcpy(challenge, m_kChallengeData.data(), m_kChallengeData.size());
            return MediaKeyErrorStatus::OK;
        }));
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRequest(m_keySessionId, m_kChallengeData, _));
    m_mediaKeySession->getChallengeData();
}

/**
 * Test that MediaKeySession fails when MediaKeysClient can't be locked
 */
TEST_F(RialtoServerMediaKeySessionGetChallengeDataTest, FailDueToMediaKeysClientError)
{
    InSequence inSequence{};

    createKeySession(kNetflixKeySystem);

    m_mediaKeysClientMock.reset();
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, nullptr, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_kChallengeData.size()), Return(MediaKeyErrorStatus::OK)));
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, _, _))
        .WillOnce(Invoke([&](bool isLDL, uint8_t *challenge, uint32_t *challengeSize) {
            EXPECT_TRUE(challengeSize);
            EXPECT_EQ(*challengeSize, m_kChallengeData.size());
            memcpy(challenge, m_kChallengeData.data(), m_kChallengeData.size());
            return MediaKeyErrorStatus::OK;
        }));
    m_mediaKeySession->getChallengeData();
}

/**
 * Test that MediaKeySession fails when second getChallengeData Fails
 */
TEST_F(RialtoServerMediaKeySessionGetChallengeDataTest, FailDueToGetChallengeDataError)
{
    InSequence inSequence{};

    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, nullptr, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_kChallengeData.size()), Return(MediaKeyErrorStatus::OK)));
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, _, _))
        .WillOnce(Return(MediaKeyErrorStatus::FAIL));
    m_mediaKeySession->getChallengeData();
}

/**
 * Test that MediaKeySession fails when first getChallengeData Fails
 */
TEST_F(RialtoServerMediaKeySessionGetChallengeDataTest, FailDueToGetChallengeSizeError)
{
    InSequence inSequence{};

    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, nullptr, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_kChallengeData.size()), Return(MediaKeyErrorStatus::FAIL)));
    m_mediaKeySession->getChallengeData();
}

/**
 * Test that MediaKeySession does nothing when key system is not netflix
 */
TEST_F(RialtoServerMediaKeySessionGetChallengeDataTest, SkipForNonNetflixKeySystem)
{
    createKeySession(kWidevineKeySystem);

    m_mediaKeySession->getChallengeData();
}
