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

class RialtoServerMediaKeysGetCdmKeySessionIdTest : public MediaKeysTestBase
{
protected:
    RialtoServerMediaKeysGetCdmKeySessionIdTest()
    {
        createMediaKeys(kNetflixKeySystem);
        createKeySession(kNetflixKeySystem);
    }
};

/**
 * Test that GetCdmKeySessionId can execute successfully.
 */
TEST_F(RialtoServerMediaKeysGetCdmKeySessionIdTest, Success)
{
    std::string cdmKeySessionId;
    EXPECT_CALL(*m_mediaKeySessionMock, getCdmKeySessionId(cdmKeySessionId)).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->getCdmKeySessionId(m_kKeySessionId, cdmKeySessionId));
}

/**
 * Test that GetCdmKeySessionId fails if the key session does not exsist.
 */
TEST_F(RialtoServerMediaKeysGetCdmKeySessionIdTest, SessionDoesNotExistFailure)
{
    std::string cdmKeySessionId;
    EXPECT_EQ(MediaKeyErrorStatus::BAD_SESSION_ID, m_mediaKeys->getCdmKeySessionId(m_kKeySessionId + 1, cdmKeySessionId));
}

/**
 * Test that GetCdmKeySessionId fails if the session api fails.
 */
TEST_F(RialtoServerMediaKeysGetCdmKeySessionIdTest, SessionFailure)
{
    std::string cdmKeySessionId;
    EXPECT_CALL(*m_mediaKeySessionMock, getCdmKeySessionId(cdmKeySessionId)).WillOnce(Return(MediaKeyErrorStatus::FAIL));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeys->getCdmKeySessionId(m_kKeySessionId, cdmKeySessionId));
}
