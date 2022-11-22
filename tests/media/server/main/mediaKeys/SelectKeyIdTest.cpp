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

class RialtoServerMediaKeysSelectKeyIdTest : public MediaKeysTestBase
{
protected:
    const std::vector<uint8_t> m_kKeyId{1, 2, 3};

    RialtoServerMediaKeysSelectKeyIdTest()
    {
        createMediaKeys(kWidevineKeySystem);
        createKeySession(kWidevineKeySystem);
    }
    ~RialtoServerMediaKeysSelectKeyIdTest() { destroyMediaKeys(); }
};

/**
 * Test that SelectKeyId can execute successfully.
 */
TEST_F(RialtoServerMediaKeysSelectKeyIdTest, Success)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, selectKeyId(m_kKeyId)).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->selectKeyId(m_kKeySessionId, m_kKeyId));
}

/**
 * Test that SelectKeyId fails if the key session does not exsist.
 */
TEST_F(RialtoServerMediaKeysSelectKeyIdTest, SessionDoesNotExistFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(MediaKeyErrorStatus::BAD_SESSION_ID, m_mediaKeys->selectKeyId(m_kKeySessionId + 1, m_kKeyId));
}

/**
 * Test that SelectKeyId fails if the session api fails.
 */
TEST_F(RialtoServerMediaKeysSelectKeyIdTest, SelectKeyIdFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, selectKeyId(m_kKeyId)).WillOnce(Return(MediaKeyErrorStatus::INVALID_STATE));

    EXPECT_EQ(MediaKeyErrorStatus::INVALID_STATE, m_mediaKeys->selectKeyId(m_kKeySessionId, m_kKeyId));
}
