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

class RialtoServerMediaKeySessionCloseKeySessionTest : public MediaKeySessionTestBase
{
};

/**
 * Test that CloseKeySession can close successfully for netflix key system.
 */
TEST_F(RialtoServerMediaKeySessionCloseKeySessionTest, SuccessNetflix)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, cancelChallengeData()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmSessionMock, cleanDecryptContext()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->closeKeySession());
}

/**
 * Test that CloseKeySession can close successfully for none netflix key systems.
 */
TEST_F(RialtoServerMediaKeySessionCloseKeySessionTest, SuccessNoneNetflix)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, close()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->closeKeySession());
}

/**
 * Test that CloseKeySession fails if the ocdm session api cancelChallengeData fails.
 */
TEST_F(RialtoServerMediaKeySessionCloseKeySessionTest, OcdmSessionCancelChallengeDataFailure)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, cancelChallengeData()).WillOnce(Return(MediaKeyErrorStatus::INVALID_STATE));

    EXPECT_EQ(MediaKeyErrorStatus::INVALID_STATE, m_mediaKeySession->closeKeySession());
}

/**
 * Test that CloseKeySession fails if the ocdm session api cleanDecryptContext fails.
 */
TEST_F(RialtoServerMediaKeySessionCloseKeySessionTest, OcdmSessionCleanDecryptContextFailure)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, cancelChallengeData()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmSessionMock, cleanDecryptContext()).WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));

    EXPECT_EQ(MediaKeyErrorStatus::NOT_SUPPORTED, m_mediaKeySession->closeKeySession());
}

/**
 * Test that CloseKeySession fails if the ocdm session api Close fails.
 */
TEST_F(RialtoServerMediaKeySessionCloseKeySessionTest, OcdmCloseFailure)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, close()).WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));

    EXPECT_EQ(MediaKeyErrorStatus::NOT_SUPPORTED, m_mediaKeySession->closeKeySession());
}

/**
 * Test that CloseKeySession fails if the ocdm session api DestructSession fails.
 */
TEST_F(RialtoServerMediaKeySessionCloseKeySessionTest, OcdmDestructSessionFailure)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, close()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::INVALID_STATE));

    EXPECT_EQ(MediaKeyErrorStatus::INVALID_STATE, m_mediaKeySession->closeKeySession());
}
