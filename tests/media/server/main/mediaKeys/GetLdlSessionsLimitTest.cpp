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

class RialtoServerMediaKeysGetLdlSessionsLimitTest : public MediaKeysTestBase
{
protected:
    std::uint32_t m_ldlSessionsLimit{0};
    RialtoServerMediaKeysGetLdlSessionsLimitTest()
    {
        createMediaKeys(kNetflixKeySystem);
        createKeySession(kNetflixKeySystem);
    }
};

/**
 * Test that GetLdlSessionsLimit returns success.
 */
TEST_F(RialtoServerMediaKeysGetLdlSessionsLimitTest, Success)
{
    EXPECT_CALL(*m_ocdmSystemMock, getLdlSessionsLimit(&m_ldlSessionsLimit)).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->getLdlSessionsLimit(m_ldlSessionsLimit));
}

/**
 * Test that GetLdlSessionsLimit fails if the session api fails.
 */
TEST_F(RialtoServerMediaKeysGetLdlSessionsLimitTest, Failure)
{
    EXPECT_CALL(*m_ocdmSystemMock, getLdlSessionsLimit(&m_ldlSessionsLimit))
        .WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));

    EXPECT_EQ(MediaKeyErrorStatus::NOT_SUPPORTED, m_mediaKeys->getLdlSessionsLimit(m_ldlSessionsLimit));
}
