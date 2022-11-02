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

class RialtoServerMediaKeysGenerateRequestTest : public MediaKeysTestBase
{
protected:
    InitDataType m_initDataType = InitDataType::CENC;
    std::vector<uint8_t> m_initData{1, 2, 3};

    RialtoServerMediaKeysGenerateRequestTest()
    {
        createMediaKeys(kNetflixKeySystem);
        createKeySession(kNetflixKeySystem);
    }
};

/**
 * Test that GenerateRequest can generate request successfully.
 */
TEST_F(RialtoServerMediaKeysGenerateRequestTest, Success)
{
    EXPECT_CALL(*m_mediaKeySessionMock, generateRequest(m_initDataType, m_initData))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->generateRequest(m_keySessionId, m_initDataType, m_initData));
}

/**
 * Test that GenerateRequest fails if the key session does not exsist.
 */
TEST_F(RialtoServerMediaKeysGenerateRequestTest, SessionDoesNotExistFailure)
{
    EXPECT_EQ(MediaKeyErrorStatus::BAD_SESSION_ID,
              m_mediaKeys->generateRequest(m_keySessionId + 1, m_initDataType, m_initData));
}

/**
 * Test that GenerateRequest fails if the session api fails.
 */
TEST_F(RialtoServerMediaKeysGenerateRequestTest, SessionFailure)
{
    EXPECT_CALL(*m_mediaKeySessionMock, generateRequest(m_initDataType, m_initData))
        .WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));

    EXPECT_EQ(MediaKeyErrorStatus::NOT_SUPPORTED,
              m_mediaKeys->generateRequest(m_keySessionId, m_initDataType, m_initData));
}
