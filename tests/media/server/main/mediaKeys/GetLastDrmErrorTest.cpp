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

#include "MediaKeysTestBase.h"

class RialtoServerMediaKeysGetLastDrmErrorTest : public MediaKeysTestBase
{
protected:
    uint32_t m_lastDrmError{0};
    RialtoServerMediaKeysGetLastDrmErrorTest()
    {
        createMediaKeys(kNetflixKeySystem);
        createKeySession(kNetflixKeySystem);
    }
};

/**
 * Test that getLastDrmError finishes successfully.
 */
TEST_F(RialtoServerMediaKeysGetLastDrmErrorTest, Success)
{
    EXPECT_CALL(*m_mediaKeySessionMock, getLastDrmError(m_lastDrmError)).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->getLastDrmError(m_keySessionId, m_lastDrmError));
}

/**
 * Test that getLastDrmError fails if the key session does not exsist.
 */
TEST_F(RialtoServerMediaKeysGetLastDrmErrorTest, SessionDoesNotExistFailure)
{
    EXPECT_EQ(MediaKeyErrorStatus::BAD_SESSION_ID, m_mediaKeys->getLastDrmError(m_keySessionId + 1, m_lastDrmError));
}

/**
 * Test that getLastDrmError fails if the session api fails.
 */
TEST_F(RialtoServerMediaKeysGetLastDrmErrorTest, getLastDrmErrorFailure)
{
    EXPECT_CALL(*m_mediaKeySessionMock, getLastDrmError(m_lastDrmError)).WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));

    EXPECT_EQ(MediaKeyErrorStatus::NOT_SUPPORTED, m_mediaKeys->getLastDrmError(m_keySessionId, m_lastDrmError));
}
