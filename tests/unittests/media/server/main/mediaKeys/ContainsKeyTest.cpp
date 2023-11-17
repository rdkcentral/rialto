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

class RialtoServerMediaKeysContainsKeyTest : public MediaKeysTestBase
{
protected:
    const std::vector<uint8_t> m_kKeyId{1, 2, 3};

    RialtoServerMediaKeysContainsKeyTest()
    {
        createMediaKeys(kNetflixKeySystem);
        createKeySession(kNetflixKeySystem);
    }
    ~RialtoServerMediaKeysContainsKeyTest() { destroyMediaKeys(); }
};

/**
 * Test that ContainsKey returns true.
 */
TEST_F(RialtoServerMediaKeysContainsKeyTest, containsKeyTrue)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, containsKey(m_kKeyId)).WillOnce(Return(true));

    EXPECT_TRUE(m_mediaKeys->containsKey(m_kKeySessionId, m_kKeyId));
}

/**
 * Test that ContainsKey returns false the key session does not exsist.
 */
TEST_F(RialtoServerMediaKeysContainsKeyTest, SessionDoesNotExistFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaKeys->containsKey(m_kKeySessionId + 1, m_kKeyId));
}

/**
 * Test that ContainsKey returns false.
 */
TEST_F(RialtoServerMediaKeysContainsKeyTest, containsKeyFalse)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, containsKey(m_kKeyId)).WillOnce(Return(false));

    EXPECT_FALSE(m_mediaKeys->containsKey(m_kKeySessionId, m_kKeyId));
}
